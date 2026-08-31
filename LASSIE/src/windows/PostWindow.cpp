#include "PostWindow.hpp"

#include <QMessageBox>
#include <QCloseEvent>
//#include <QOverload>

#include <QVBoxLayout>
#include <QTextCursor>
#include <QTextCharFormat>

#include <algorithm>

PostWindow::PostWindow(QProcess *process, QWidget *parent)
    : QWidget(parent, Qt::Window), proc(process)
{
    setWindowTitle("Process Output");

    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setStyleSheet("background:white;");

    QToolBar *toolbar = new QToolBar(this);
    QAction *bigger = toolbar->addAction("Zoom in");
    QAction *smaller = toolbar->addAction("Zoom out");
    QAction *clear = toolbar->addAction("Clear");
    QAction *toggleScroll = toolbar->addAction("Scroll");
    toggleScroll->setCheckable(true);
    toggleScroll->setChecked(true);

    // spacer for clarity since the remaining actions will be related to the process
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    QAction *termProc = toolbar->addAction("Terminate");
    termProc->setToolTip("Ask CMOD to stop running");
    termProc->setEnabled(false);
    QAction *killProc = toolbar->addAction("Kill");
    killProc->setToolTip("Force CMOD to stop running");
    killProc->setEnabled(false);
    QAction *runProc = toolbar->addAction("Run");
    runProc->setToolTip("Run project (if none currently running)");
    runProc->setEnabled(false);
    
    connect(bigger, &QAction::triggered, this, &PostWindow::increaseFont);
    connect(smaller, &QAction::triggered, this, &PostWindow::decreaseFont);
    connect(clear, &QAction::triggered, this, &PostWindow::clearOutput);
    connect(toggleScroll, &QAction::toggled, this, 
        [this](bool checked){
            autoscroll = checked;
        });
    connect(termProc, &QAction::triggered, this, &PostWindow::termProcess);
    connect(killProc, &QAction::triggered, this, &PostWindow::killProcess);
    connect(runProc, &QAction::triggered, this, &PostWindow::runProcess);
    
    connect(proc, &QProcess::stateChanged, this, [this, termProc, killProc, runProc]{
        if (proc->state() == QProcess::Starting) stopRequested = false;
        if(proc->state() != QProcess::NotRunning){
            termProc->setEnabled(true);
            killProc->setEnabled(true);
            runProc->setEnabled(false);
        }else{
            termProc->setEnabled(false);
            killProc->setEnabled(false);
            runProc->setEnabled(true);
        }
    });

    connect(proc, &QProcess::started, this, [this] {
        resetProcessOutputState();
        appendColored(QStringLiteral("CMOD executable: %1").arg(proc->program()),
                      Qt::black);
    });

    connect(proc, &QProcess::finished, this,
            [this, runProc](int exitCode, QProcess::ExitStatus exitStatus) {
        // Drain anything emitted immediately before the process exited.
        handleStdout();
        handleStderr();

        runProc->setEnabled(true);
        const bool succeeded =
            exitStatus == QProcess::NormalExit && exitCode == 0;
        if (stopRequested) {
            if (!succeeded) recolorStderr(Qt::red);
            appendColored(
                QStringLiteral("*** Process exited after your stop request (%1; exit code %2) ***")
                    .arg(exitStatus == QProcess::CrashExit ? QStringLiteral("abnormal exit")
                         : succeeded ? QStringLiteral("normal exit") : QStringLiteral("failure"))
                    .arg(exitCode), Qt::red);
        } else if (succeeded) {
            appendColored(
                "*** Process exited normally (exit code 0) ***",
                Qt::black);
        } else {
            recolorStderr(Qt::red);
            const QString summary = exitStatus == QProcess::NormalExit
                ? QStringLiteral("*** Process failed (exit code %1) ***")
                      .arg(exitCode)
                : QStringLiteral(
                      "*** Process crashed (abnormal exit; exit code %1) ***")
                      .arg(exitCode);
            appendColored(summary, Qt::red);
            if (exitStatus == QProcess::CrashExit) {
                appendColored(
                    QStringLiteral(
                        "CMOD stopped unexpectedly before the build completed.\n"
                        "Suggestion: Check the CMOD executable path above. If the problem persists, "
                        "send the DISSCO developers the project file, seed, and full output. "
                        "The exit code alone does not identify the cause."),
                    Qt::red);
            }
        }
    });

    connect(proc, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            appendColored(
                QStringLiteral(
                    "*** Process failed to start: %1 ***\n"
                    "CMOD executable: %2\n"
                    "Suggestion: Check that this executable exists and can be run. "
                    "Rebuild or reinstall DISSCO if the executable or its required libraries are missing.")
                    .arg(proc->errorString(), proc->program()),
                Qt::red);
        }
    });
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(toolbar);
    layout->addWidget(textEdit);
    setLayout(layout);

    // feed output to window
    connect(proc, &QProcess::readyReadStandardOutput, this, &PostWindow::handleStdout);
    connect(proc, &QProcess::readyReadStandardError,  this, &PostWindow::handleStderr);
}

