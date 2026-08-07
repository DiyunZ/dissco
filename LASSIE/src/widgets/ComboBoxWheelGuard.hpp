#ifndef COMBO_BOX_WHEEL_GUARD_HPP
#define COMBO_BOX_WHEEL_GUARD_HPP

#include <QObject>

class QApplication;
class QEvent;

// Prevents a closed combo box from changing selection when the user scrolls
// past it. If the combo is inside a scrolling view, the wheel input continues
// to that view so scrolling the surrounding page is uninterrupted.
class ComboBoxWheelGuard final : public QObject
{
public:
    explicit ComboBoxWheelGuard(QApplication& application);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    Q_DISABLE_COPY_MOVE(ComboBoxWheelGuard)
};

#endif
