#include "PartialModifierDialog.hpp"

#include "FunctionGenerator.hpp"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>

namespace {

struct ParameterPresentation {
    QString magnitudeLabel;
    QString widthLabel;
    QString rateLabel;
    bool usesMagnitude = false;
    bool usesWidth = false;
    bool usesRate = false;
};

ParameterPresentation presentationFor(int modifierType)
{
    switch (modifierType) {
    case 0:
        return {QObject::tr("Magnitude (depth):"), QString(),
                QObject::tr("Rate (Hz):"), true, false, true};
    case 1:
        return {QObject::tr("Magnitude (frequency depth):"), QString(),
                QObject::tr("Rate (Hz):"), true, false, true};
    case 2:
        return {QObject::tr("Frequency change:"), QString(), QString(),
                true, false, false};
    case 3:
        return {QObject::tr("Detuning:"), QString(), QString(),
                true, false, false};
    case 4:
    case 5:
        return {QObject::tr("Magnitude:"), QObject::tr("Width:"),
                QObject::tr("Rate:"), true, true, true};
    case 6:
        return {QObject::tr("Wave type:"), QString(), QString(),
                true, false, false};
    case 7:
        return {QObject::tr("Magnitude (cycle depth):"), QString(),
                QObject::tr("Rate (Hz):"), true, false, true};
    default:
        return {QObject::tr("Magnitude:"), QObject::tr("Width:"),
                QObject::tr("Rate:"), true, true, true};
    }
}

QString unusedLabel(const QString& parameter)
{
    return QObject::tr("%1 (not used by this modifier):").arg(parameter);
}

} // namespace

PartialModifierDialog::PartialModifierDialog(QWidget* parent,
                                             int modifierType,
                                             int suggestedPartialCount,
                                             const QString& originalString)
    : QDialog(parent),
      m_modifierType(modifierType)
{
    setWindowTitle(tr("Customize Partial Parameters"));
    setModal(true);
    resize(900, 700);

    auto* mainLayout = new QVBoxLayout(this);
    auto* explanation = new QLabel(
        tr("Each row controls one spectrum partial. Probability decides whether "
           "the modifier is applied to that partial. Use Insert Function to build "
           "each envelope; N/A means that value is not used."),
        this);
    explanation->setWordWrap(true);
    mainLayout->addWidget(explanation);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    QString parseWarning;
    QVector<PartialModifierFormat::Values> values =
        PartialModifierFormat::parse(originalString, &parseWarning);
    const int requestedRows = std::max(1, suggestedPartialCount);
    m_suggestedPartialCount = requestedRows;
    const int rowCount = std::max(requestedRows, static_cast<int>(values.size()));
    values.resize(rowCount);
    m_activeRowCount = rowCount;

    if (parseWarning.isEmpty()) {
        m_statusLabel->setText(
            tr("Started with %1 partial(s), based on this value and the largest "
               "Spectrum in the project.")
                .arg(rowCount));
    } else {
        m_statusLabel->setText(parseWarning + tr(" Cancel preserves the original value."));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #b06000;"));
    }

    auto* countLayout = new QHBoxLayout;
    auto* countLabel = new QLabel(tr("Number of partial rows:"), this);
    m_rowCountSpin = new QSpinBox(this);
    m_rowCountSpin->setRange(1, std::max(1024, rowCount));
    m_rowCountSpin->setValue(rowCount);
    countLayout->addWidget(countLabel);
    countLayout->addWidget(m_rowCountSpin);
    countLayout->addStretch();
    mainLayout->addLayout(countLayout);

    m_countWarningLabel = new QLabel(
        tr("Partials after the last configured row will not receive this modifier."),
        this);
    m_countWarningLabel->setWordWrap(true);
    m_countWarningLabel->setStyleSheet(QStringLiteral("color: #b06000;"));
    m_countWarningLabel->setVisible(false);
    mainLayout->addWidget(m_countWarningLabel);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto* rowsWidget = new QWidget(scrollArea);
    m_rowsLayout = new QVBoxLayout(rowsWidget);
    m_rowsLayout->addStretch();
    for (int i = 0; i < rowCount; ++i)
        addPartialRow(i, values.at(i));
    scrollArea->setWidget(rowsWidget);
    mainLayout->addWidget(scrollArea, 1);

    auto* previewLabel = new QLabel(tr("Generated Partial Result String:"), this);
    mainLayout->addWidget(previewLabel);
    m_preview = new QPlainTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setMaximumHeight(100);
    m_preview->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    mainLayout->addWidget(m_preview);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    connect(m_rowCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int count) { setPartialRowCount(count); });

    updatePreview();
}

