#include "EncoderManager.h"
#include "MidiManager.h"
#include "DisplayManager.h"  // Add this include

extern MidiManager midiManager;
extern DisplayManager displayManager;  // Declare once at the top

EncoderManager::EncoderManager() 
    : encoderAccelerationEnabled(true), currentBank(0) 
{
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        for (int enc = 0; enc < NUM_ENCODERS; enc++) {
            encoderBanks[bank][enc] = EncoderConfig();
        }
    }
}

EncoderManager::~EncoderManager() {}

bool EncoderManager::initialize(AppConfig* config) {
    Serial.println(F("Inicializando gestor de encoders..."));
    encoderAccelerationEnabled = config->encoderAcceleration;
    currentBank = config->currentBank;
    return true;
}

void EncoderManager::setCurrentBank(uint8_t bank) {
    currentBank = bank;
}

EncoderConfig EncoderManager::getEncoderConfig(uint8_t index, uint8_t bank) {
    if (index < NUM_ENCODERS && bank < NUM_BANKS) {
        return encoderBanks[bank][index];
    }
    return EncoderConfig();
}

EncoderConfig& EncoderManager::getEncoderConfigMutable(uint8_t index, uint8_t bank) {
    static EncoderConfig dummy;
    if (index < NUM_ENCODERS && bank < NUM_BANKS) {
        return encoderBanks[bank][index];
    }
    return dummy;
}

EncoderConfig (*EncoderManager::getEncoderBanks())[NUM_BANKS] {
    return encoderBanks;
}

void EncoderManager::processEncoderChange(uint8_t encoderIndex, int8_t change, uint8_t bank) {
    if (encoderIndex >= NUM_ENCODERS || bank >= NUM_BANKS) return;
    
    EncoderConfig& config = encoderBanks[bank][encoderIndex];
    config.value = constrain(config.value + change, config.minValue, config.maxValue);
    
    switch (config.controlType) {
        case CT_CC:
            midiManager.sendControlChange(config.channel, config.control, config.value);
            break;
        case CT_NOTE:
            if (change > 0) {
                midiManager.sendNoteOn(config.channel, config.control, config.value);
            } else {
                midiManager.sendNoteOff(config.channel, config.control, 0);
            }
            break;
        case CT_PITCH:
            midiManager.sendPitchBend(config.channel, map(config.value, 0, 127, -8192, 8191));
            break;
    }
}

void EncoderManager::processSwitchPress(uint8_t switchIndex, uint8_t bank) {
    if (switchIndex < 8) {
        uint8_t track = switchIndex;
        if (track < NUM_ENCODERS && bank < NUM_BANKS) {
            encoderBanks[bank][track].isMute = !encoderBanks[bank][track].isMute;
            midiManager.sendControlChange(encoderBanks[bank][track].channel, 120 + track, encoderBanks[bank][track].isMute ? 127 : 0);
        }
    } else if (switchIndex < 16) {
        uint8_t track = switchIndex - 8;
        if (track < NUM_ENCODERS && bank < NUM_BANKS) {
            encoderBanks[bank][track].isSolo = !encoderBanks[bank][track].isSolo;
            midiManager.sendControlChange(encoderBanks[bank][track].channel, 110 + track, encoderBanks[bank][track].isSolo ? 127 : 0);
        }
    }
}

void EncoderManager::syncFromDAW(uint8_t track, uint8_t bank, uint8_t value, uint16_t color) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS) {
        encoderBanks[bank][track].dawValue = value;
        encoderBanks[bank][track].trackColor = color;
    }
}

void EncoderManager::setEncoderDAWValue(uint8_t track, uint8_t bank, uint8_t value) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS) {
        encoderBanks[bank][track].dawValue = value;
    }
}

// En EncoderManager.cpp
uint8_t EncoderManager::getEncoderDAWValue(uint8_t track, uint8_t bank) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS) {
        return encoderBanks[bank][track].dawValue;
    }
    return 0;
}

