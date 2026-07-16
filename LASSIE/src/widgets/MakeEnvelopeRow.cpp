#include "MakeEnvelopeRow.hpp"
#include "../dialogs/FunctionGenerator.hpp"

MakeEnvelopeRow::MakeEnvelopeRow(int index, QWidget* parent)
    : QFrame(parent), m_index(index)
{
    setFrameShape(QFrame::NoFrame);

    m_vBox = new QVBoxLayout(this);
    m_vBox->setContentsMargins(0, 0, 0, 0);
    m_vBox->setSpacing(4);

    m_label   = new QLabel(QString("Node %1:").arg(index + 1));
    m_xEdit   = new QLineEdit;
    m_xFnBtn  = new QPushButton("fn");
    m_yEdit   = new QLineEdit;
    m_yFnBtn  = new QPushButton("fn");

    m_typeCombo = new QComboBox;
    m_typeCombo->addItem("LINEAR");
    m_typeCombo->addItem("SPLINE");
    m_typeCombo->addItem("EXPONENTIAL");

    m_proCombo = new QComboBox;
    m_proCombo->addItem("FLEXIBLE");
    m_proCombo->addItem("FIXED");

    m_rmBtn  = new QPushButton("rm");
    m_insBtn = new QPushButton("ins");

    auto* firstRow = new QHBoxLayout;
    auto* secondRow = new QHBoxLayout;
    auto* thirdRow = new QHBoxLayout;

    firstRow->addWidget(m_label);
    firstRow->addWidget(new QLabel("X:"));
    firstRow->addWidget(m_xEdit);
    firstRow->addWidget(m_xFnBtn);
    secondRow->addWidget(new QLabel("Y:"));
    secondRow->addWidget(m_yEdit);
    secondRow->addWidget(m_yFnBtn);
    thirdRow->addWidget(m_typeCombo);
    thirdRow->addWidget(m_proCombo);
    thirdRow->addWidget(m_rmBtn);
    thirdRow->addWidget(m_insBtn);

    m_xEdit->setFixedHeight(30);
    m_yEdit->setFixedHeight(30);
    m_xEdit->setMinimumWidth(70);
    m_yEdit->setMinimumWidth(70);

    m_vBox->addLayout(firstRow);
    m_vBox->addLayout(secondRow);
    m_vBox->addLayout(thirdRow);

    connect(m_xFnBtn,    &QPushButton::clicked, this, &MakeEnvelopeRow::onXFnClicked);
    connect(m_yFnBtn,    &QPushButton::clicked, this, &MakeEnvelopeRow::onYFnClicked);
    connect(m_rmBtn,     &QPushButton::clicked, this, &MakeEnvelopeRow::onRmClicked);
    connect(m_insBtn,    &QPushButton::clicked, this, &MakeEnvelopeRow::onInsClicked);
    connect(m_xEdit,     &QLineEdit::textChanged,
            this, &MakeEnvelopeRow::onChanged);
    connect(m_yEdit,     &QLineEdit::textChanged,
            this, &MakeEnvelopeRow::onChanged);
    connect(m_typeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MakeEnvelopeRow::onChanged);
    connect(m_proCombo,  qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MakeEnvelopeRow::onChanged);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

MakeEnvelopeRow::~MakeEnvelopeRow() {}

void MakeEnvelopeRow::setIndex(int index) {
    m_index = index;
    setLabel(QString("Point %1:").arg(index + 1));
}

QString MakeEnvelopeRow::getX() const { return m_xEdit->text(); }
QString MakeEnvelopeRow::getY() const { return m_yEdit->text(); }
QString MakeEnvelopeRow::getType() const { return m_typeCombo->currentText(); }
QString MakeEnvelopeRow::getPro()  const { return m_proCombo->currentText(); }

void MakeEnvelopeRow::setX(const QString& text) { m_xEdit->setText(text); }
void MakeEnvelopeRow::setY(const QString& text) { m_yEdit->setText(text); }

void MakeEnvelopeRow::setType(const QString& type) {
    int i = m_typeCombo->findText(type);
    if (i >= 0) m_typeCombo->setCurrentIndex(i);
}

void MakeEnvelopeRow::setPro(const QString& pro) {
    int i = m_proCombo->findText(pro);
    if (i >= 0) m_proCombo->setCurrentIndex(i);
}

void MakeEnvelopeRow::setLabel(const QString& text) {
    if (m_label) m_label->setText(text);
}

void MakeEnvelopeRow::onXFnClicked() {
    FunctionGenerator* gen = new FunctionGenerator(nullptr, FunctionReturnType::functionReturnFloat, m_xEdit->text());
    if (gen->exec() == QDialog::Accepted && !gen->getResultString().isEmpty())
        m_xEdit->setText(gen->getResultString());
    delete gen;
}

void MakeEnvelopeRow::onYFnClicked() {
    FunctionGenerator* gen = new FunctionGenerator(nullptr, FunctionReturnType::functionReturnFloat, m_yEdit->text());
    if (gen->exec() == QDialog::Accepted && !gen->getResultString().isEmpty())
        m_yEdit->setText(gen->getResultString());
    delete gen;
}

void MakeEnvelopeRow::onRmClicked()  { emit deleteRequested(this); }
void MakeEnvelopeRow::onInsClicked() { emit insertRequested(this); }
void MakeEnvelopeRow::onChanged()    { emit textChanged(this); }
