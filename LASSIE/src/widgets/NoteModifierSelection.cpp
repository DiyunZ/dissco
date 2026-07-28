#include "NoteModifierSelection.hpp"

#include <QCheckBox>
#include <QSignalBlocker>

namespace NoteModifierSelection {

QStringList save(
    const QStringList& existingModifiers,
    const QList<QCheckBox*>& checkBoxes)
{
    QStringList visibleModifierNames;
    QStringList selectedModifiers;
    for (QCheckBox* checkBox : checkBoxes) {
        visibleModifierNames.append(checkBox->text());
        if (checkBox->isChecked())
            selectedModifiers.append(checkBox->text());
    }

    // A project may contain custom modifiers that this version of the editor
    // cannot display. Keep those values instead of deleting them on save.
    for (const QString& modifier : existingModifiers) {
        if (!visibleModifierNames.contains(modifier)
            && !selectedModifiers.contains(modifier)) {
            selectedModifiers.append(modifier);
        }
    }
    return selectedModifiers;
}

void load(
    const QStringList& modifiers,
    const QList<QCheckBox*>& checkBoxes)
{
    for (QCheckBox* checkBox : checkBoxes) {
        const QSignalBlocker blocker(checkBox);
        checkBox->setChecked(modifiers.contains(checkBox->text()));
    }
}

} // namespace NoteModifierSelection
