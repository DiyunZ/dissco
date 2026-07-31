#ifndef MODIFIERUIPOLICY_HPP
#define MODIFIERUIPOLICY_HPP

#include <QString>

namespace ModifierUiPolicy {

inline constexpr int fieldCount = 8;

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

// Fields: probability, magnitude, rate, width, spread, direction, velocity,
// partial-result string.
inline bool fieldEnabled(int modifierType, int field, bool applyByPartial)
{
    static constexpr bool fields[8][7] = {
        /* TREMOLO   */ { true, true,  true,  false, false, false, false },
        /* VIBRATO   */ { true, true,  true,  false, false, false, false },
        /* GLISSANDO */ { true, true,  false, false, false, false, false },
        /* DETUNE    */ { true, false, false, false, true,  true,  true  },
        /* AMPTRANS  */ { true, true,  true,  true,  false, false, false },
        /* FREQTRANS */ { true, true,  true,  true,  false, false, false },
        /* WAVE_TYPE */ { false, true, false, false, false, false, false },
        /* PHASE_MOD */ { true, true,  true,  false, false, false, false },
    };
    if (modifierType < 0 || modifierType >= 8 || field < 0 || field >= fieldCount)
        return false;
    // In PARTIAL mode CMOD reads these values exclusively from
    // PartialResultString. Leaving the top-level PM controls enabled would
    // make edits appear effective even though CMOD ignores them.
    if (modifierType == 7 && applyByPartial)
        return field == 7;
    return field == 7 ? applyByPartial : fields[modifierType][field];
}

} // namespace ModifierUiPolicy

#endif // MODIFIERUIPOLICY_HPP
