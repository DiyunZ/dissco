#ifndef MODIFIERDETAILSDIALOG_HPP
#define MODIFIERDETAILSDIALOG_HPP

#include <QDialog>
#include <QVector>

#include "../core/event_struct.hpp"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

/**
 * Edits the synthesis parameters of one Modifier as an atomic draft.
 *
 * The compact Modifier row owns activation probability and conditional rules.
 * This dialog intentionally owns only effect parameters and legacy fields.
 */
class ModifierDetailsDialog : public QDialog
{
public:
    explicit ModifierDetailsDialog(const Modifier& modifier,
                                   bool modifierUsageEnabled,
                                   bool showLegacyFields,
                                   QWidget* parent = nullptr);

    Modifier resultModifier() const { return m_modifier; }

protected:
    void accept() override;

private:
    enum Field {
        Probability = 0,
        Magnitude,
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
    int maximumSpectrumPartialCount() const;

    Modifier m_modifier;
    QComboBox* m_applyCombo = nullptr;
    QLineEdit* m_groupNameEdit = nullptr;
    QVector<FieldWidgets> m_fields;
};

#endif // MODIFIERDETAILSDIALOG_HPP
