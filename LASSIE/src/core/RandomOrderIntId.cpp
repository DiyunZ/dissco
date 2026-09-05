#include "RandomOrderIntId.hpp"

#include <QDomDocument>
#include <QSet>

#include <cmath>
#include <limits>

namespace {
QSet<int> reservedIds;
int nextId = 1;

void reserveDocument(const QDomDocument& document)
{
    const auto functions = document.elementsByTagName(QStringLiteral("Fun"));
    for (int i = 0; i < functions.size(); ++i) {
        const auto function = functions.at(i).toElement();
        if (function.firstChildElement(QStringLiteral("Name")).text()
            != QStringLiteral("RandomOrderInt")) continue;
        bool valid = false;
        const double value = function.firstChildElement(QStringLiteral("Id")).text().toDouble(&valid);
        const double id = std::trunc(value);
        // CMOD evaluates numeric Id values and truncates their fractional part.
        if (valid && std::isfinite(value) && id >= std::numeric_limits<int>::min()
            && id <= std::numeric_limits<int>::max())
            reservedIds.insert(static_cast<int>(id));
    }
}
}

void RandomOrderIntId::reserve(const QString& xml)
{
    QDomDocument document;
    if (document.setContent(xml, QDomDocument::ParseOption::PreserveSpacingOnlyNodes))
        reserveDocument(document);
}

QString RandomOrderIntId::allocate()
{
    while (reservedIds.contains(nextId))
        nextId = nextId == std::numeric_limits<int>::max() ? 1 : nextId + 1;
    const int result = nextId;
    reservedIds.insert(result);
    return QString::number(result);
}

QString RandomOrderIntId::repairMissing(const QString& xml)
{
    QDomDocument document;
    if (!document.setContent(xml, QDomDocument::ParseOption::PreserveSpacingOnlyNodes))
        return xml;
    reserveDocument(document);
    bool changed = false;
    const auto functions = document.elementsByTagName(QStringLiteral("Fun"));
    for (int i = 0; i < functions.size(); ++i) {
        auto function = functions.at(i).toElement();
        if (function.firstChildElement(QStringLiteral("Name")).text()
            != QStringLiteral("RandomOrderInt")) continue;
        auto id = function.firstChildElement(QStringLiteral("Id"));
        if (!id.isNull() && !id.text().trimmed().isEmpty()) continue;
        if (id.isNull()) {
            id = document.createElement(QStringLiteral("Id"));
            function.appendChild(id);
        }
        while (!id.firstChild().isNull()) id.removeChild(id.firstChild());
        id.appendChild(document.createTextNode(allocate()));
        changed = true;
    }
    return changed ? document.toString(-1) : xml;
}
