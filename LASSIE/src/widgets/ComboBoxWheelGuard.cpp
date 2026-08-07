#include "ComboBoxWheelGuard.hpp"

#include <QAbstractScrollArea>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QWheelEvent>
#include <QWidget>

namespace {

QAbstractScrollArea* containingScrollArea(QWidget* widget)
{
    for (QWidget* parent = widget->parentWidget(); parent;
         parent = parent->parentWidget()) {
        if (auto* scrollArea = qobject_cast<QAbstractScrollArea*>(parent))
            return scrollArea;
    }

    return nullptr;
}

void forwardToScrollArea(QWheelEvent* sourceEvent,
                         QAbstractScrollArea* scrollArea)
{
    QWidget* viewport = scrollArea->viewport();
    const QPointF globalPosition = sourceEvent->globalPosition();
    const QPoint viewportPosition = viewport->mapFromGlobal(
        globalPosition.toPoint());
    QWheelEvent forwardedEvent(
        QPointF(viewportPosition),
        globalPosition,
        sourceEvent->pixelDelta(),
        sourceEvent->angleDelta(),
        sourceEvent->buttons(),
        sourceEvent->modifiers(),
        sourceEvent->phase(),
        sourceEvent->inverted(),
        sourceEvent->source(),
        sourceEvent->pointingDevice());
    QCoreApplication::sendEvent(viewport, &forwardedEvent);
}

} // namespace

ComboBoxWheelGuard::ComboBoxWheelGuard(QApplication& application)
    : QObject(&application)
{
    application.installEventFilter(this);
}

bool ComboBoxWheelGuard::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::Wheel)
        return false;

    auto* combo = qobject_cast<QComboBox*>(watched);
    if (!combo)
        return false;

    auto* wheelEvent = static_cast<QWheelEvent*>(event);
    if (QAbstractScrollArea* scrollArea = containingScrollArea(combo))
        forwardToScrollArea(wheelEvent, scrollArea);
    wheelEvent->accept();
    return true;
}
