#include "ModifierDetailsDialog.hpp"

#include "FunctionGenerator.hpp"
#include "PartialModifierDialog.hpp"
#include "../inst.hpp"
#include "../widgets/ModifierUiPolicy.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

using enum FunctionReturnType;

ModifierDetailsDialog::ModifierDetailsDialog(const Modifier& modifier,
                                             bool modifierUsageEnabled,
                                             bool showLegacyFields,
                                             QWidget* parent)
    : QDialog(parent),
      m_modifier(modifier)
{
    setWindowTitle(tr("%1 Parameters")
        .arg(ModifierUiPolicy::displayName(static_cast<int>(m_modifier.type))));
    setModal(true);
    resize(680, 420);

    auto* root = new QVBoxLayout(this);

    QString explanationText;
    if (!modifierUsageEnabled) {
        explanationText =
            tr("This Bottom uses Legacy Modifier Groups. Probability Envelope "
               "and Group Name remain active selection fields.");
    } else if (showLegacyFields) {
        explanationText =
            tr("Default ON chance and exceptions are edited in the main list. "
               "Legacy fields remain available because an inherited modifier "
               "can also reach a Bottom using Legacy Modifier Groups.");
    } else {
        explanationText =
            tr("Default ON chance and conditional exceptions are edited in the "
               "main Modifier Usage list. This window controls what the selected "
               "modifier does.");
    }
    auto* explanation = new QLabel(explanationText, this);
    explanation->setWordWrap(true);
    root->addWidget(explanation);

    auto* effectGroup = new QGroupBox(tr("Effect parameters"), this);
    auto* effectLayout = new QGridLayout(effectGroup);

    auto* applyLabel = new QLabel(tr("Apply to:"), effectGroup);
    m_applyCombo = new QComboBox(effectGroup);
    m_applyCombo->addItems({tr("SOUND"), tr("PARTIAL")});
    m_applyCombo->setCurrentIndex(m_modifier.applyhow_flag ? 1 : 0);
    effectLayout->addWidget(applyLabel, 0, 0);
    effectLayout->addWidget(m_applyCombo, 0, 1, 1, 2);

    addFieldRow(effectLayout, 1, Magnitude, tr("Magnitude Envelope:"));
    addFieldRow(effectLayout, 2, Rate, tr("Rate Envelope:"));
    addFieldRow(effectLayout, 3, Width, tr("Width Envelope:"));
    addFieldRow(effectLayout, 4, Spread, tr("Detune Spread:"));
    addFieldRow(effectLayout, 5, Direction, tr("Detune Direction:"));
    addFieldRow(effectLayout, 6, Velocity, tr("Detune Velocity:"));
    addFieldRow(effectLayout, 7, PartialResult, tr("Partial Result String:"));
    root->addWidget(effectGroup);

    auto* legacyGroup = new QGroupBox(tr("Legacy compatibility"), this);
    auto* legacyLayout = new QGridLayout(legacyGroup);
    addFieldRow(legacyLayout, 0, Probability,
                modifierUsageEnabled
                    ? tr("Legacy Probability Envelope:")
                    : tr("Probability Envelope:"));
    auto* groupLabel = new QLabel(
        modifierUsageEnabled ? tr("Legacy Group Name:") : tr("Group Name:"),
        legacyGroup);
    m_groupNameEdit = new QLineEdit(m_modifier.group_name, legacyGroup);
    legacyLayout->addWidget(groupLabel, 1, 0);
    legacyLayout->addWidget(m_groupNameEdit, 1, 1, 1, 2);
    // Keep legacy values in the draft and write them back unchanged, but do
    // not burden the new workflow with controls that have no effect there.
    legacyGroup->setVisible(showLegacyFields);
    root->addWidget(legacyGroup);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted,
            this, &ModifierDetailsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &ModifierDetailsDialog::reject);
    root->addWidget(buttons);

    connect(m_applyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateVisibleFields(); });
    updateVisibleFields();
}

void ModifierDetailsDialog::addFieldRow(QGridLayout* layout, int row,
                                        Field field,
                                        const QString& labelText)
{
    auto* label = new QLabel(labelText, this);
    auto* edit = new QLineEdit(valueFor(field), this);
    auto* button = new QPushButton(
        field == PartialResult ? tr("Edit...") : tr("Insert Function"), this);
    if (field == Spread || field == Direction || field == Velocity) {
        edit->setPlaceholderText(tr("Numeric value"));
        auto* validator = new QDoubleValidator(edit);
        validator->setNotation(QDoubleValidator::StandardNotation);
        edit->setValidator(validator);
        button->setVisible(false);
    }

    layout->addWidget(label, row, 0);
    layout->addWidget(edit, row, 1);
    layout->addWidget(button, row, 2);

    m_fields.append({field, label, edit, button});
    connect(button, &QPushButton::clicked, this,
            [this, field]() { editField(field); });
}

