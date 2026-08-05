#ifndef TEXT_OVERFLOW_DISPLAY_POLICY_HPP
#define TEXT_OVERFLOW_DISPLAY_POLICY_HPP

#include <QObject>

class QApplication;
class QEvent;
class QLineEdit;

// Keeps single-line editors scrolled to the start when a value is displayed
// rather than actively edited. User typing, pasting, selection, and cursor
// navigation retain Qt's normal behaviour until editing is finished.
class TextOverflowDisplayPolicy final : public QObject
{
public:
    explicit TextOverflowDisplayPolicy(QApplication& application);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    Q_DISABLE_COPY_MOVE(TextOverflowDisplayPolicy)

    static void registerLineEdit(QLineEdit* edit);
    static void markInteractiveOperation(QLineEdit* edit);
    static void scheduleShowBeginning(QLineEdit* edit,
                                      bool allowFocusedProgrammaticChange = false,
                                      bool finishSelection = false);
    static void scheduleShowBeginningAfterEditing(QLineEdit* edit);
    static void showBeginning(QLineEdit* edit,
                              bool finishSelection = false);
    static bool textOverflows(const QLineEdit* edit);
};

#endif
