#include "ModifierUsageQtAdapter.hpp"

#include <ModifierUsage.hpp>

#include <limits>
#include <utility>

namespace {

double parseProbability(const QString& text)
{
    bool valid = false;
    const double probability = text.trimmed().toDouble(&valid);
    return valid ? probability
                 : std::numeric_limits<double>::quiet_NaN();
}

std::string utf8(const QString& text)
{
    return text.toUtf8().toStdString();
}

} // namespace

ModifierUsageAnalysis analyzeModifierUsage(
    const QList<Modifier>& modifiers,
    ModifierSamplingScope scope)
{
    using namespace dissco::modifier_usage;

    Config config;
    config.scope = scope == ModifierSamplingScope::PerBottom
        ? SamplingScope::PerBottom
        : SamplingScope::PerSound;
    config.orderedModifiers.reserve(
        static_cast<std::size_t>(modifiers.size()));

    for (const Modifier& modifier : modifiers) {
        Entry entry;
        entry.id = utf8(modifier.instance_id);
        entry.defaultOnChance =
            parseProbability(modifier.default_on_chance);
        entry.rules.reserve(
            static_cast<std::size_t>(modifier.rules.size()));

        for (const ModifierChanceRule& sourceRule : modifier.rules) {
            Rule rule;
            rule.onChance = parseProbability(sourceRule.on_chance);
            rule.when.reserve(
                static_cast<std::size_t>(sourceRule.conditions.size()));
            for (const ModifierCondition& condition
                 : sourceRule.conditions) {
                rule.when.push_back(Predicate{
                    utf8(condition.modifier_id),
                    condition.required_on
                });
            }
            entry.rules.push_back(std::move(rule));
        }
        config.orderedModifiers.push_back(std::move(entry));
    }

    CompileResult compiled = compile(std::move(config));
    ModifierUsageAnalysis analysis;
    for (const Diagnostic& diagnostic : compiled.diagnostics)
        analysis.diagnostics.append(
            QString::fromStdString(diagnostic.message));

    if (!compiled.program)
        return analysis;

    const std::vector<OverallUsage>& overall =
        compiled.program->overallUsage();
    analysis.overall_on_chances.reserve(
        static_cast<qsizetype>(overall.size()));
    for (const OverallUsage& usage : overall)
        analysis.overall_on_chances.append(usage.chance);
    return analysis;
}
