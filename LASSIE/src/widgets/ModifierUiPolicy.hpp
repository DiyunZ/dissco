#ifndef MODIFIERUIPOLICY_HPP
#define MODIFIERUIPOLICY_HPP

#include "../core/event_struct.hpp"

#include <QString>

namespace ModifierUiPolicy {

inline constexpr int fieldCount = 7;

inline bool usageSummaryVisible(Eventtype eventType)
{
    switch (eventType) {
    case top:
    case high:
    case mid:
    case low:
    case bottom:
        return true;
    default:
        return false;
    }
}

inline bool samplingScopeVisible(Eventtype eventType)
{
    return eventType == bottom;
}

inline QString displayName(int modifierType)
{
    switch (modifierType) {
    case 0: return QStringLiteral("Tremolo");
    case 1: return QStringLiteral("Vibrato");
    case 2: return QStringLiteral("Glissando");
    case 3: return QStringLiteral("Detune");
    case 4: return QStringLiteral("Amplitude Transient");
    case 5: return QStringLiteral("Frequency Transient");
    case 6: return QStringLiteral("Wave Type");
    case 7: return QStringLiteral("Phase Modulation");
    default: return QStringLiteral("Unknown Modifier");
    }
}

// Fields: magnitude, rate, width, spread, direction, velocity,
// partial-result string.
inline bool fieldEnabled(int modifierType, int field, bool applyByPartial)
{
    static constexpr bool fields[8][7] = {
        /* TREMOLO   */ { true,  true,  false, false, false, false, false },
        /* VIBRATO   */ { true,  true,  false, false, false, false, false },
        /* GLISSANDO */ { true,  false, false, false, false, false, false },
        /* DETUNE    */ { false, false, false, true,  true,  true,  false },
        /* AMPTRANS  */ { true,  true,  true,  false, false, false, false },
        /* FREQTRANS */ { true,  true,  true,  false, false, false, false },
        /* WAVE_TYPE */ { true,  false, false, false, false, false, false },
        /* PHASE_MOD */ { true,  true,  false, false, false, false, false },
    };
    if (modifierType < 0 || modifierType >= 8 || field < 0 || field >= fieldCount)
        return false;
    // In PARTIAL mode CMOD reads effect parameters exclusively from
    // PartialResultString for every modifier type. Leaving top-level controls
    // enabled would make edits appear effective even though CMOD ignores them.
    if (applyByPartial)
        return field == 6;
    return field == 6 ? false : fields[modifierType][field];
}

} // namespace ModifierUiPolicy

#endif // MODIFIERUIPOLICY_HPP
