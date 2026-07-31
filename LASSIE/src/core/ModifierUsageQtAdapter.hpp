#ifndef MODIFIERUSAGEQTADAPTER_HPP
#define MODIFIERUSAGEQTADAPTER_HPP

#include "event_struct.hpp"

#include <QStringList>
#include <QVector>

struct ModifierUsageAnalysis {
    QVector<double> overall_on_chances;
    QStringList diagnostics;

    bool isValid() const { return diagnostics.isEmpty(); }
};

/**
 * Validates a LASSIE modifier list and calculates each exact marginal ON rate.
 *
 * Runtime sampling stays in the shared, Qt-free ModifierUsage module. This
 * adapter is deliberately narrow so the editor preview cannot acquire its own
 * subtly different probability semantics.
 */
ModifierUsageAnalysis analyzeModifierUsage(
    const QList<Modifier>& modifiers,
    ModifierSamplingScope scope);

#endif // MODIFIERUSAGEQTADAPTER_HPP
