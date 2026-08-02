#ifndef DISSCO_MODIFIER_USAGE_HPP
#define DISSCO_MODIFIER_USAGE_HPP

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace dissco::modifier_usage {

using ModifierId = std::string;
using UnitRandom = std::function<double()>;

enum class SamplingScope {
    PerSound,
    PerBottom
};

struct Predicate {
    ModifierId modifierId;
    bool requiredOn = false;
};

struct Rule {
    std::vector<Predicate> when;
    double onChance = 0.0;
};

struct Entry {
    ModifierId id;
    double defaultOnChance = 1.0;
    std::vector<Rule> rules;
};

struct Config {
    SamplingScope scope = SamplingScope::PerSound;
    std::vector<Entry> orderedModifiers;
};

enum class OverallUsageMode {
    Exact,
    Skip
};

struct CompileOptions {
    OverallUsageMode overallUsageMode = OverallUsageMode::Exact;
};

enum class DiagnosticCode {
    InvalidSamplingScope,
    EmptyId,
    DuplicateId,
    InvalidProbability,
    EmptyRule,
    DuplicatePredicate,
    ConflictingPredicate,
    MissingReference,
    SelfReference,
    ForwardReference,
    AmbiguousRules
};

struct Diagnostic {
    DiagnosticCode code;
    ModifierId targetId;
    std::string message;
};

struct OverallUsage {
    ModifierId id;
    double chance = 0.0;
};

struct Selection {
    // IDs remain in the configured selection/application order.
    std::vector<ModifierId> orderedOnIds;

    bool operator==(const Selection&) const = default;
};

struct CompileResult;

/**
 * A validated modifier-usage program.
 *
 * The program is movable but deliberately not copyable because PerBottom
 * sampling owns a cached selection for one runtime Bottom instance.
 */
class Program {
public:
    Program(Program&&) noexcept;
    Program& operator=(Program&&) noexcept;
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;
    ~Program();

    /**
     * Select modifiers with one uniform random value in [0, 1).
     *
     * Non-empty PerSound programs call nextUnit once per invocation. Non-empty
     * PerBottom programs call it only for the first invocation and return that
     * cached selection thereafter. Empty programs draw nothing. A missing
     * callback or a non-finite/out-of-range result throws std::invalid_argument
     * or std::domain_error respectively.
     */
    Selection select(const UnitRandom& nextUnit);

    /**
     * Exact marginal ON probability for every modifier, in configured order.
     * Empty when compilation explicitly skipped the exponential preview.
     */
    const std::vector<OverallUsage>& overallUsage() const noexcept;

private:
    struct Impl;
    explicit Program(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> m_impl;

    friend CompileResult compile(Config config, CompileOptions options);
};

struct CompileResult {
    std::optional<Program> program;
    std::vector<Diagnostic> diagnostics;
};

/**
 * Validate and compile a modifier-usage configuration.
 *
 * Invalid configurations return diagnostics and no Program. Rule order has no
 * semantic effect: the most-specific matching rule wins. Two rules of equal
 * specificity that can match the same history are rejected as ambiguous.
 *
 * Exact overall-usage calculation is exponential in the number of modifiers
 * in the worst case, as required for arbitrary dependencies on earlier
 * decisions. Runtime callers should select OverallUsageMode::Skip.
 */
CompileResult compile(Config config, CompileOptions options = {});

} // namespace dissco::modifier_usage

#endif // DISSCO_MODIFIER_USAGE_HPP
