#ifndef MARBLE_TRACK_SONG_CONSTANTS_H
#define MARBLE_TRACK_SONG_CONSTANTS_H

#include <cstdint>

// Voice sound are created here:
// https://luvvoice.com/ (Dutch/Belgium  Dena)

// Idea's for themes
// -----------------
// - Default
// - Circus
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

    const int BUTTON_CLICK = 4;
    const int BUTTON_DOWN = 5;
    const int BUTTON_UP = 6;

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
    // Reuzenwiel calibratie wordt gestart, dit kan even duren...
    const int WHEEL_CALIBRATION_START = 10;
    // Eikpunt van het reuzenwiel niet gevonden.
    const int WHEEL_ZERO_NOT_FOUND = 11;
    // Calibratie reuzenwiel succesvol beëindigd...
    const int WHEEL_CALIBRATION_DONE = 12;
    // Eikpunt van het reuzenwiel op onverwachte plaatst, wiel was geblokkeerd of hercalibratie nodig.
    const int WHEEL_ZERO_UNEXPECTED = 13;
    // Initializatie reuzenwiel mislukt
    const int WHEEL_INIT_FAILED = 14;

}

#endif // MARBLE_TRACK_SONG_CONSTANTS_H
