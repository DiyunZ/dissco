#include "ModifierRulesDialog.hpp"

#include "../widgets/ModifierUiPolicy.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QString modifierLabel(const Modifier& modifier, int oneBasedPosition)
{
    return QObject::tr("%1 (%2)")
        .arg(ModifierUiPolicy::displayName(static_cast<int>(modifier.type)))
        .arg(oneBasedPosition);
}

QString normalizedChance(double percent)
{
    return QString::number(percent / 100.0, 'g', 15);
}

class RuleEditDialog : public QDialog
{
public:
    RuleEditDialog(const QList<Modifier>& earlierModifiers,
                   const ModifierChanceRule* existing,
                   QWidget* parent)
        : QDialog(parent),
          m_earlierModifiers(earlierModifiers)
    {
        setWindowTitle(existing
            ? tr("Edit conditional exception")
            : tr("Add conditional exception"));
        setModal(true);

        auto* root = new QVBoxLayout(this);
        auto* explanation = new QLabel(
            tr("Choose only the earlier states that matter; Any ignores that "
               "modifier. When the selected context occurs, the target uses the "
               "ON chance below instead of its default."),
            this);
        explanation->setWordWrap(true);
        root->addWidget(explanation);

        auto* form = new QFormLayout;
        for (int index = 0; index < earlierModifiers.size(); ++index) {
            const Modifier& modifier = earlierModifiers[index];
            auto* state = new QComboBox(this);
            state->addItem(tr("Any"));
            state->addItem(tr("ON"), true);
            state->addItem(tr("OFF"), false);

            if (existing) {
                for (const ModifierCondition& condition : existing->conditions) {
                    if (condition.modifier_id == modifier.instance_id) {
                        state->setCurrentIndex(condition.required_on ? 1 : 2);
                        break;
                    }
                }
            }

            form->addRow(modifierLabel(modifier, index + 1) + QStringLiteral(":"),
                         state);
            m_stateCombos.append(state);
        }

        m_chanceSpin = new QDoubleSpinBox(this);
        m_chanceSpin->setRange(0.0, 100.0);
        m_chanceSpin->setDecimals(1);
        m_chanceSpin->setSingleStep(1.0);
        m_chanceSpin->setSuffix(QStringLiteral("%"));
        m_chanceSpin->setValue(existing
            ? existing->on_chance.toDouble() * 100.0
            : 50.0);
        form->addRow(tr("Use ON chance:"), m_chanceSpin);
        root->addLayout(form);

        auto* buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted,
                this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected,
                this, &QDialog::reject);
        root->addWidget(buttons);
    }

    ModifierChanceRule resultRule() const
    {
        ModifierChanceRule rule;
        rule.on_chance = normalizedChance(m_chanceSpin->value());
        for (int index = 0; index < m_earlierModifiers.size(); ++index) {
            if (m_stateCombos[index]->currentIndex() == 0)
                continue;
            ModifierCondition condition;
            condition.modifier_id =
                m_earlierModifiers[index].instance_id;
            condition.required_on =
                m_stateCombos[index]->currentData().toBool();
            rule.conditions.append(condition);
        }
        return rule;
    }

protected:
    void accept() override
    {
        for (QComboBox* state : m_stateCombos) {
            if (state->currentIndex() != 0) {
                QDialog::accept();
                return;
            }
        }
        QMessageBox::warning(
            this, tr("Condition required"),
            tr("Choose ON or OFF for at least one earlier modifier."));
    }

private:
    QList<Modifier> m_earlierModifiers;
    QList<QComboBox*> m_stateCombos;
    QDoubleSpinBox* m_chanceSpin = nullptr;
};

} // namespace

