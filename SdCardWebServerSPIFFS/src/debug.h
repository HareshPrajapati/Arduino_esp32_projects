#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <Arduino.h>
#include <string.h>


#define DEBUG_ENABLE                (1)
#define ENABLE_DEBUG_MAIN           (1)
#define ENABLE_DEBUG_SERVER         (1)
#define ENABLE_DEBUG_SD_CARD        (1)
#define ENABLE_DEBUG_GUI            (1)
#define ENABLE_DEBUG_SPIFFS         (1)
#define ENABLE_DEBUG_HEXPARSER      (1)
#define ENABLE_DEBUG_UPDATER        (1)

#if defined(DEBUG_ENABLE) && (DEBUG_ENABLE == 1)

    const unsigned long DEBUG_BAUDRATE = 921600UL;
    #define DEBUG_SERIAL             Serial
    #define DEBUG_INIT()             DEBUG_SERIAL.begin(DEBUG_BAUDRATE)
    #define DEBUG_SET_OUTPUT()       DEBUG_SERIAL.setDebugOutput(true);
    #define DEBUG_WAIT()             while(!DEBUG_SERIAL)

    #define DEBUG(HEADER, ...)      DEBUG_SERIAL.print(HEADER);             \
                                    DEBUG_SERIAL.print(__LINE__);           \
                                    DEBUG_SERIAL.print(":");                \
                                    DEBUG_SERIAL.printf(__VA_ARGS__)

    #define DEBUG_NL(HEADER, ...)   DEBUG_SERIAL.print(HEADER);             \
                                    DEBUG_SERIAL.print(__LINE__);           \
                                    DEBUG_SERIAL.print(":");                \
                                    DEBUG_SERIAL.printf(__VA_ARGS__);       \
                                    DEBUG_SERIAL.println()
    #else
    #define DEBUG_INIT()             (void)0
    #define DEBUG_SET_OUTPUT()       (void)0
    #define DEBUG_WAIT()             (void)0
    #define DEBUG(...)               (void)0
    #define DEBUG_NL(...)            (void)0

#endif //defined(DEBUG_ENABLE) && (DEBUG_ENABLE == 1)

#if defined(ENABLE_DEBUG_MAIN) && (ENABLE_DEBUG_MAIN == 1)
    #define DEBUG_MAIN(...)          DEBUG("[MAIN]:", __VA_ARGS__)
    #define DEBUG_MAIN_NL(...)       DEBUG_NL("[MAIN]:", __VA_ARGS__)
#else
    #define DEBUG_MAIN(...)          (void)0
    #define DEBUG_MAIN_NL(...)       (void)0

#endif // defined(ENABLE_DEBUG_MAIN) && (ENABLE_DEBUG_MAIN == 1)

#if defined(ENABLE_DEBUG_SERVER) && (ENABLE_DEBUG_SERVER == 1)
    #define DEBUG_SERVER(...)         DEBUG("[SERVER]:", __VA_ARGS__)
    #define DEBUG_SERVER_NL(...)      DEBUG_NL("[SERVER]:", __VA_ARGS__)
#else
    #define DEBUG_SERVER(...)         (void)0
    #define DEBUG_SERVER_NL(...)      (void)0

#endif // defined(ENABLE_DEBUG_SERVER) && (ENABLE_DEBUG_SERVER == 1)

#if defined(ENABLE_DEBUG_SD_CARD) && (ENABLE_DEBUG_SD_CARD == 1)
    #define DEBUG_SD_CARD(...)        DEBUG("[SD_CARD]:", __VA_ARGS__)
    #define DEBUG_SD_CARD_NL(...)     DEBUG_NL("[SD_CARD]:", __VA_ARGS__)
#else
    #define DEBUG_SD_CARD(...)        (void)0
    #define DEBUG_SD_CARD_NL(...)     (void)0

#endif // defined(ENABLE_DEBUG_SD_CARD) && (ENABLE_DEBUG_SD_CARD == 1)

#if defined(ENABLE_DEBUG_SD_CARD) && (ENABLE_DEBUG_SD_CARD == 1)
    #define DEBUG_GUI(...)            DEBUG("[GUI]:", __VA_ARGS__)
    #define DEBUG_GUI_NL(...)         DEBUG_NL("[GUI]:", __VA_ARGS__)
#else
    #define DEBUG_GUI(...)            (void)0
    #define DEBUG_GUI_NL(...)         (void)0

#endif // defined(ENABLE_DEBUG_GUI) && (ENABLE_DEBUG_GUI == 1)

#if defined(ENABLE_DEBUG_SPIFFS) && (ENABLE_DEBUG_SPIFFS == 1)
    #define DEBUG_SPIFFS(...)          DEBUG("[SPIFFS]:",__VA_ARGS__)
    #define DEBUG_SPIFFS_NL(...)       DEBUG_NL("[SPIFFS]:",__VA_ARGS__)
#else
    #define DEBUG_SPIFFS(...)          (void)0
    #define DEBUG_SPIFFS_NL(...)       (void)0

#endif // defined(ENABLE_DEBUG_SPIFFS) && (ENABLE_DEBUG_SPIFFS == 1)

#if defined(ENABLE_DEBUG_HEXPARSER) && (ENABLE_DEBUG_HEXPARSER == 1)
    #define DEBUG_HEXPARSER(...)       DEBUG("[HEXPARSER]:",__VA_ARGS__)
    #define DEBUG_HEXPARSER_NL(...)    DEBUG_NL("[HEXPARSER]:",__VA_ARGS__)
#else
    #define DEBUG_HEXPARSER(...)       (void)0
    #define DEBUG_HEXPARSER(...)       (void)0

#endif // defined(ENABLE_DEBUG_HEXPARSER) && (ENABLE_DEBUG_HEXPARSER == 1)

#if defined(ENABLE_DEBUG_UPDATER) && (ENABLE_DEBUG_UPDATER == 1)
    #define DEBUG_UPDATER(...)       DEBUG("[UPDATER]:",__VA_ARGS__)
    #define DEBUG_UPDATER_NL(...)    DEBUG_NL("[UPDATER]:",__VA_ARGS__)
#else
    #define DEBUG_UPDATER(...)       (void)0
    #define DEBUG_UPDATER(...)       (void)0

#endif // defined(ENABLE_DEBUG_UPDATER) && (ENABLE_DEBUG_UPDATER == 1)


#endif // _DEBUG_H_