#ifndef WINDOW_SHORTCUT_POLICY_HPP
#define WINDOW_SHORTCUT_POLICY_HPP

#include <QObject>

class QApplication;
class QEvent;

// Gives normal secondary windows and dialogs the platform's standard Close
// shortcuts. Closing follows the normal window close event, so dialogs reject
// pending edits and windows retain their existing close confirmations.
class WindowShortcutPolicy final : public QObject
{
public:
    explicit WindowShortcutPolicy(QApplication& application);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    Q_DISABLE_COPY_MOVE(WindowShortcutPolicy)
};

#endif