void ModifierDetailsDialog::updateVisibleFields()
{
    const int type = static_cast<int>(m_modifier.type);
    const bool applyByPartial = (m_applyCombo->currentIndex() == 1);

    for (const FieldWidgets& widgets : m_fields) {
        const bool enabled = ModifierUiPolicy::fieldEnabled(
            type, static_cast<int>(widgets.field), applyByPartial);
        const bool supportsFunction =
            widgets.field != Spread
            && widgets.field != Direction
            && widgets.field != Velocity;
        widgets.label->setVisible(enabled);
        widgets.edit->setVisible(enabled);
        widgets.functionButton->setVisible(enabled && supportsFunction);

        if (type == 7 && widgets.field == Magnitude)
            widgets.label->setText(tr("Magnitude Envelope (cycle depth):"));
        else if (widgets.field == Magnitude)
            widgets.label->setText(tr("Magnitude Envelope:"));

        if (type == 7 && widgets.field == Rate)
            widgets.label->setText(tr("Rate Envelope (Hz):"));
        else if (widgets.field == Rate)
            widgets.label->setText(tr("Rate Envelope:"));
    }
}

void ModifierDetailsDialog::editField(Field field)
{
    FieldWidgets* widgets = nullptr;
    for (FieldWidgets& candidate : m_fields) {
        if (candidate.field == field) {
            widgets = &candidate;
            break;
        }
    }
    if (!widgets)
        return;

    if (field == PartialResult && m_modifier.type == 7) {
        PartialModifierDialog dialog(
            this, std::max(1, maximumSpectrumPartialCount()),
            widgets->edit->text());
        if (dialog.exec() == QDialog::Accepted)
            widgets->edit->setText(dialog.resultString());
        return;
    }

    FunctionGenerator dialog(this, functionReturnENV, widgets->edit->text());
    if (dialog.exec() == QDialog::Accepted
        && !dialog.getResultString().isEmpty()) {
        widgets->edit->setText(dialog.getResultString());
    }
}

QString ModifierDetailsDialog::valueFor(Field field) const
{
    switch (field) {
    case Probability: return m_modifier.probability;
    case Magnitude: return m_modifier.amplitude;
    case Rate: return m_modifier.rate;
    case Width: return m_modifier.width;
    case Spread: return m_modifier.detune_spread;
    case Direction: return m_modifier.detune_direction;
    case Velocity: return m_modifier.detune_velocity;
    case PartialResult: return m_modifier.partialresult_string;
    }
    return {};
}

void ModifierDetailsDialog::setValue(Field field, const QString& value)
{
    switch (field) {
    case Probability: m_modifier.probability = value; break;
    case Magnitude: m_modifier.amplitude = value; break;
    case Rate: m_modifier.rate = value; break;
    case Width: m_modifier.width = value; break;
    case Spread: m_modifier.detune_spread = value; break;
    case Direction: m_modifier.detune_direction = value; break;
    case Velocity: m_modifier.detune_velocity = value; break;
    case PartialResult: m_modifier.partialresult_string = value; break;
    }
}

int ModifierDetailsDialog::maximumSpectrumPartialCount() const
{
    ProjectManager* projectManager = Inst::get_project_manager();
    if (!projectManager || !projectManager->get_curr_project())
        return 1;

    int maximum = 1;
    constexpr int generatedSpectrumPartialCount = 20;
    for (const SpectrumEvent& spectrum : projectManager->spectrumevents()) {
        maximum = std::max(maximum,
                           static_cast<int>(spectrum.spectrum.partials.size()));
        bool validCount = false;
        const int declared = spectrum.num_partials.toInt(&validCount);
        if (validCount)
            maximum = std::max(maximum, declared);
        if (!spectrum.generate_spectrum.trimmed().isEmpty())
            maximum = std::max(maximum, generatedSpectrumPartialCount);
    }
    return maximum;
}

void ModifierDetailsDialog::accept()
{
    m_modifier.applyhow_flag = (m_applyCombo->currentIndex() == 1);
    m_modifier.group_name = m_groupNameEdit->text();
    for (const FieldWidgets& widgets : m_fields)
        setValue(widgets.field, widgets.edit->text());
    QDialog::accept();
}
