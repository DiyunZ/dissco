#ifndef NOTEMODIFIERSELECTION_HPP
#define NOTEMODIFIERSELECTION_HPP

#include <QList>
#include <QStringList>

class QCheckBox;

namespace NoteModifierSelection {

QStringList save(
    const QStringList& existingModifiers,
    const QList<QCheckBox*>& checkBoxes);

void load(
    const QStringList& modifiers,
    const QList<QCheckBox*>& checkBoxes);

} // namespace NoteModifierSelection

#endif // NOTEMODIFIERSELECTION_HPP
