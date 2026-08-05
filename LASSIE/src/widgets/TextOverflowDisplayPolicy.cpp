#include "TextOverflowDisplayPolicy.hpp"

#include <QApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyleOptionFrame>
#include <QTextLayout>
#include <QTimer>
#include <QVariant>
#include <QWidget>

namespace {

constexpr auto kRegisteredProperty = "_lassie_textOverflowDisplayPolicy";
constexpr auto kInteractiveEditProperty = "_lassie_interactiveTextEdit";
constexpr auto kInteractiveGenerationProperty =
    "_lassie_interactiveTextEditGeneration";
constexpr int kLineEditHorizontalMargin = 2;

bool isInteractiveInput(const QEvent* event)
{
    const QEvent::Type type = event->type();
    if (type == QEvent::MouseMove) {
        return static_cast<const QMouseEvent*>(event)->buttons()
            != Qt::NoButton;
    }

    return type == QEvent::KeyPress
        || type == QEvent::MouseButtonPress
        || type == QEvent::MouseButtonDblClick
        || type == QEvent::InputMethod
        || type == QEvent::ContextMenu
        || type == QEvent::Drop;
}

QStyleOptionFrame styleOptionFor(const QLineEdit* edit)
{
    QStyleOptionFrame option;
    option.initFrom(edit);
    option.rect = edit->contentsRect();
    option.lineWidth = edit->hasFrame()
        ? edit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth,
                                     &option, edit)
        : 0;
    option.midLineWidth = 0;
    option.state |= QStyle::State_Sunken;
    if (edit->isReadOnly())
        option.state |= QStyle::State_ReadOnly;
    option.features = QStyleOptionFrame::None;
    return option;
}

QMargins effectiveTextMargins(const QLineEdit* edit)
{
    QMargins margins = edit->textMargins();
    const QRect editRect = edit->rect();
    int leftSideWidgetWidth = 0;
    int rightSideWidgetWidth = 0;

    const auto childWidgets = edit->findChildren<QWidget*>(
        QString(), Qt::FindDirectChildrenOnly);
    for (const QWidget* child : childWidgets) {
        if (!child->isVisibleTo(edit))
            continue;

        const QRect geometry = child->geometry().intersected(editRect);
        if (geometry.isEmpty())
            continue;

        const int distanceFromLeft = geometry.left() - editRect.left();
        const int distanceFromRight = editRect.right() - geometry.right();
        if (distanceFromLeft <= distanceFromRight) {
            leftSideWidgetWidth = qMax(
                leftSideWidgetWidth,
                geometry.right() - editRect.left() + 1);
        } else {
            rightSideWidgetWidth = qMax(
                rightSideWidgetWidth,
                editRect.right() - geometry.left() + 1);
        }
    }

    margins.setLeft(margins.left() + leftSideWidgetWidth);
    margins.setRight(margins.right() + rightSideWidgetWidth);
    return margins;
}

} // namespace

TextOverflowDisplayPolicy::TextOverflowDisplayPolicy(QApplication& application)
    : QObject(&application)
{
    application.installEventFilter(this);
}

bool TextOverflowDisplayPolicy::eventFilter(QObject* watched, QEvent* event)
{
    const QEvent::Type type = event->type();
    const bool displayMayNeedReset = type == QEvent::Polish
        || type == QEvent::FocusOut || type == QEvent::Show
        || type == QEvent::Resize;
    const bool interactiveInput = isInteractiveInput(event);
    if (!displayMayNeedReset && !interactiveInput) {
        return false;
    }

    auto* edit = qobject_cast<QLineEdit*>(watched);
    if (!edit)
        return false;

    if (interactiveInput) {
        markInteractiveOperation(edit);
        return false;
    }

    if (type == QEvent::Polish) {
        registerLineEdit(edit);
        if (!edit->hasFocus())
            showBeginning(edit);
        return false;
    }

    if (type == QEvent::Show || type == QEvent::Resize) {
        if (!edit->hasFocus())
            scheduleShowBeginning(edit);
        return false;
    }

    const auto reason = static_cast<QFocusEvent*>(event)->reason();
    if (reason != Qt::PopupFocusReason
        && reason != Qt::ActiveWindowFocusReason) {
        scheduleShowBeginning(edit, false, true);
    }

    return false;
}