void PartialModifierDialog::addPartialRow(
    int partialIndex, const PartialModifierFormat::Values& values)
{
    auto* group = new QGroupBox(tr("Partial %1").arg(partialIndex + 1), this);
    auto* layout = new QVBoxLayout(group);
    const ParameterPresentation presentation = presentationFor(m_modifierType);

    PartialRow row;
    row.group = group;
    addEnvelopeEntry(layout, tr("Probability:"), values.probability, true, &row.probability);

    addEnvelopeEntry(
        layout,
        presentation.usesMagnitude
            ? presentation.magnitudeLabel : unusedLabel(tr("Magnitude")),
        values.magnitude, presentation.usesMagnitude, &row.magnitude);

    addEnvelopeEntry(
        layout,
        presentation.usesWidth
            ? presentation.widthLabel : unusedLabel(tr("Width")),
        values.width, presentation.usesWidth, &row.width);

    addEnvelopeEntry(
        layout,
        presentation.usesRate
            ? presentation.rateLabel : unusedLabel(tr("Rate")),
        values.rate, presentation.usesRate, &row.rate);

    m_rows.append(row);
    m_rowsLayout->insertWidget(m_rowsLayout->count() - 1, group);
}

void PartialModifierDialog::addEnvelopeEntry(QVBoxLayout* layout,
                                             const QString& label,
                                             const QString& value,
                                             bool enabled,
                                             QLineEdit** entry)
{
    auto* rowLayout = new QHBoxLayout;
    auto* rowLabel = new QLabel(label, this);
    rowLabel->setMinimumWidth(225);
    auto* lineEdit = new QLineEdit(
        PartialModifierFormat::normalizedValue(value, enabled), this);
    auto* button = new QPushButton(tr("Insert Function"), this);

    lineEdit->setEnabled(enabled);
    button->setEnabled(enabled);
    rowLabel->setEnabled(enabled);
    lineEdit->setToolTip(enabled
        ? tr("An ENV-returning function, or N/A.")
        : tr("This value is not used by the selected modifier type."));

    connect(button, &QPushButton::clicked, this,
            [this, lineEdit]() { openEnvelopeGenerator(lineEdit); });
    connect(lineEdit, &QLineEdit::textChanged, this,
            [this]() { updatePreview(); });

    rowLayout->addWidget(rowLabel);
    rowLayout->addWidget(lineEdit, 1);
    rowLayout->addWidget(button);
    layout->addLayout(rowLayout);
    *entry = lineEdit;
}

void PartialModifierDialog::openEnvelopeGenerator(QLineEdit* entry)
{
    QString original = entry->text().trimmed();
    if (original == QStringLiteral("N/A"))
        original.clear();

    FunctionGenerator generator(this, FunctionReturnType::functionReturnENV, original);
    if (generator.exec() == QDialog::Accepted) {
        const QString result = generator.getResultString().trimmed();
        if (!result.isEmpty())
            entry->setText(result);
    }
}

void PartialModifierDialog::setPartialRowCount(int count)
{
    while (m_rows.size() < count)
        addPartialRow(m_rows.size(), PartialModifierFormat::Values{});

    m_activeRowCount = count;
    for (int index = 0; index < m_rows.size(); ++index)
        m_rows[index].group->setVisible(index < m_activeRowCount);
    m_countWarningLabel->setVisible(count < m_suggestedPartialCount);
    updatePreview();
}

QString PartialModifierDialog::resultString() const
{
    QVector<PartialModifierFormat::Values> values;
    values.reserve(m_activeRowCount);
    for (int index = 0; index < m_activeRowCount; ++index) {
        const PartialRow& row = m_rows[index];
        PartialModifierFormat::Values value;
        value.probability = row.probability->text();
        value.magnitude = row.magnitude->text();
        value.width = row.width->text();
        value.rate = row.rate->text();
        values.append(value);
    }
    return PartialModifierFormat::serialize(values);
}

void PartialModifierDialog::updatePreview()
{
    if (m_preview)
        m_preview->setPlainText(resultString());
}
