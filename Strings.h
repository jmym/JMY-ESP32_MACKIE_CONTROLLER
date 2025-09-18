#ifndef STRINGS_H
#define STRINGS_H

#include <Arduino.h>

namespace Strings {
const char str_volume[] = "Volumen";
const char str_pan[] = "Pan";
const char str_mute[] = "Mute";
const char str_solo[] = "Solo";
const char str_track[] = "Track";
const char str_bank[] = "Banco";
const char str_time[] = "Tiempo";

const char str_encoders[] = "Configurar Encoders";
const char str_display[] = "Configurar Pantalla";
const char str_midi[] = "Configurar MIDI";
const char str_global[] = "Ajustes Globales";
const char str_system_test[] = "Test Sistema";
const char str_exit[] = "Salir";

const char str_loading[] = "Cargando...";
const char str_error[] = "Error";
const char str_success[] = "Exito";
const char str_warning[] = "Advertencia";
}

class StringUtils {
public:
    static void readString(const char* str, char* buffer, size_t bufferSize) {
        strncpy(buffer, str, bufferSize);
        buffer[bufferSize - 1] = '\0';
    }

    static void printString(const char* str) {
        Serial.print(str);
    }
};

#endif