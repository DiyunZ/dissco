#ifndef MODIFIERDETAILSDIALOG_HPP
#define MODIFIERDETAILSDIALOG_HPP

#include <QDialog>
#include <QVector>

#include "../core/event_struct.hpp"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace ModifierUiPolicy {
struct PartialRowConstraint;
}

/**
 * Edits the synthesis parameters of one Modifier as an atomic draft.
 *
 * The compact Modifier row owns activation probability and conditional rules.
 * This dialog intentionally owns only effect parameters.
 */
class ModifierDetailsDialog : public QDialog
{
public:
    explicit ModifierDetailsDialog(const Modifier& modifier,
                                   Eventtype eventType,
                                   unsigned eventIndex,
                                   QWidget* parent = nullptr);

    Modifier resultModifier() const { return m_modifier; }

protected:
    void accept() override;

private:
    enum Field {
        Magnitude = 0,
        Rate,
        Width,
        Spread,
        Direction,
        Velocity,
        PartialResult
    };

    struct FieldWidgets {
        Field field;
        QLabel* label = nullptr;
        QLineEdit* edit = nullptr;
        QPushButton* functionButton = nullptr;
    };

    void addFieldRow(class QGridLayout* layout, int row, Field field,
                     const QString& labelText);
    void updateVisibleFields();
    void editField(Field field);
    QString valueFor(Field field) const;
    void setValue(Field field, const QString& value);
    ModifierUiPolicy::PartialRowConstraint partialRowConstraint() const;

    Modifier m_modifier;
    Eventtype m_eventType = bottom;
    unsigned m_eventIndex = 0;
    QComboBox* m_applyCombo = nullptr;
    QVector<FieldWidgets> m_fields;
};

#endif // MODIFIERDETAILSDIALOG_HPP
