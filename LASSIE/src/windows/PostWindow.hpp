#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QAction>
#include <QProcess>
#include <QStringDecoder>
#include <QVector>

class PostWindow : public QWidget
{
    Q_OBJECT
public:
    explicit PostWindow(QProcess *process, QWidget *parent = nullptr);

private slots:
    void handleStdout();
    void handleStderr();
    void increaseFont();
    void decreaseFont();
    void clearOutput();
    void termProcess();
    void killProcess();
    void runProcess();

private:
    struct TextRange {
        int start;
        int end;
    };

    QTextEdit *textEdit;
    QProcess *proc;
    bool autoscroll = true;
    bool stopRequested = false;
    QStringDecoder stdoutDecoder{QStringDecoder::Utf8};
    QStringDecoder stderrDecoder{QStringDecoder::Utf8};
    QVector<TextRange> stderrRanges;

    void closeEvent(QCloseEvent*);
    void appendColored(const QString &text, const QColor &color);
    void appendProcessText(const QString &text, bool fromStderr);
    void recolorStderr(const QColor &color);
    void resetProcessOutputState();
    void scrollToBottom();
};
