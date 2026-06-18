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
    const int NOTIFICATION = 21;
    const int ERROR = 22;
    const int NO_NETWORK = 23;

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
    // Reuzenwiel eikpunt niet gevonden. (Fout code 10)
    const int WHEEL_ZERO_NOT_FOUND = 10;
    // Reuzenwiel calibratie wordt gestart, dit kan even duren...
    const int WHEEL_CALIBRATION_START = 11;
    // Reuzenwiel calibratie beëindigd...
    const int WHEEL_CALIBRATION_END = 12;
    // Reuzenwiel eikpunt gevonden op een onverwachte plaatst, mogelijks was het reuzenwiel geblokkeerd of is hercalibratie nodig. (Fout code 13)
    const int WHEEL_CALIBRATION_FIRST_ZERO_NOT_FOUND = 13;
    // Reuzenwiel calibratie mislukt. (Foutcode 14)
    // Het eikpunt is geen 2de keer gevonden.
    // Mogelijks is de 'Max steps per revolutions' configuratie te laag of heeft het reuzenwiel door geslipt.
    const int WHEEL_CALIBRATION_SECOND_ZERO_NOT_FOUND = 14;
    // Reuzenwiel eikpunt is op een onverwacht punt geactiveerd. (Fout code 15)
    // Druk 8 seconden op de knop om een hercalibratie te starten.
    const int WHEEL_UNEXPECTED_ZERO_TRIGGER = 15;
    // Sound effect at long press
    const int WHEEL_GOTO_BREAKPOINT = 16;

    // Lift initialisatie mislukt. (Fout code 17)
    // Druk 8 seconden op de knop om een hercalibratie te starten.
    const int LIFT_INIT_ERROR = 17;

    // Lift zero sensor niet gevonden (Fout code 18)
    // Controleer of er een bal onder zit.
    // Druk 8 seconden op de knop om een hercalibratie te starten.
    const int LIFT_NO_ZERO = 18;

    // Elevator bell
    const int LIFT_STOP = 19;
    const int LIFT_POWER_UNLOAD = 20;
    const int LIFT_INIT_BUSY = 24;

    // Lift wordt opnieuw gestart
    const int LIFT_RESTART = 25;
    // Reuzenwiel wordt opnieuw gestart
    const int WHEEL_RESTART = 26;
    const int LAUNCH = 27;
    // Maximum van 2 ballen bereikt
    const int LAUNDER_MAX_2_BALLS = 28;
    // Configuratie fout.
    // Ga naar de website om het op te lossen:
    // marble, streepje, track, punt, local
    const int CONFIG_ERROR = 29;

    // Batterij is leeg. Zet het systeem uit en vervang de batterij
    const int BATTERY_CRITICAL = 30;
    // Batterij is bijna leeg, vervang de batterij
    const int BATTERY_LOW = 31;
}

#endif // MARBLE_TRACK_SONG_CONSTANTS_H