ModifierRulesDialog::ModifierRulesDialog(
    const Modifier& target,
    const QList<Modifier>& earlierModifiers,
    QWidget* parent)
    : QDialog(parent),
      m_earlierModifiers(earlierModifiers),
      m_rules(target.rules)
{
    setWindowTitle(tr("%1 conditional exceptions")
        .arg(ModifierUiPolicy::displayName(static_cast<int>(target.type))));
    setModal(true);
    resize(680, 360);

    auto* root = new QVBoxLayout(this);
    m_explanation = new QLabel(this);
    m_explanation->setWordWrap(true);
    root->addWidget(m_explanation);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels(
        {tr("Earlier modifier context"), tr("Use ON chance")});
    m_table->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(m_table);

    auto* actionRow = new QHBoxLayout;
    m_addButton = new QPushButton(tr("+ Add exception"), this);
    m_editButton = new QPushButton(tr("Edit"), this);
    m_removeButton = new QPushButton(tr("Remove"), this);
    actionRow->addWidget(m_addButton);
    actionRow->addWidget(m_editButton);
    actionRow->addWidget(m_removeButton);
    actionRow->addStretch();
    root->addLayout(actionRow);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    root->addWidget(buttons);

    connect(m_addButton, &QPushButton::clicked,
            this, [this]() { addRule(); });
    connect(m_editButton, &QPushButton::clicked,
            this, [this]() { editSelectedRule(); });
    connect(m_removeButton, &QPushButton::clicked,
            this, [this]() { removeSelectedRule(); });
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, [this](int, int) { editSelectedRule(); });
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool selected = selectedRuleIndex() >= 0;
        m_editButton->setEnabled(selected);
        m_removeButton->setEnabled(selected);
    });

    if (m_earlierModifiers.isEmpty()) {
        m_explanation->setText(
            tr("This modifier is evaluated first, so it has no earlier "
               "modifier context and always uses its Default ON chance."));
        m_addButton->setEnabled(false);
    } else {
        m_explanation->setText(
            tr("Default ON chance is the fallback. Add only contexts that "
               "should use a different chance."));
    }

    rebuildTable();
}

void ModifierRulesDialog::accept()
{
    for (int index = 0; index < m_rules.size(); ++index) {
        if (m_rules[index].conditions.isEmpty()) {
            QMessageBox::warning(
                this, tr("Condition required"),
                tr("Every exception must depend on at least one earlier "
                   "modifier."));
            return;
        }
        if (hasDuplicateContext(m_rules[index], index)
            || hasAmbiguousContext(m_rules[index], index)) {
            QMessageBox::warning(
                this, tr("Overlapping exceptions"),
                tr("Two exceptions can match the same earlier state with "
                   "equal specificity. Edit or remove one before saving."));
            return;
        }
    }
    QDialog::accept();
}

void ModifierRulesDialog::rebuildTable()
{
    m_table->setRowCount(m_rules.size());
    for (int row = 0; row < m_rules.size(); ++row) {
        m_table->setItem(
            row, 0, new QTableWidgetItem(conditionSummary(m_rules[row])));
        bool valid = false;
        const double chance = m_rules[row].on_chance.toDouble(&valid);
        const QString chanceText = valid
            ? QStringLiteral("%1%").arg(chance * 100.0, 0, 'f', 1)
            : tr("Invalid");
        m_table->setItem(row, 1, new QTableWidgetItem(chanceText));
    }

    const bool selected = selectedRuleIndex() >= 0;
    m_editButton->setEnabled(selected);
    m_removeButton->setEnabled(selected);
}