void TextOverflowDisplayPolicy::registerLineEdit(QLineEdit* edit)
{
    if (edit->property(kRegisteredProperty).toBool())
        return;

    edit->setProperty(kRegisteredProperty, true);
    connect(edit, &QLineEdit::textEdited, edit, [edit]() {
        markInteractiveOperation(edit);
    });
    connect(edit, &QLineEdit::textChanged, edit, [edit]() {
        scheduleShowBeginning(edit, true);
    });
    connect(edit, &QLineEdit::cursorPositionChanged, edit, [edit]() {
        // setText(existingText) still moves the cursor to the end but emits no
        // textChanged signal. User input is marked by the event filter, so the
        // focused case can be handled without disrupting cursor navigation.
        scheduleShowBeginning(edit, true);
    });
    connect(edit, &QLineEdit::returnPressed, edit, [edit]() {
        scheduleShowBeginningAfterEditing(edit);
    });
}

void TextOverflowDisplayPolicy::markInteractiveOperation(QLineEdit* edit)
{
    const qulonglong generation =
        edit->property(kInteractiveGenerationProperty).toULongLong() + 1;
    edit->setProperty(kInteractiveGenerationProperty, generation);
    edit->setProperty(kInteractiveEditProperty, true);
    // Keep the marker through every queued callback created by this input,
    // regardless of the order of textEdited and textChanged. Clear it on the
    // following event-loop turn.
    QTimer::singleShot(0, edit, [edit, generation]() {
        QTimer::singleShot(0, edit, [edit, generation]() {
            if (edit->property(kInteractiveGenerationProperty).toULongLong()
                == generation) {
                edit->setProperty(kInteractiveEditProperty, false);
            }
        });
    });
}

void TextOverflowDisplayPolicy::scheduleShowBeginning(
    QLineEdit* edit,
    bool allowFocusedProgrammaticChange,
    bool finishSelection)
{
    QTimer::singleShot(0, edit, [edit, allowFocusedProgrammaticChange,
                                 finishSelection]() {
        const bool isInteractiveEdit =
            edit->property(kInteractiveEditProperty).toBool();
        if (!edit->hasFocus()
            || (allowFocusedProgrammaticChange && !isInteractiveEdit)) {
            showBeginning(edit, finishSelection);
        }
    });
}

void TextOverflowDisplayPolicy::scheduleShowBeginningAfterEditing(
    QLineEdit* edit)
{
    const qulonglong generation =
        edit->property(kInteractiveGenerationProperty).toULongLong();
    // returnPressed is emitted inside the key event. Wait until the
    // interactive-operation marker has completed before restoring the view.
    QTimer::singleShot(0, edit, [edit, generation]() {
        QTimer::singleShot(0, edit, [edit, generation]() {
            // A new key, mouse, input-method, or drop event means the user has
            // already started another interaction; an older Return must not
            // move that interaction's cursor.
            if (edit->property(kInteractiveGenerationProperty).toULongLong()
                == generation) {
                showBeginning(edit, true);
            }
        });
    });
}

void TextOverflowDisplayPolicy::showBeginning(QLineEdit* edit,
                                               bool finishSelection)
{
    if (edit->cursorPosition() == 0
        || (!finishSelection && edit->hasSelectedText())
        || !textOverflows(edit)) {
        return;
    }

    const QSignalBlocker blocker(edit);
    edit->setCursorPosition(0);
}

bool TextOverflowDisplayPolicy::textOverflows(const QLineEdit* edit)
{
    QStyleOptionFrame option = styleOptionFor(edit);
    QRect textRect = edit->style()->subElementRect(
        QStyle::SE_LineEditContents, &option, edit);
    textRect = textRect.marginsRemoved(effectiveTextMargins(edit));
    const int availableWidth = textRect.width()
        - 2 * kLineEditHorizontalMargin;

    QTextLayout layout(edit->displayText(), edit->font());
    layout.beginLayout();
    const QTextLine line = layout.createLine();
    layout.endLayout();
    const int usedWidth = line.isValid()
        ? qRound(line.naturalTextWidth()) + 1
        : 0;

    return usedWidth > availableWidth;
}
