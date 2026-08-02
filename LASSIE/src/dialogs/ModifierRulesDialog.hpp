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
 * Each rule mentions only the earlier states that matter. Equal-specificity
 * overlapping contexts are rejected so declaration order stays irrelevant.
 */
class ModifierRulesDialog : public QDialog
{
public:
    ModifierRulesDialog(const Modifier& target,
                        const QList<Modifier>& earlierModifiers,
                        QWidget* parent = nullptr);

    QList<ModifierChanceRule> resultRules() const { return m_rules; }

protected:
    void accept() override;

private:
    void rebuildTable();
    void addRule();
    void editSelectedRule();
    void removeSelectedRule();
    QString conditionSummary(const ModifierChanceRule& rule) const;
    QString contextKey(const ModifierChanceRule& rule) const;
    bool hasDuplicateContext(const ModifierChanceRule& candidate,
                             int ignoredIndex = -1) const;
    bool hasAmbiguousContext(const ModifierChanceRule& candidate,
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