void PostWindow::closeEvent(QCloseEvent *event)
{
    if(proc->state() != QProcess::NotRunning){
        QMessageBox message;
        if(proc->state() == QProcess::Running)
            message.setText("CMOD has not yet finished running.");
        else
            message.setText("CMOD has not yet started running.");
        message.setInformativeText("Abort CMOD process?");
        message.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        message.setDefaultButton(QMessageBox::No);

        int ret = message.exec();

        switch(ret){
            case QMessageBox::Yes:
                stopRequested = true;
                proc->kill();
                break;
            case QMessageBox::No:
                event->ignore();
                return;
        }
    }

    QWidget::closeEvent(event);
}

void PostWindow::appendColored(const QString &text, const QColor &color)
{
    QTextCharFormat fmt;
    fmt.setForeground(color);

    textEdit->setCurrentCharFormat(fmt);
    textEdit->append(text);

    scrollToBottom();
}

void PostWindow::appendProcessText(const QString &text, bool fromStderr)
{
    if (text.isEmpty()) {
        return;
    }

    QTextCursor cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;
    format.setForeground(Qt::black);

    const int start = cursor.position();
    cursor.insertText(text, format);
    const int end = cursor.position();
    textEdit->setTextCursor(cursor);

    if (fromStderr) {
        stderrRanges.push_back({start, end});
    }

    scrollToBottom();
}

void PostWindow::recolorStderr(const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);

    for (const TextRange &range : stderrRanges) {
        QTextCursor cursor(textEdit->document());
        cursor.setPosition(range.start);
        cursor.setPosition(range.end, QTextCursor::KeepAnchor);
        cursor.mergeCharFormat(format);
    }
}

void PostWindow::resetProcessOutputState()
{
    stdoutDecoder.resetState();
    stderrDecoder.resetState();
    stderrRanges.clear();
}

void PostWindow::handleStdout()
{
    const QString output = stdoutDecoder(proc->readAllStandardOutput());
    appendProcessText(output, false);
}

void PostWindow::handleStderr()
{
    const QString output = stderrDecoder(proc->readAllStandardError());
    appendProcessText(output, true);
}

void PostWindow::increaseFont()
{
    QFont f = textEdit->font();
    f.setPointSize(f.pointSize() + 1);
    textEdit->setFont(f);
}

void PostWindow::decreaseFont()
{
    QFont f = textEdit->font();
    f.setPointSize(std::max(1, f.pointSize() - 1));
    textEdit->setFont(f);
}

void PostWindow::clearOutput()
{
    textEdit->clear();
    stderrRanges.clear();
}

void PostWindow::termProcess()
{
    stopRequested = true;
    proc->terminate();
    appendColored("*** User requested process terminate ***", Qt::red);
}

void PostWindow::killProcess()
{
    stopRequested = true;
    proc->kill();
    appendColored("*** Process killed by user ***", Qt::red);
}

void PostWindow::runProcess()
{
    proc->start(proc->program(), proc->arguments());
}

void PostWindow::scrollToBottom()
{
    if (autoscroll) {
        textEdit->moveCursor(QTextCursor::End);
        textEdit->ensureCursorVisible();
    }
}