// En EncoderManager.cpp
void EncoderManager::syncNameFromDAW(uint8_t track, uint8_t bank, const char* name) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS && name != nullptr) {
        strncpy(encoderBanks[bank][track].trackName, name, 5);
        encoderBanks[bank][track].trackName[4] = '\0';
        
        if (bank == currentBank) {
            extern DisplayManager displayManager;
            displayManager.markChannelDirty(track);
            
            Serial.print(F("DAW Name Update - Track: "));
            Serial.print(track);
            Serial.print(F(", Bank: "));
            Serial.print(bank);
            Serial.print(F(", Name: "));
            Serial.println(name);
        }
    }
}

void EncoderManager::resetEncoderConfig(uint8_t index, uint8_t bank) {
    if (index < NUM_ENCODERS && bank < NUM_BANKS) {
        encoderBanks[bank][index] = EncoderConfig();
    }
}

void EncoderManager::resetAllBanks() {
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        for (int enc = 0; enc < NUM_ENCODERS; enc++) {
            encoderBanks[bank][enc] = EncoderConfig();
        }
    }
}

bool EncoderManager::getEncoderAcceleration() const {
    return encoderAccelerationEnabled;
}

void EncoderManager::setEncoderAcceleration(bool enabled) {
    encoderAccelerationEnabled = enabled;
}

void EncoderManager::updateFromDAW(uint8_t track, uint8_t bank, uint8_t value, uint16_t color) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS) {
        EncoderConfig& config = encoderBanks[bank][track];
        
        // Actualizar valores del DAW
        config.dawValue = value;
        config.trackColor = color;
        
        // Si es el banco actual, sincronizar el valor físico del encoder
        if (bank == currentBank) {
            config.value = value; // Sincronizar el encoder físico con el valor del DAW
            
            // Forzar actualización visual de este canal
            extern DisplayManager displayManager;
            displayManager.markChannelDirty(track);
            
            // También actualizar el canal correspondiente de pan (si aplica)
            if (track < 8) {
                displayManager.markChannelDirty(track);
            }
        }
        
        Serial.print(F("DAW Update - Track: "));
        Serial.print(track);
        Serial.print(F(", Bank: "));
        Serial.print(bank);
        Serial.print(F(", Value: "));
        Serial.print(value);
        Serial.print(F(", Color: 0x"));
        Serial.println(color, HEX);
    }
}

// Función sobrecargada para actualizar solo el valor
void EncoderManager::updateFromDAW(uint8_t track, uint8_t bank, uint8_t value) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS) {
        EncoderConfig& config = encoderBanks[bank][track];
        config.dawValue = value;
        
        if (bank == currentBank) {
            config.value = value;
            
            extern DisplayManager displayManager;
            displayManager.markChannelDirty(track);
            
            Serial.print(F("DAW Value Update - Track: "));
            Serial.print(track);
            Serial.print(F(", Bank: "));
            Serial.print(bank);
            Serial.print(F(", Value: "));
            Serial.println(value);
        }
    }
}

// Función sobrecargada para actualizar solo el color
void EncoderManager::updateFromDAW(uint8_t track, uint8_t bank, uint16_t color) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS) {
        EncoderConfig& config = encoderBanks[bank][track];
        config.trackColor = color;
        
        if (bank == currentBank) {
            extern DisplayManager displayManager;
            displayManager.markChannelDirty(track);
            
            Serial.print(F("DAW Color Update - Track: "));
            Serial.print(track);
            Serial.print(F(", Bank: "));
            Serial.print(bank);
            Serial.print(F(", Color: 0x"));
            Serial.println(color, HEX);
        }
    }
}

// Añade esta función a EncoderManager.cpp
void EncoderManager::updateVULevel(uint8_t track, uint8_t level) {
    if (track < NUM_ENCODERS) {
        // Buscar el displayManager externo
        extern DisplayManager displayManager;
        
        // Actualizar el VU meter en el display
        displayManager.setVULevel(track, level);
        
        // También podrías almacenar el valor localmente si es necesario
        // Por ejemplo, para promediar o suavizar los valores
        // vuLevels[track] = level;
    }
}

void EncoderManager::updateTrackName(uint8_t track, uint8_t bank, const char* name) {
    if (track < NUM_ENCODERS && bank < NUM_BANKS && name != nullptr) {
        strncpy(encoderBanks[bank][track].trackName, name, 4);
        encoderBanks[bank][track].trackName[3] = '\0';
    }
}
