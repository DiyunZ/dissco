#ifndef FUNCTIONXMLFORMAT_HPP
#define FUNCTIONXMLFORMAT_HPP

#include <QString>

namespace FunctionXmlFormat {

// Change XML layout without changing parameter text. Invalid input is left
// untouched so the Result String can still be used to inspect and fix typos.
QString preview(const QString& xml);
QString compact(const QString& xml);

} // namespace FunctionXmlFormat

#endif // FUNCTIONXMLFORMAT_HPP
