#ifndef MARBLE_TRACK_SONG_CONSTANTS_H
#define MARBLE_TRACK_SONG_CONSTANTS_H

#include <cstdint>

// Voice sound are created here:
// https://luvvoice.com/ (Dutch/Belgium  Dena)

// Idea's for themes
// -----------------
// - Default
// - Circus
// - Kermis
// - Space
// - Animals
// - 8-bit games
// - Farts

namespace songs
{

    // GENERAL
    //---------
    // Automatische modus actief
    const int AUTO_MODE = 1;
    // Manuele modus actief
    const int MAN_MODE = 2;
    // Ik ben al 5 minuten niet gebruikt!
    // Vergeet mij niet uit te schakelen
    const int IDLE = 3;
    const int STARTUP_SOUND = 7;

    const int BUTTON_CLICK = 4;
    const int BUTTON_DOWN = 5;
    const int BUTTON_UP = 6;

    const int FART = 8;

    // Button sound functions (theme-aware in future)
    inline int getButtonClickSound()
    {
        return BUTTON_CLICK;
    }

    inline int getButtonDownSound()
    {
        return BUTTON_DOWN;
    }

    inline int getButtonUpSound()
    {
        return BUTTON_UP;
    }

    // WHEEL
    //------
    // Reuzenwiel eikpunt niet gevonden, mogelijk slipt het reuzenwiel door, defect aan de zero sensor of hercalibratie nodig....
    const int WHEEL_ZERO_NOT_FOUND = 10;
    // Reuzenwiel calibratie wordt gestart, dit kan even duren...
    const int WHEEL_CALIBRATION_START = 11;
    // Calibratie reuzenwiel beëindigd...
    const int WHEEL_CALIBRATION_END = 12;
    // Reuzenwiel calibratie gefaald. Eikpunt van het reuzenwiel was niet gevonden. Mogelijke oorzaken: 'Max steps per revolutions' configuratie te laag, reuzenwiel slipt door of zero sensor defect.
    const int WHEEL_CALIBRATION_FIRST_ZERO_NOT_FOUND = 13;
    // Reuzenwiel calibratie gefaald. Tweede eikpunt van het reuzenwiel was niet gevonden tijdens calibratie. Mogelijke oorzaken: 'Max steps per revolutions' configuratie te laag of reuzenwiel slipt door.
    const int WHEEL_CALIBRATION_SECOND_ZERO_NOT_FOUND = 14;
    // Eikpunt van reuzewiel is onverwacht geactiveerd. Mogelijke oorzaken: reuzenwiel slipt door of hercalibratie nodig.
    const int WHEEL_UNEXPECTED_ZERO_TRIGGER = 15;
    // Sound effect at long press
    const int WHEEL_GOTO_BREAKPOINT = 16;

    // Lift initialisatie mislukt.
    // Enkele mogelijk oorzaken:
    // 1) Als de lift niet beweegt, controleer dan of de stepper motor werkt
    // 2) Als de lift bewoog maar stopte voor ze beneden was, dan is configuratie 'Max steps' te laag.
    // 3) Als de lift niet stopte onderaan, controleer dan de sensor door die manueel in te drukken
    const int LIFT_INIT_ERROR = 17;

}

#endif // MARBLE_TRACK_SONG_CONSTANTS_H
