#include "FunctionEntryRow.hpp"
#include "../../dialogs/FunctionGenerator.hpp"

#include <QDomDocument>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionFrame>

namespace {
class ExpressionLineEdit : public QLineEdit {
public:
    ExpressionLineEdit()
    {
        connect(this, &QLineEdit::textChanged, this, [this](const QString& xml) {
            m_summary.clear();
            QDomDocument document;
            if (document.setContent(xml)) {
                const auto function = document.documentElement();
                const QString name = function.firstChildElement(QStringLiteral("Name")).text();
                if (function.tagName() == QStringLiteral("Fun") && !name.isEmpty()) {
                    QStringList arguments;
                    for (auto arg = function.firstChildElement(); !arg.isNull();
                         arg = arg.nextSiblingElement()) {
                        if (arg.tagName() == QStringLiteral("Name")
                            || arg.tagName() == QStringLiteral("Id")) continue;
                        const auto nested = arg.firstChildElement(QStringLiteral("Fun"));
                        arguments << (nested.isNull() ? arg.text().simplified()
                            : nested.firstChildElement(QStringLiteral("Name")).text()
                                + QStringLiteral("(…)"));
                    }
                    m_summary = name + '(' + arguments.join(QStringLiteral(", ")) + ')';
                }
            }
            setAccessibleDescription(m_summary);
            setToolTip(m_summary.isEmpty() ? QString()
                : QStringLiteral("<pre>%1</pre>").arg(xml.toHtmlEscaped()));
            update();
        });
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        if (hasFocus() || m_summary.isEmpty()) {
            QLineEdit::paintEvent(event);
            return;
        }
        // Render a summary without replacing the editable/stored expression.
        QStyleOptionFrame option;
        initStyleOption(&option);
        QPainter painter(this);
        style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, this);
        QRect area = style()->subElementRect(QStyle::SE_LineEditContents, &option, this);
        const auto margins = textMargins();
        area.adjust(margins.left() + 2, margins.top(), -margins.right() - 2, -margins.bottom());
        painter.setClipRect(area);
        style()->drawItemText(&painter, area, Qt::AlignVCenter | Qt::AlignLeft,
            palette(), isEnabled(), fontMetrics().elidedText(m_summary, Qt::ElideRight, area.width()),
            QPalette::Text);
    }

private:
    QString m_summary;
};
}

FunctionEntryRow::FunctionEntryRow(const QString& labelText,
                                   int index,
                                   FunctionReturnType fnReturnType,
                                   bool rmVisible,
                                   bool insVisible,
                                   QWidget* parent,
                                   bool fnVisible)
    : QFrame(parent),
      m_index(index),
      m_fnReturnType(fnReturnType)
{
    setFrameShape(QFrame::NoFrame);

    m_hBox    = new QHBoxLayout(this);
    // Set spacing between spectrum partial rows
    m_hBox->setContentsMargins(0, 0, 0, 0);
    m_hBox->setSpacing(4);

    m_label   = new QLabel(labelText);
    m_entry   = new ExpressionLineEdit;
    if(fnVisible) { m_fnButton = new QPushButton("fn"); }
    if(rmVisible) { m_rmButton = new QPushButton("rm"); }
    if(insVisible) { m_insButton = new QPushButton("ins"); }

    m_hBox->addWidget(m_label);
    m_hBox->addWidget(m_entry);
    if(fnVisible) { m_hBox->addWidget(m_fnButton); }
    if(rmVisible) { m_hBox->addWidget(m_rmButton); }
    if(insVisible) { m_hBox->addWidget(m_insButton); }

    if(fnVisible) { connect(m_fnButton, &QPushButton::clicked, this, &FunctionEntryRow::onFnClicked); }
    if(rmVisible) { connect(m_rmButton, &QPushButton::clicked, this, &FunctionEntryRow::onRmClicked); }
    if(insVisible) { connect(m_insButton, &QPushButton::clicked, this, &FunctionEntryRow::onInsClicked); }
    connect(m_entry,    &QLineEdit::textChanged,         this, &FunctionEntryRow::onTextChanged);
    connect(m_entry,    &QLineEdit::cursorPositionChanged, this, [this](){ emit editFocused(m_entry); });
    // Set spacing between spectrum partial rows
    m_label->setFixedHeight(24);
    m_entry->setFixedHeight(24);
    if (fnVisible) { m_fnButton->setFixedHeight(24); }
    if (rmVisible) { m_rmButton->setFixedHeight(24); }
    if (insVisible) { m_insButton->setFixedHeight(24); }

    setFixedHeight(28);
}

QString FunctionEntryRow::getText() const {
    return m_entry->text();
}

void FunctionEntryRow::setText(const QString& text) {
    if (m_entry)
        m_entry->setText(text);
}

void FunctionEntryRow::setLabel(const QString& text) {
    if (m_label)
        m_label->setText(text);
}

void FunctionEntryRow::onFnClicked() {
    FunctionGenerator* gen = new FunctionGenerator(nullptr, m_fnReturnType, m_entry->text());
    if (gen) {
        if (gen->exec() == QDialog::Accepted && !gen->getResultString().isEmpty())
            m_entry->setText(gen->getResultString());
        delete gen;
    }
}

void FunctionEntryRow::onRmClicked() {
    emit deleteRequested(this);
}

void FunctionEntryRow::onInsClicked() {
    emit insertRequested(this);
}

void FunctionEntryRow::onTextChanged() {
    emit textChanged(this);
}

FunctionEntryRow::~FunctionEntryRow() {}
