#ifndef MODIFIERRULESDIALOG_HPP
#define MODIFIERRULESDIALOG_HPP

#include <QDialog>
#include <QList>

#include "../core/event_struct.hpp"

class QLabel;
class QPushButton;
class QTableWidget;

/**
 * Edits a target modifier's conditional exceptions as one atomic draft.
 *
 * Every rule created here describes the complete ON/OFF state of all earlier
 * modifiers. That keeps contexts disjoint and makes rule order irrelevant.
 */
class ModifierRulesDialog : public QDialog
{
public:
    ModifierRulesDialog(const Modifier& target,
                        const QList<Modifier>& earlierModifiers,
                        QWidget* parent = nullptr);

    QList<ModifierChanceRule> resultRules() const { return m_rules; }

private:
    void rebuildTable();
    void addRule();
    void editSelectedRule();
    void removeSelectedRule();
    QString conditionSummary(const ModifierChanceRule& rule) const;
    QString contextKey(const ModifierChanceRule& rule) const;
    bool hasDuplicateContext(const ModifierChanceRule& candidate,
                             int ignoredIndex = -1) const;
    int selectedRuleIndex() const;

    QList<Modifier> m_earlierModifiers;
    QList<ModifierChanceRule> m_rules;
    QTableWidget* m_table = nullptr;
    QLabel* m_explanation = nullptr;
    QPushButton* m_addButton = nullptr;
    QPushButton* m_editButton = nullptr;
    QPushButton* m_removeButton = nullptr;
};

#endif // MODIFIERRULESDIALOG_HPP
