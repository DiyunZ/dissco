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
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

using enum FunctionReturnType;

ModifierDetailsDialog::ModifierDetailsDialog(const Modifier& modifier,
                                             Eventtype eventType,
                                             unsigned eventIndex,
                                             QWidget* parent)
    : QDialog(parent),
      m_modifier(modifier),
      m_eventType(eventType),
      m_eventIndex(eventIndex)
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
    applyLabel->setBuddy(m_applyCombo);
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
    if (field == PartialResult)
        edit->setReadOnly(true);
    label->setBuddy(edit);
    auto* button = new QPushButton(
        field == PartialResult ? tr("Customize...") : tr("Insert Function"), this);
    if (field == Spread || field == Direction || field == Velocity) {
        edit->setPlaceholderText(
            field == Direction
                ? tr("Negative = detune; positive = tune")
                : tr("Numeric value"));
        auto* validator = new QDoubleValidator(edit);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setLocale(QLocale::c());
        if (field == Spread) {
            validator->setBottom(0.0);
            validator->setTop(1.0);
        } else if (field == Velocity) {
            validator->setBottom(-1.0);
            validator->setTop(1.0);
        }
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
        const bool visible = ModifierUiPolicy::fieldVisible(
            type, static_cast<int>(widgets.field), applyByPartial);
        const bool enabled = ModifierUiPolicy::fieldEnabled(
            type, static_cast<int>(widgets.field), applyByPartial);
        const bool supportsFunction =
            widgets.field != Spread
            && widgets.field != Direction
            && widgets.field != Velocity;
        widgets.label->setVisible(visible);
        widgets.edit->setVisible(visible);
        widgets.functionButton->setVisible(visible && supportsFunction);
        widgets.label->setEnabled(enabled);
        widgets.edit->setEnabled(enabled);
        widgets.functionButton->setEnabled(enabled);
        if (visible && !enabled) {
            const QString explanation = tr(
                "This is a SOUND parameter. Switch Apply to SOUND to edit it.");
            widgets.label->setToolTip(explanation);
            widgets.edit->setToolTip(explanation);
            widgets.functionButton->setToolTip(explanation);
            widgets.label->setAccessibleDescription(explanation);
            widgets.edit->setAccessibleDescription(explanation);
            widgets.functionButton->setAccessibleDescription(explanation);
        } else {
            widgets.label->setToolTip(QString());
            widgets.edit->setToolTip(QString());
            widgets.functionButton->setToolTip(QString());
            widgets.label->setAccessibleDescription(QString());
            widgets.edit->setAccessibleDescription(QString());
            widgets.functionButton->setAccessibleDescription(QString());
        }

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
            partialRowConstraint(),
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

ModifierUiPolicy::PartialRowConstraint
ModifierDetailsDialog::partialRowConstraint() const
{
    ModifierUiPolicy::PartialRowConstraint constraint;
    ProjectManager* projectManager = Inst::get_project_manager();
    if (!projectManager || !projectManager->get_curr_project()) {
        constraint.maximumRows = 1;
        constraint.explanation = tr(
            "No open project is available; one placeholder partial row is shown.");
        return constraint;
    }

    const QList<SpectrumEvent>& spectra = projectManager->spectrumevents();
    QList<const SpectrumEvent*> candidates;
    QStringList resolutionIssues;
    QSet<QString> seenNames;
    bool bottomScope = false;
    bool runtimeDependentPath = false;

    enum class PackageTypeKind { Spectrum, Other, RuntimeDependent };
    const auto classifyPackageType = [](const QString& value) {
        const QString type = value.trimmed();
        bool validInteger = false;
        const int integerType = type.toInt(&validInteger);
        if (validInteger) {
            return integerType == static_cast<int>(sound)
                ? PackageTypeKind::Spectrum : PackageTypeKind::Other;
        }

        bool validNumber = false;
        const double numericType = type.toDouble(&validNumber);
        if (validNumber && std::isfinite(numericType)
            && std::floor(numericType) == numericType) {
            return numericType == static_cast<double>(sound)
                ? PackageTypeKind::Spectrum : PackageTypeKind::Other;
        }

        if (type == QStringLiteral("Spectrum"))
            return PackageTypeKind::Spectrum;
        return PackageTypeKind::RuntimeDependent;
    };

    const auto addSpectrumCandidates = [&](const QString& name) {
        if (name.isEmpty()) {
            resolutionIssues.append(tr(
                "A Spectrum package has an empty event name."));
            runtimeDependentPath = true;
            return;
        }
        if (seenNames.contains(name))
            return;
        seenNames.insert(name);

        QList<const SpectrumEvent*> matches;
        for (const SpectrumEvent& spectrum : spectra) {
            if (spectrum.name == name)
                matches.append(&spectrum);
        }
        if (matches.isEmpty()) {
            resolutionIssues.append(tr(
                "Referenced Spectrum \"%1\" does not exist.").arg(name));
            runtimeDependentPath = true;
            return;
        }
        if (matches.size() > 1) {
            resolutionIssues.append(tr(
                "Spectrum name \"%1\" is duplicated, so the reference is "
                "ambiguous.").arg(name));
            runtimeDependentPath = true;
        }
        for (const SpectrumEvent* match : matches) {
            if (!candidates.contains(match))
                candidates.append(match);
        }
    };

    if (m_eventType == bottom) {
        if (m_eventIndex
            >= static_cast<unsigned>(projectManager->bottomevents().size())) {
            constraint.maximumRows = 0;
            constraint.explanation = tr(
                "The current Bottom event is no longer available. Reopen its "
                "modifier before configuring partial rows.");
            return constraint;
        }
        bottomScope = true;
        const HEvent& bottomEvent =
            projectManager->bottomevents()[static_cast<int>(m_eventIndex)].event;
        for (const Layer& layer : bottomEvent.event_layers) {
            for (const Package& package : layer.discrete_packages) {
                const PackageTypeKind kind =
                    classifyPackageType(package.event_type);
                if (kind == PackageTypeKind::Other)
                    continue;
                if (kind == PackageTypeKind::RuntimeDependent) {
                    runtimeDependentPath = true;
                    resolutionIssues.append(tr(
                        "The child type for package \"%1\" is evaluated at "
                        "runtime, so its Spectrum status cannot be guaranteed.")
                        .arg(package.event_name));
                }
                // EventName is a literal key in CMOD. Do not trim or otherwise
                // normalize it here, or the editor could validate a reference
                // that runtime lookup will reject.
                addSpectrumCandidates(package.event_name);
            }
        }
    } else {
        for (const SpectrumEvent& spectrum : spectra) {
            if (seenNames.contains(spectrum.name)) {
                runtimeDependentPath = true;
                resolutionIssues.append(tr(
                    "Spectrum name \"%1\" is duplicated in the project.")
                    .arg(spectrum.name));
            }
            seenNames.insert(spectrum.name);
            candidates.append(&spectrum);
        }
    }

    if (candidates.isEmpty()) {
        constraint.maximumRows = 0;
        constraint.explanation = bottomScope
            ? tr("This Bottom has no statically resolved Spectrum candidate. "
                 "PARTIAL settings do not affect note-only children; a runtime "
                 "child expression cannot be capped in the editor.")
            : tr("No reachable Spectrum can be resolved for this modifier. "
                 "One placeholder row is shown without a Spectrum-derived limit.");
        if (!resolutionIssues.isEmpty())
            constraint.explanation += tr("\nReview: %1")
                .arg(resolutionIssues.join(QStringLiteral(" ")));
        return constraint;
    }

    bool exactMaximum = !runtimeDependentPath;
    int maximum = 1;
    for (const SpectrumEvent* spectrum : candidates) {
        const ModifierUiPolicy::SpectrumPartialCount count =
            ModifierUiPolicy::spectrumPartialCount(*spectrum);
        maximum = std::max(maximum, count.count);
        exactMaximum = exactMaximum && count.exact;

        if (!count.generated && count.exact) {
            int configuredPartials = 0;
            for (const QString& partial : spectrum->spectrum.partials) {
                if (!partial.trimmed().isEmpty())
                    ++configuredPartials;
            }
            if (configuredPartials != count.count) {
                resolutionIssues.append(tr(
                    "Spectrum \"%1\" declares %2 partials but contains %3 "
                    "configured partial envelopes.")
                    .arg(spectrum->name)
                    .arg(count.count)
                    .arg(configuredPartials));
            }
            if (!spectrum->generate_spectrum.trimmed().isEmpty()) {
                resolutionIssues.append(tr(
                    "Spectrum \"%1\" has GenerateSpectrum text but no function "
                    "element; CMOD will use its explicit partial list.")
                    .arg(spectrum->name));
            }
        } else if (!count.exact) {
            resolutionIssues.append(tr(
                "Spectrum \"%1\" determines NumberOfPartials at runtime.")
                .arg(spectrum->name));
        }
    }

    constraint.suggestedRows = maximum;
    constraint.maximumRows = exactMaximum ? maximum : 0;
    if (!exactMaximum) {
        constraint.explanation = bottomScope
            ? tr("At least one Spectrum referenced by this Bottom has a runtime-"
                 "determined or inconsistent partial count. %1 rows are suggested; "
                 "CMOD uses the actual count at runtime.").arg(maximum)
            : tr("At least one project Spectrum has a runtime-determined or "
                 "inconsistent partial count. %1 rows are suggested; inherited "
                 "modifiers use each sound's actual count at runtime.").arg(maximum);
    } else if (bottomScope && candidates.size() == 1) {
        constraint.explanation = tr(
            "This Bottom references Spectrum \"%1\", which declares up to %2 "
            "partials. You can configure at most %2 rows; CMOD may use fewer "
            "at high base frequencies.")
            .arg(candidates.front()->name)
            .arg(maximum);
    } else if (bottomScope) {
        constraint.explanation = tr(
            "This Bottom can choose %1 Spectra; the largest has %2 partials. "
            "You can configure at most %2 rows; smaller spectra ignore later rows.")
            .arg(candidates.size())
            .arg(maximum);
    } else {
        constraint.explanation = tr(
            "For inherited modifiers, the project-wide safe maximum covers %1 "
            "Spectra; the largest declares up to %2 partials. You can configure "
            "at most %2 rows.")
            .arg(candidates.size())
            .arg(maximum);
    }
    if (!resolutionIssues.isEmpty())
        constraint.explanation += tr("\nReview: %1")
            .arg(resolutionIssues.join(QStringLiteral(" ")));
    return constraint;
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
        if (widgets.field == Direction) {
            bool validDirection = false;
            const double direction = value.toDouble(&validDirection);
            if (!validDirection || !std::isfinite(direction)
                || direction == 0.0) {
                QMessageBox::warning(
                    this, tr("Invalid Detune direction"),
                    tr("Enter a negative value to detune or a positive "
                       "value to tune. Direction cannot be zero."));
                widgets.edit->setFocus();
                return;
            }
        }
    }

    m_modifier.applyhow_flag = applyByPartial;
    for (const FieldWidgets& widgets : m_fields) {
        if (widgets.field == Direction
            && ModifierUiPolicy::fieldEnabled(
                static_cast<int>(m_modifier.type),
                static_cast<int>(widgets.field), applyByPartial)) {
            const double direction = widgets.edit->text().toDouble();
            setValue(widgets.field,
                     direction < 0.0 ? QStringLiteral("-1")
                                     : QStringLiteral("1"));
        } else {
            setValue(widgets.field, widgets.edit->text());
        }
    }
    QDialog::accept();
}
