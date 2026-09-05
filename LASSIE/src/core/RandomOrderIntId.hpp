#ifndef RANDOMORDERINTID_HPP
#define RANDOMORDERINTID_HPP

#include <QString>

namespace RandomOrderIntId {
// Reserve IDs before editing a project or an expression so that newly
// created functions cannot share a pool with an existing function.
void reserve(const QString& xml);
QString allocate();

// Only valid XML is changed. Existing non-empty IDs and all other nodes
// are retained; each missing/blank ID receives its own pool.
QString repairMissing(const QString& xml);
}

#endif
