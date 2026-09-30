#ifndef MARBLE_TRACK_SONG_CONSTANTS_H
#define MARBLE_TRACK_SONG_CONSTANTS_H

#include <cstdint>
#include <vector>

// Voice sound are created here:
// https://luvvoice.com/ (Dutch/Belgium  Dena)
// Remark:
// Try to use WAV files instead of MP3,
// these are uncompressed and played without a smaller delay

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
// List of all songs: Name, ID, Description
#define SONG_DEFS(X)                                                                                                                                                                            \
    X(AUTO_MODE, 1, "Automatische modus actief")                                                                                                                                                \
    X(MAN_MODE, 2, "Manuele modus actief")                                                                                                                                                      \
    X(IDLE, 3, "Ik ben al 5 minuten niet gebruikt! Vergeet mij niet uit te schakelen")                                                                                                          \
    X(BUTTON_CLICK, 4, "")                                                                                                                                                                      \
    X(BUTTON_DOWN, 5, "")                                                                                                                                                                       \
    X(BUTTON_UP, 6, "")                                                                                                                                                                         \
    X(STARTUP_SOUND, 7, "")                                                                                                                                                                     \
    X(FART, 8, "")                                                                                                                                                                              \
    X(WHEEL_ZERO_NOT_FOUND, 10, "Reuzenwiel eikpunt niet gevonden. (Fout code 10)")                                                                                                             \
    X(WHEEL_CALIBRATION_START, 11, "Reuzenwiel calibratie wordt gestart, dit kan even duren...")                                                                                                \
    X(WHEEL_CALIBRATION_END, 12, "Reuzenwiel calibratie beëindigd...")                                                                                                                          \
    X(WHEEL_CALIBRATION_FIRST_ZERO_NOT_FOUND, 13, "Reuzenwiel eikpunt gevonden op een onverwachte plaatst, mogelijks was het reuzenwiel geblokkeerd of is hercalibratie nodig. (Fout code 13)") \
    X(WHEEL_CALIBRATION_SECOND_ZERO_NOT_FOUND, 14, "Reuzenwiel calibratie mislukt. (Foutcode 14) Het eikpunt is geen 2de keer gevonden.")                                                       \
    X(WHEEL_UNEXPECTED_ZERO_TRIGGER, 15, "Reuzenwiel eikpunt is op een onverwacht punt geactiveerd. (Fout code 15) Druk 8 seconden op de knop om een hercalibratie te starten.")                \
    X(WHEEL_GOTO_BREAKPOINT, 16, "Sound effect at long press")                                                                                                                                  \
    X(LIFT_INIT_ERROR, 17, "Lift initialisatie mislukt. (Fout code 17) Druk 8 seconden op de knop om een hercalibratie te starten.")                                                            \
    X(LIFT_NO_ZERO, 18, "Lift zero sensor niet gevonden (Fout code 18) Controleer of er een bal onder zit.")                                                                                    \
    X(LIFT_STOP, 19, "Elevator bell")                                                                                                                                                           \
    X(LIFT_POWER_UNLOAD, 20, "")                                                                                                                                                                \
    X(NOTIFICATION, 21, "")                                                                                                                                                                     \
    X(ERROR, 22, "")                                                                                                                                                                            \
    X(NO_NETWORK, 23, "Geen netwerk")                                                                                                                                                           \
    X(LIFT_INIT_BUSY, 24, "Lift initialisatie is bezig")                                                                                                                                        \
    X(LIFT_RESTART, 25, "Lift wordt opnieuw gestart")                                                                                                                                           \
    X(WHEEL_RESTART, 26, "Reuzenwiel wordt opnieuw gestart")                                                                                                                                    \
    X(LAUNCH, 27, "Bal lancering")                                                                                                                                                              \
    X(LAUNDER_MAX_2_BALLS, 28, "Maximum van 2 ballen bereikt")                                                                                                                                  \
    X(CONFIG_ERROR, 29, "Configuratie fout. Ga naar de website om het op te lossen: marble, streepje, track, punt, local")                                                                      \
    X(BATTERY_CRITICAL, 30, "Batterij is leeg. Zet het systeem uit en vervang de batterij")                                                                                                     \
    X(BATTERY_LOW, 31, "Batterij is bijna leeg, vervang de batterij")                                                                                                                           \
    X(LAUNCH_NOTIFICATION, 32, "Bal ligt klaar om gelanceerd te worden!")                                                                                                                       \
    X(SHUTDOWN_TEXT, 33, "Systeem wordt uitgeschakeld")                                                                                                                                         \
    X(SHUTDOWN, 34, "")                                                                                                                                                                         \
    X(LIFT_AUTO_MODE_START, 35, "Automatische liftmodus gestart")                                                                                                                               \
    X(LIFT_AUTO_MODE_END, 36, "Automatische liftmodus beëindigd")                                                                                                                               \
    X(LIFT_BALL_WAITING, 37, "Ball can be loaded in the lift")                                                                                                                                  \
    X(SPLITTER_ERROR, 38, "Splitter fout, herstart het systeem")                                                                                                                                \
    X(BUTTON_CLICK_2, 39, "")                                                                                                                                                                   \
    X(BUTTON_CLICK_3, 40, "")                                                                                                                                                                   \
    X(BUTTON_CLICK_4, 41, "")                                                                                                                                                                   \
    X(BUTTON_CLICK_5, 42, "")                                                                                                                                                                   \
    X(BUTTON_CLICK_6, 43, "")                                                                                                                                                                   \
    X(BUTTON_CLICK_7, 44, "")                                                                                                                                                                   \
    X(BUTTON_CLICK_8, 45, "")                                                                                                                                                                   \
    X(CLEAR_QUEUE, 46, "Clear the queue")

    /**
     * @brief Song ID Enum
     */
    enum Song : int
    {
#define X(name, val, desc) name = val,
        SONG_DEFS(X)
#undef X
    };

    /**
     * @brief Returns the name of the song constant
     */
    inline const char *getName(int songId)
    {
        switch (songId)
        {
#define X(name, val, desc) \
    case name:             \
        return #name;
            SONG_DEFS(X)
#undef X
        default:
            return "UNKNOWN";
        }
    }

    /**
     * @brief Returns the description of the song constant
     */
    inline const char *getDescription(int songId)
    {
        switch (songId)
        {
#define X(name, val, desc) \
    case name:             \
        return desc;
            SONG_DEFS(X)
#undef X
        default:
            return "";
        }
    }

    // Button sound functions (theme-aware in future)
    inline int getButtonClickSound(int count = 0)
    {
        if (count == 2)
            return BUTTON_CLICK_2;
        if (count == 3)
            return BUTTON_CLICK_3;
        if (count == 4)
            return BUTTON_CLICK_4;
        if (count == 5)
            return BUTTON_CLICK_5;
        if (count == 6)
            return BUTTON_CLICK_6;
        if (count == 7)
            return BUTTON_CLICK_7;
        if (count >= 8)
            return BUTTON_CLICK_8;

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

    inline std::vector<int> getButtonSounds()
    {
        return {
            BUTTON_DOWN,
            BUTTON_UP,
            BUTTON_CLICK,
            BUTTON_CLICK_2,
            BUTTON_CLICK_3,
            BUTTON_CLICK_4,
            BUTTON_CLICK_5,
            BUTTON_CLICK_6,
            BUTTON_CLICK_7,
            BUTTON_CLICK_8};
    }
}

#endif // MARBLE_TRACK_SONG_CONSTANTS_H
