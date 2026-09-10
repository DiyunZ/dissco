#include "../src/inst.hpp"
#include "../src/core/EnvelopeLibraryEntry.hpp"
#include "../src/dialogs/FunctionGenerator.hpp"
#include "../src/dialogs/FunctionXmlFormat.hpp"
#include "../src/dialogs/PartialModifierDialog.hpp"
#include "../src/widgets/EnvLibDrawingArea.hpp"
#include "../src/widgets/ProjectViewController.hpp"
#include "../src/windows/EnvelopeLibraryWindow.hpp"
#include "../src/windows/MainWindow.hpp"

#include <QApplication>
#include <QDebug>
#include <QDomDocument>
#include <QImage>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTimer>
#include <QTreeView>
#include <optional>

namespace {
int failures = 0;

void check(bool passed, const char* description)
{
    if (!passed) {
        qCritical() << description;
        ++failures;
    }
}

void mouse(QWidget& widget, QEvent::Type type, QPointF position)
{
    const bool release = type == QEvent::MouseButtonRelease;
    const auto button = type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton;
    QMouseEvent event(type, position, widget.mapToGlobal(position), button,
                      release ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &event);
    QApplication::processEvents();
}

void contextAction(EnvLibDrawingArea& graph, QPointF position, const QString& text)
{
    QTimer::singleShot(0, [&graph, text] {
        auto* menu = graph.findChild<QMenu*>();
        for (auto* action : menu->actions()) {
            if (action->text() == text) { action->trigger(); break; }
        }
        menu->close();
    });
    QMouseEvent event(QEvent::MouseButtonPress, position, graph.mapToGlobal(position),
                      Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QApplication::sendEvent(&graph, &event);
    QApplication::processEvents();
}

// Find and click a rendered black node, without depending on the graph's
// private coordinate transform. Offscreen/out-of-range nodes cannot pass.
std::optional<QPointF> selectNode(EnvLibDrawingArea& graph,
                                  EnvelopeLibraryWindow& window,
                                  double x, double y)
{
    QImage image(graph.size(), QImage::Format_RGB32);
    image.fill(Qt::white);
    graph.render(&image);
    const int expectedX = qRound(x * (image.width() - 1));
    for (int py = 0; py < image.height(); ++py) {
        for (int px = qMax(0, expectedX - 60);
             px < qMin(image.width(), expectedX + 61); ++px) {
            if (image.pixelColor(px, py) != QColor(Qt::black)) continue;
            const QPointF position(px, py);
            mouse(graph, QEvent::MouseButtonPress, position);
            mouse(graph, QEvent::MouseButtonRelease, position);
            if (!window.xEntry->text().isEmpty()
                && qAbs(window.xEntry->text().toDouble() - x) < 0.0001
                && qAbs(window.yEntry->text().toDouble() - y) < 0.0001)
                return position;
        }
    }
    return std::nullopt;
}

EnvelopeLibraryEntry* triangle(int number, double first, double middle, double last)
{
    auto* env = new EnvelopeLibraryEntry(number);
    auto* left = env->head;
    auto* right = left->rightSeg->rightNode;
    auto* center = new EnvLibEntryNode(0.5, middle);
    left->y = first;
    right->y = last;
    center->leftSeg = left->rightSeg;
    center->rightSeg = new EnvLibEntrySeg;
    center->rightSeg->leftNode = center;
    center->rightSeg->rightNode = right;
    left->rightSeg->rightNode = center;
    right->leftSeg = center->rightSeg;
    return env;
}

void functionChecks()
{
    const QString xml = "<Fun>\n  <Name>Random</Name>\n  <Low>0</Low>\n"
        "  <High><Fun><Name>RandomInt</Name><Low>5</Low><High>25</High></Fun></High>\n"
        "  <Unknown>keep &amp; preserve</Unknown>\n</Fun>";
    FunctionGenerator dialog(nullptr, FunctionReturnType::functionReturnFloat, xml);
    auto* result = dialog.findChild<QTextEdit*>("resultTextEdit");
    check(result && !result->toPlainText().contains('\n'),
          "Nested Result String must open on one line");
    check(result && result->lineWrapMode() == QTextEdit::NoWrap,
          "Result String must support one horizontal line without wrapping");
    check(dialog.getResultString() == FunctionXmlFormat::compact(xml),
          "Opening/accepting a function must preserve unknown fields and nested values");
    if (!result) return;

    for (auto* entry : dialog.findChildren<QLineEdit*>()) {
        if (entry->text() == "0") { entry->setText("2"); break; }
    }
    check(!result->toPlainText().contains('\n') && result->toPlainText().contains("<Low>2</Low>"),
          "Changing function parameters must keep the result compact");
    const QString spaced = "<Fun><Name>Unknown</Name><Value> a&#10;b&#9;c&#13;d&#160; </Value></Fun>";
    result->setPlainText(spaced);
    QDomDocument before, after;
    check(bool(before.setContent(spaced)) && bool(after.setContent(dialog.getResultString()))
          && before.documentElement().text() == after.documentElement().text(),
          "Single-line formatting must preserve whitespace inside parameter values");
    result->setPlainText("<Fun>unfinished");
    check(dialog.getResultString() == "<Fun>unfinished",
          "Incomplete manual edits must remain available to correct");

    const QString envelope = "<Fun><Name>EnvLib</Name><Env>1</Env><Scale>1</Scale></Fun>";
    const QString partialXml = "<Fun><Name>Partials</Name><Envelopes><Envelope>"
        + envelope + "</Envelope><Envelope>" + envelope
        + "</Envelope><Envelope>N/A</Envelope><Envelope>" + envelope
        + "</Envelope></Envelopes></Fun>";
    PartialModifierDialog partials(nullptr, 0, {1, 1, {}}, partialXml);
    auto* partialPreview = partials.findChild<QPlainTextEdit*>();
    check(partialPreview && !partialPreview->toPlainText().contains('\n')
          && partialPreview->lineWrapMode() == QPlainTextEdit::NoWrap,
          "Generated Partial Result String must also display on one line");
    check(bool(before.setContent(partialXml)) && bool(after.setContent(partials.resultString()))
          && before.documentElement().text() == after.documentElement().text(),
          "Changing partial result display must preserve saved parameter values");
}

void projectChecks(const QString& which, const QString& folder)
{
    auto* pm = Inst::get_project_manager();
    pm->build(folder + "/tutorial_check.dissco");
    MainWindow mainWindow(Inst::instance());
    ProjectView view(&mainWindow, pm->fileinfo().absoluteFilePath());
    if (which == "Defaults") {
        check(pm->samplesize() == "24", "New projects must default to 24-bit samples");
        QTimer::singleShot(0, [] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            auto* size = dialog ? dialog->findChild<QLineEdit*>("sizeEntry") : nullptr;
            check(size && size->text() == "24", "Project Properties must show 24 bits by default");
            if (dialog) dialog->accept();
        });
        view.setProperties();
        check(view.save(), "Default project must save");
        QString error;
        check(pm->open(pm->fileinfo().absoluteFilePath(), {}, &error) != nullptr,
              "Default project must reopen");
        check(pm->samplesize() == "24", "24-bit sample size must survive save/reopen");
        pm->samplesize() = "16";
        check(view.save(), "An explicitly chosen 16-bit setting must save");
        check(pm->open(pm->fileinfo().absoluteFilePath(), {}, &error) != nullptr,
              "Existing project must reopen");
        check(pm->samplesize() == "16", "Opening existing files must preserve their sample size");
        return;
    }

    auto* first = triangle(1, 2.5, 1.0, 0.5);
    first->next = triangle(2, 0.0, 2.5, 0.0);
    first->next->prev = first;
    pm->envlibentries() = first;
    check(view.save(), "High-Y envelopes must save");
    QString error;
    if (!pm->open(pm->fileinfo().absoluteFilePath(), {}, &error)) {
        check(false, "High-Y envelope project must reopen");
        return;
    }
    EnvelopeLibraryWindow window;
    window.setActiveProject(&view);
    window.resize(900, 700);
    window.show();
    auto* tree = window.findChild<QTreeView*>();
    auto* graph = window.findChild<EnvLibDrawingArea*>();
    auto selectEnvelope = [&](int row) {
        tree->selectionModel()->setCurrentIndex(tree->model()->index(row, 0),
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        QApplication::processEvents();
    };
    selectEnvelope(0);
    auto* env = window.getActiveEnvelope();
    check(env && env->head->y == 2.5, "First node above Y=1 must survive reopen");
    auto firstPosition = selectNode(*graph, window, 0, 2.5);
    check(firstPosition.has_value(), "Reopened first node above Y=1 must be visible and selectable");
    if (firstPosition) {
        window.yEntry->setText("3");
        check(env->head->y == 3.0, "Reopened high first node must accept numeric edits");
    }
    auto middlePosition = selectNode(*graph, window, 0.5, 1.0);
    check(middlePosition.has_value(), "Y=1 node must be selectable");
    if (middlePosition) {
        mouse(*graph, QEvent::MouseButtonPress, *middlePosition);
        mouse(*graph, QEvent::MouseMove, *middlePosition + QPointF(0, 2));
        mouse(*graph, QEvent::MouseButtonRelease, *middlePosition + QPointF(0, 2));
        check(env->head->rightSeg->rightNode->y == 1.0,
              "Dragging near Y=1 must snap exactly to Y=1");
        window.yEntry->setText("1");
        check(env->head->rightSeg->rightNode->y == 1.0,
              "Typing Y=1 must place the node exactly at 1");
    }

    selectEnvelope(1);
    auto* peak = window.getActiveEnvelope()->head->rightSeg->rightNode;
    auto peakPosition = selectNode(*graph, window, 0.5, 2.5);
    check(peakPosition.has_value(), "Reopened peak above Y=1 must be selectable");
    if (peakPosition) {
        mouse(*graph, QEvent::MouseButtonPress, *peakPosition);
        const QPointF target = *peakPosition + QPointF(0, 25);
        mouse(*graph, QEvent::MouseMove, target);
        const double firstY = peak->y;
        mouse(*graph, QEvent::MouseMove, target);
        check(peak->y == firstY, "A stationary pointer must not move a peak as the graph repaints");
        mouse(*graph, QEvent::MouseButtonRelease, target);
        window.yEntry->setText("2.75");
        check(peak->y == 2.75, "Reopened peak must accept an exact Y above 1");
    }
    check(view.save(), "Edited envelopes must save");
    check(pm->open(pm->fileinfo().absoluteFilePath(), {}, &error) != nullptr,
          "Edited envelopes must reopen");
    check(pm->envlibentries()->head->rightSeg->rightNode->y == 1.0
          && pm->envlibentries()->next->head->rightSeg->rightNode->y == 2.75,
          "Exact Y=1 and edited high peak must survive save/reopen");

    window.setActiveProject(&view);
    selectEnvelope(0);
    env = window.getActiveEnvelope();
    firstPosition = selectNode(*graph, window, 0, env->head->y);
    middlePosition = selectNode(*graph, window, 0.5, 1.0);
    if (firstPosition && middlePosition) {
        const QPointF insertPosition = (*firstPosition + *middlePosition) / 2;
        const double expectedY = (env->head->y + 1.0) / 2;
        contextAction(*graph, insertPosition, "Insert Node");
        auto* inserted = env->head->rightSeg->rightNode;
        check(env->head->countNumOfNodes() == 4 && qAbs(inserted->x - 0.25) < 0.02
              && qAbs(inserted->y - expectedY) < 0.08,
              "Insert Node must use the clicked X/Y, without inverting Y");
        const QPointF firstSegment = (*firstPosition + insertPosition) / 2;
        contextAction(*graph, firstSegment, "Set Exponential");
        check(env->head->rightSeg->segmentType == envSegmentTypeExponential,
              "Segment actions must target the segment under the pointer");
        contextAction(*graph, firstSegment, "Set Fixed");
        check(env->head->rightSeg->segmentProperty == envSegmentPropertyFixed,
              "Fixed segment action must target the segment under the pointer");
        contextAction(*graph, firstSegment, "Set Spline");
        check(env->head->rightSeg->segmentType == envSegmentTypeSpline,
              "Spline segment action must use the same coordinates");
        contextAction(*graph, firstSegment, "Set Flexible");
        contextAction(*graph, firstSegment, "Set Linear");
        check(env->head->rightSeg->segmentProperty == envSegmentPropertyFlexible
              && env->head->rightSeg->segmentType == envSegmentTypeLinear,
              "Linear and Flexible segment actions must use the same coordinates");
        const int count = env->head->countNumOfNodes();
        contextAction(*graph, QPointF(0, 0), "Insert Node");
        check(env->head->countNumOfNodes() == count,
              "A click outside the plot must not duplicate an endpoint");
    }
}
} // namespace

int runSoundTutorialChecks(const QString& which)
{
    QTemporaryDir folder;
    if (!folder.isValid()) return 1;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, folder.path());
    if (which == "Functions") functionChecks();
    else projectChecks(which, folder.path());
    return failures ? 1 : 0;
}
