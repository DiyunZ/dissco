#include "RandomOrderIntFunction.hpp"

#include "../../../core/RandomOrderIntId.hpp"
#include "../../../widgets/generic/FunctionEntryRow.hpp"
#include "../../FunctionXmlFormat.hpp"

#include <QSignalBlocker>
#include <QTextStream>
#include <QXmlStreamWriter>

namespace {
QDomElement boundElement(const QDomElement& function, const QString& tag)
{
    const auto current = function.firstChildElement(tag);
    return current.isNull() ? function.firstChildElement(tag + QStringLiteral("Bound")) : current;
}

QString innerXml(const QDomElement& element)
{
    QString xml;
    QTextStream stream(&xml);
    element.save(stream, -1);
    QXmlStreamReader reader(xml);
    return reader.readNextStartElement() ? FunctionWidget::readInner(reader) : QString();
}

void setInnerXml(QDomDocument& document, QDomElement element, const QString& value)
{
    while (!element.firstChild().isNull()) element.removeChild(element.firstChild());
    QDomDocument fragment;
    if (fragment.setContent(QStringLiteral("<Value>") + value + QStringLiteral("</Value>"),
                            QDomDocument::ParseOption::PreserveSpacingOnlyNodes)) {
        for (auto child = fragment.documentElement().firstChild(); !child.isNull();
             child = child.nextSibling())
            element.appendChild(document.importNode(child, true));
    } else {
        element.appendChild(document.createTextNode(value));
    }
}
}

RandomOrderIntFunction::RandomOrderIntFunction(QWidget* parent)
    : MultiEntryFunction({
          { tr("Lower Bound:"), "Low",
            FunctionReturnType::functionReturnInt, QStringLiteral("0") },
          { tr("Upper Bound:"), "High",
            FunctionReturnType::functionReturnInt, QStringLiteral("1") },
      }, parent)
{}

void RandomOrderIntFunction::setOriginalXml(const QString& xml)
{
    m_original.setContent(xml, QDomDocument::ParseOption::PreserveSpacingOnlyNodes);
    RandomOrderIntId::reserve(xml);
    m_poolId = m_original.documentElement().firstChildElement(QStringLiteral("Id")).text();
}

QString RandomOrderIntFunction::buildXMLString() const
{
    if (m_poolId.trimmed().isEmpty()) m_poolId = RandomOrderIntId::allocate();
    QDomDocument document = m_original.cloneNode(true).toDocument();
    if (document.documentElement().isNull())
        document.setContent(QStringLiteral("<Fun><Name>RandomOrderInt</Name></Fun>"));
    auto function = document.documentElement();
    for (int i = 0; i < m_rows.size(); ++i) {
        auto bound = boundElement(function, m_specs[i].xmlTag);
        if (bound.isNull()) {
            bound = document.createElement(m_specs[i].xmlTag);
            function.appendChild(bound);
        }
        setInnerXml(document, bound, m_rows[i]->getText());
    }
    auto id = function.firstChildElement(QStringLiteral("Id"));
    if (id.isNull()) {
        id = document.createElement(QStringLiteral("Id"));
        function.appendChild(id);
    }
    if (id.text().trimmed().isEmpty()) setInnerXml(document, id, m_poolId);
    return FunctionXmlFormat::compact(document.toString(-1));
}

void RandomOrderIntFunction::populateFromXML(QXmlStreamReader& reader)
{
    // Keep source fields even when called directly through FunctionWidget.
    QString remaining;
    QXmlStreamWriter writer(&remaining);
    int depth = 0;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isEndElement() && depth == 0) break;
        if (reader.isStartElement()) ++depth;
        else if (reader.isEndElement()) --depth;
        writer.writeCurrentToken(reader);
    }
    if (m_original.documentElement().isNull())
        setOriginalXml(QStringLiteral("<Fun><Name>RandomOrderInt</Name>")
                       + remaining + QStringLiteral("</Fun>"));
    const QSignalBlocker blocker(this);
    for (int i = 0; i < m_rows.size(); ++i)
        m_rows[i]->setText(innerXml(boundElement(m_original.documentElement(), m_specs[i].xmlTag)));
}

void RandomOrderIntFunction::reset()
{
    m_original.clear();
    m_poolId.clear();
    QSignalBlocker blocker(this);
    MultiEntryFunction::reset();
    blocker.unblock();
    emit xmlChanged();
}