void ModifierRulesDialog::addRule()
{
    RuleEditDialog dialog(m_earlierModifiers, nullptr, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const ModifierChanceRule rule = dialog.resultRule();
    if (hasDuplicateContext(rule)) {
        QMessageBox::warning(
            this, tr("Duplicate context"),
            tr("An exception already exists for this earlier modifier context."));
        return;
    }
    if (hasAmbiguousContext(rule)) {
        QMessageBox::warning(
            this, tr("Overlapping context"),
            tr("This exception can match at the same time as another "
               "exception with the same number of conditions. Add another "
               "ON/OFF condition so only one of them can match."));
        return;
    }
    m_rules.append(rule);
    rebuildTable();
    m_table->selectRow(m_rules.size() - 1);
}

void ModifierRulesDialog::editSelectedRule()
{
    const int row = selectedRuleIndex();
    if (row < 0)
        return;

    RuleEditDialog dialog(m_earlierModifiers, &m_rules[row], this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const ModifierChanceRule rule = dialog.resultRule();
    if (hasDuplicateContext(rule, row)) {
        QMessageBox::warning(
            this, tr("Duplicate context"),
            tr("An exception already exists for this earlier modifier context."));
        return;
    }
    if (hasAmbiguousContext(rule, row)) {
        QMessageBox::warning(
            this, tr("Overlapping context"),
            tr("This exception can match at the same time as another "
               "exception with the same number of conditions. Add another "
               "ON/OFF condition so only one of them can match."));
        return;
    }
    m_rules[row] = rule;
    rebuildTable();
    m_table->selectRow(row);
}

void ModifierRulesDialog::removeSelectedRule()
{
    const int row = selectedRuleIndex();
    if (row < 0)
        return;
    m_rules.removeAt(row);
    rebuildTable();
}

QString ModifierRulesDialog::conditionSummary(
    const ModifierChanceRule& rule) const
{
    QStringList parts;
    for (const ModifierCondition& condition : rule.conditions) {
        QString label = condition.modifier_id;
        for (int index = 0; index < m_earlierModifiers.size(); ++index) {
            if (m_earlierModifiers[index].instance_id
                == condition.modifier_id) {
                label = modifierLabel(m_earlierModifiers[index], index + 1);
                break;
            }
        }
        parts.append(tr("%1 is %2")
            .arg(label, condition.required_on ? tr("ON") : tr("OFF")));
    }
    return parts.isEmpty() ? tr("Invalid or empty context")
                           : parts.join(QStringLiteral(" / "));
}

QString ModifierRulesDialog::contextKey(
    const ModifierChanceRule& rule) const
{
    QStringList parts;
    for (const Modifier& modifier : m_earlierModifiers) {
        QString state = QStringLiteral("?");
        for (const ModifierCondition& condition : rule.conditions) {
            if (condition.modifier_id == modifier.instance_id) {
                state = condition.required_on
                    ? QStringLiteral("1") : QStringLiteral("0");
                break;
            }
        }
        parts.append(modifier.instance_id + QStringLiteral("=") + state);
    }
    return parts.join(QStringLiteral("|"));
}

bool ModifierRulesDialog::hasDuplicateContext(
    const ModifierChanceRule& candidate,
    int ignoredIndex) const
{
    const QString key = contextKey(candidate);
    for (int index = 0; index < m_rules.size(); ++index) {
        if (index != ignoredIndex && contextKey(m_rules[index]) == key)
            return true;
    }
    return false;
}

bool ModifierRulesDialog::hasAmbiguousContext(
    const ModifierChanceRule& candidate,
    int ignoredIndex) const
{
    for (int index = 0; index < m_rules.size(); ++index) {
        if (index == ignoredIndex
            || m_rules[index].conditions.size()
                   != candidate.conditions.size()) {
            continue;
        }

        bool canOverlap = true;
        for (const ModifierCondition& candidateCondition
             : candidate.conditions) {
            for (const ModifierCondition& existingCondition
                 : m_rules[index].conditions) {
                if (candidateCondition.modifier_id
                        == existingCondition.modifier_id
                    && candidateCondition.required_on
                        != existingCondition.required_on) {
                    canOverlap = false;
                    break;
                }
            }
            if (!canOverlap)
                break;
        }
        if (canOverlap)
            return true;
    }
    return false;
}

int ModifierRulesDialog::selectedRuleIndex() const
{
    const auto selected = m_table->selectionModel()->selectedRows();
    return selected.size() == 1 ? selected.front().row() : -1;
}
