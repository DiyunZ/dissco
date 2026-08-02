#include "ModifierDetailsDialog.hpp"

#include "FunctionGenerator.hpp"
#include "PartialModifierDialog.hpp"
#include "../inst.hpp"
#include "../widgets/ModifierUiPolicy.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

using enum FunctionReturnType;

ModifierDetailsDialog::ModifierDetailsDialog(const Modifier& modifier,
                                             QWidget* parent)
    : QDialog(parent),
      m_modifier(modifier)
{
    setWindowTitle(tr("%1 Parameters")
        .arg(ModifierUiPolicy::displayName(static_cast<int>(m_modifier.type))));
    setModal(true);
    resize(680, 420);

    auto* root = new QVBoxLayout(this);

    auto* explanation = new QLabel(
        tr("Default ON chance and conditional exceptions are edited in the "
           "main Modifier Usage list. This window controls what the selected "
           "modifier does."),
        this);
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
    addFieldRow(effectLayout, 7, PartialResult, tr("Partial Parameters:"));
    root->addWidget(effectGroup);

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

    if (field == PartialResult) {
        PartialModifierDialog dialog(
            this, static_cast<int>(m_modifier.type),
            std::max(1, maximumSpectrumPartialCount()),
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
    const bool applyByPartial = (m_applyCombo->currentIndex() == 1);
    for (const FieldWidgets& widgets : m_fields) {
        if (!ModifierUiPolicy::fieldEnabled(
                static_cast<int>(m_modifier.type),
                static_cast<int>(widgets.field), applyByPartial)) {
            continue;
        }

        const QString value = widgets.edit->text().trimmed();
        if (widgets.field == PartialResult) {
            const QString error = PartialModifierFormat::validationError(
                static_cast<int>(m_modifier.type), value);
            if (!error.isEmpty()) {
                QMessageBox::warning(
                    this, tr("Invalid partial parameters"), error);
                widgets.edit->setFocus();
                return;
            }
            continue;
        }
        if (value.isEmpty()
            || value.compare(
                   QStringLiteral("N/A"), Qt::CaseInsensitive) == 0) {
            QMessageBox::warning(
                this, tr("Missing modifier parameter"),
                tr("Enter or generate every parameter shown for this "
                   "modifier before saving."));
            widgets.edit->setFocus();
            return;
        }
        if ((widgets.field == Spread
             || widgets.field == Direction
             || widgets.field == Velocity)
            && !widgets.edit->hasAcceptableInput()) {
            QMessageBox::warning(
                this, tr("Invalid modifier parameter"),
                tr("Enter a valid number for every Detune parameter."));
            widgets.edit->setFocus();
            return;
        }
    }

    m_modifier.applyhow_flag = applyByPartial;
    for (const FieldWidgets& widgets : m_fields)
        setValue(widgets.field, widgets.edit->text());
    QDialog::accept();
}
