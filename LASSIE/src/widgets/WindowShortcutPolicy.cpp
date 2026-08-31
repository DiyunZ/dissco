#include "WindowShortcutPolicy.hpp"
#include "../windows/MainWindow.hpp"

#include <QApplication>
#include <QEvent>
#include <QKeySequence>
#include <QShortcut>
#include <QVariant>
#include <QWidget>

namespace {

constexpr auto kRegisteredProperty = "_lassie_windowShortcutPolicy";

} // namespace

WindowShortcutPolicy::WindowShortcutPolicy(QApplication& application)
    : QObject(&application)
{
    application.installEventFilter(this);
}

bool WindowShortcutPolicy::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::Show)
        return false;

    auto* window = qobject_cast<QWidget*>(watched);
    if (!window || !window->isWindow()
        || qobject_cast<MainWindow*>(window)
        || window->property(kRegisteredProperty).toBool()) {
        return false;
    }

    const Qt::WindowType type = window->windowType();
    // Limit this policy to LASSIE's editor windows/dialogs. Tool windows can
    // share their parent's shortcut scope; popups and tooltips are not editors.
    if (type != Qt::Window && type != Qt::Dialog) {
        return false;
    }

    window->setProperty(kRegisteredProperty, true);
    auto* closeShortcut = new QShortcut(window);
    closeShortcut->setKeys(QKeySequence::keyBindings(QKeySequence::Close));
    closeShortcut->setContext(Qt::WindowShortcut);
    connect(closeShortcut, &QShortcut::activated, window, &QWidget::close);
    return false;
}
