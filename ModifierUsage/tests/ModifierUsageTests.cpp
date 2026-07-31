#include "ModifierUsage.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace dissco::modifier_usage;

namespace {

void expect(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void expectNear(double actual, double expected, const std::string& message)
{
    constexpr double tolerance = 1.0e-12;
    if (std::abs(actual - expected) > tolerance) {
        throw std::runtime_error(
            message + ": expected " + std::to_string(expected)
            + ", got " + std::to_string(actual));
    }
}

bool hasDiagnostic(const CompileResult& result, DiagnosticCode code)
{
    for (const Diagnostic& diagnostic : result.diagnostics) {
        if (diagnostic.code == code)
            return true;
    }
    return false;
}

void expectInvalid(Config config,
                   DiagnosticCode expectedCode,
                   const std::string& message)
{
    CompileResult result = compile(std::move(config));
    expect(!result.program.has_value(), message + " unexpectedly compiled");
    expect(hasDiagnostic(result, expectedCode),
           message + " did not report the expected diagnostic");
}

Config professorExample(SamplingScope scope = SamplingScope::PerSound)
{
    Config config;
    config.scope = scope;
    config.orderedModifiers = {
        Entry{"V", 0.60, {}},
        Entry{"T", 0.45, {
            Rule{{Predicate{"V", true}}, 0.30}
        }},
        Entry{"P", 0.15, {
            Rule{{Predicate{"V", true}, Predicate{"T", true}}, 0.25},
            Rule{{Predicate{"V", true}, Predicate{"T", false}}, 0.20},
            Rule{{Predicate{"V", false}, Predicate{"T", true}}, 0.65}
        }}
    };
    return config;
}

void testProfessorExample()
{
    CompileResult result = compile(professorExample());
    expect(result.program.has_value(), "Professor example failed to compile");
    expect(result.diagnostics.empty(), "Professor example produced diagnostics");

    Program program = std::move(*result.program);
    const std::vector<OverallUsage>& overall = program.overallUsage();
    expect(overall.size() == 3, "Professor example usage count is wrong");
    expect(overall[0].id == "V", "V usage is out of order");
    expect(overall[1].id == "T", "T usage is out of order");
    expect(overall[2].id == "P", "P usage is out of order");
    expectNear(overall[0].chance, 0.600, "V overall usage is wrong");
    expectNear(overall[1].chance, 0.360, "T overall usage is wrong");
    expectNear(overall[2].chance, 0.279, "P overall usage is wrong");

    int randomCalls = 0;
    const Selection selected = program.select([&randomCalls] {
        ++randomCalls;
        return 0.50;
    });
    expect(randomCalls == 1, "Direct sampling did not use exactly one random value");
    expect(selected.orderedOnIds == std::vector<ModifierId>{"V"},
           "r=0.50 should select only Vibrato");
}

void testZeroAndOneProbabilities()
{
    Config config;
    config.orderedModifiers = {
        Entry{"never", 0.0, {}},
        Entry{"always", 1.0, {}}
    };

    CompileResult result = compile(std::move(config));
    expect(result.program.has_value(), "0/1 probability config failed to compile");
    Program program = std::move(*result.program);
    const Selection selected = program.select([] { return 0.999999; });
    expect(selected.orderedOnIds == std::vector<ModifierId>{"always"},
           "p=0/p=1 decisions are wrong");
    expectNear(program.overallUsage()[0].chance, 0.0, "p=0 overall usage is wrong");
    expectNear(program.overallUsage()[1].chance, 1.0, "p=1 overall usage is wrong");
}

void testSamplingScopeRandomCalls()
{
    Config perSound;
    perSound.scope = SamplingScope::PerSound;
    perSound.orderedModifiers = {Entry{"A", 0.5, {}}};
    CompileResult soundResult = compile(std::move(perSound));
    expect(soundResult.program.has_value(), "PerSound config failed to compile");
    Program soundProgram = std::move(*soundResult.program);

    int soundCalls = 0;
    const auto soundRandom = [&soundCalls] {
        ++soundCalls;
        return soundCalls == 1 ? 0.25 : 0.75;
    };
    const Selection firstSound = soundProgram.select(soundRandom);
    const Selection secondSound = soundProgram.select(soundRandom);
    expect(soundCalls == 2, "PerSound did not draw once per selection");
    expect(firstSound.orderedOnIds == std::vector<ModifierId>{"A"},
           "First PerSound selection is wrong");
    expect(secondSound.orderedOnIds.empty(),
           "Second PerSound selection is wrong");

    Config perBottom;
    perBottom.scope = SamplingScope::PerBottom;
    perBottom.orderedModifiers = {Entry{"A", 0.5, {}}};
    CompileResult bottomResult = compile(std::move(perBottom));
    expect(bottomResult.program.has_value(), "PerBottom config failed to compile");
    Program bottomProgram = std::move(*bottomResult.program);

    int bottomCalls = 0;
    const auto bottomRandom = [&bottomCalls] {
        ++bottomCalls;
        return bottomCalls == 1 ? 0.25 : 0.75;
    };
    const Selection firstBottom = bottomProgram.select(bottomRandom);
    const Selection secondBottom = bottomProgram.select(bottomRandom);
    expect(bottomCalls == 1, "PerBottom drew more than once");
    expect(firstBottom == secondBottom, "PerBottom did not return its cached selection");
}

void testEmptyConfigDoesNotDraw()
{
    for (SamplingScope scope :
         {SamplingScope::PerSound, SamplingScope::PerBottom}) {
        Config config;
        config.scope = scope;
        CompileResult result = compile(std::move(config));
        expect(result.program.has_value(), "Empty config failed to compile");

        int calls = 0;
        Program program = std::move(*result.program);
        const Selection first = program.select([&calls] {
            ++calls;
            return 0.5;
        });
        const Selection second = program.select([&calls] {
            ++calls;
            return 0.5;
        });
        expect(first.orderedOnIds.empty() && second.orderedOnIds.empty(),
               "Empty config selected a modifier");
        expect(calls == 0, "Empty config consumed a random value");
    }
}

void testMostSpecificRuleWins()
{
    Config config;
    config.orderedModifiers = {
        Entry{"A", 0.5, {}},
        Entry{"B", 0.5, {}},
        Entry{"C", 0.1, {
            Rule{{Predicate{"A", true}}, 0.2},
            Rule{{Predicate{"A", true}, Predicate{"B", true}}, 0.8}
        }}
    };

    CompileResult result = compile(std::move(config));
    expect(result.program.has_value(), "Specificity config failed to compile");
    Program program = std::move(*result.program);
    const Selection selected = program.select([] { return 0.10; });
    expect(selected.orderedOnIds == std::vector<ModifierId>({"A", "B", "C"}),
           "Most-specific matching rule did not win");
}

void testValidation()
{
    expectInvalid(
        Config{SamplingScope::PerSound, {Entry{"", 0.5, {}}}},
        DiagnosticCode::EmptyId,
        "Empty ID");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}}, Entry{"A", 0.5, {}}}},
        DiagnosticCode::DuplicateId,
        "Duplicate ID");

    expectInvalid(
        Config{SamplingScope::PerSound, {Entry{"A", 1.1, {}}}},
        DiagnosticCode::InvalidProbability,
        "Invalid default probability");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}},
                Entry{"B", 0.5, {Rule{{Predicate{"A", true}}, -0.1}}}}},
        DiagnosticCode::InvalidProbability,
        "Invalid rule probability");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}},
                Entry{"B", 0.5, {Rule{{}, 0.2}}}}},
        DiagnosticCode::EmptyRule,
        "Empty rule");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}},
                Entry{"B", 0.5,
                      {Rule{{Predicate{"A", true}, Predicate{"A", true}}, 0.2}}}}},
        DiagnosticCode::DuplicatePredicate,
        "Duplicate predicate");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}},
                Entry{"B", 0.5,
                      {Rule{{Predicate{"A", true}, Predicate{"A", false}}, 0.2}}}}},
        DiagnosticCode::ConflictingPredicate,
        "Conflicting predicate");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {Rule{{Predicate{"missing", true}}, 0.2}}}}},
        DiagnosticCode::MissingReference,
        "Missing reference");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {Rule{{Predicate{"A", true}}, 0.2}}}}},
        DiagnosticCode::SelfReference,
        "Self reference");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {Rule{{Predicate{"B", true}}, 0.2}}},
                Entry{"B", 0.5, {}}}},
        DiagnosticCode::ForwardReference,
        "Forward reference");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}},
                Entry{"B", 0.5, {}},
                Entry{"C", 0.5,
                      {Rule{{Predicate{"A", true}}, 0.2},
                       Rule{{Predicate{"B", true}}, 0.3}}}}},
        DiagnosticCode::AmbiguousRules,
        "Equal-specificity overlapping rules");

    expectInvalid(
        Config{static_cast<SamplingScope>(99), {}},
        DiagnosticCode::InvalidSamplingScope,
        "Invalid sampling scope");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", std::numeric_limits<double>::quiet_NaN(), {}}}},
        DiagnosticCode::InvalidProbability,
        "NaN probability");
}

void testInvalidRandomValue()
{
    Config config;
    config.orderedModifiers = {Entry{"A", 0.5, {}}};
    CompileResult result = compile(std::move(config));
    expect(result.program.has_value(), "Random validation config failed to compile");
    Program program = std::move(*result.program);

    bool threw = false;
    try {
        program.select([] { return 1.0; });
    } catch (const std::domain_error&) {
        threw = true;
    }
    expect(threw, "Out-of-range random value was accepted");
}

} // namespace

int main()
{
    try {
        testProfessorExample();
        testZeroAndOneProbabilities();
        testSamplingScopeRandomCalls();
        testEmptyConfigDoesNotDraw();
        testMostSpecificRuleWins();
        testValidation();
        testInvalidRandomValue();
    } catch (const std::exception& error) {
        std::cerr << "ModifierUsageTests failed: " << error.what() << '\n';
        return 1;
    }

    std::cout << "ModifierUsageTests passed\n";
    return 0;
}
