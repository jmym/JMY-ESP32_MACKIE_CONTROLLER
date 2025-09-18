#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include "Config.h"
#include <Arduino.h>

class EncoderManager {
private:
    EncoderConfig encoderBanks[NUM_ENCODERS][NUM_BANKS];
    bool encoderAccelerationEnabled;
    uint8_t currentBank;
    
public:
    EncoderManager();
    ~EncoderManager();
    
    bool initialize(AppConfig* config);
    void setCurrentBank(uint8_t bank);
    
    EncoderConfig getEncoderConfig(uint8_t index, uint8_t bank);
    EncoderConfig& getEncoderConfigMutable(uint8_t index, uint8_t bank);
    EncoderConfig (*getEncoderBanks())[NUM_BANKS];
    
    void processEncoderChange(uint8_t encoderIndex, int8_t change, uint8_t bank);
    void processSwitchPress(uint8_t switchIndex, uint8_t bank);
    
    void syncFromDAW(uint8_t track, uint8_t bank, uint8_t value, uint16_t color);
    void setEncoderDAWValue(uint8_t track, uint8_t bank, uint8_t value);
    uint8_t getEncoderDAWValue(uint8_t track, uint8_t bank);
    void syncNameFromDAW(uint8_t track, uint8_t bank, const char* name);
    
    void resetEncoderConfig(uint8_t index, uint8_t bank);
    void resetAllBanks();
    
    bool getEncoderAcceleration() const;
    void setEncoderAcceleration(bool enabled);

    void updateVULevel(uint8_t track, uint8_t level);
    void updateTrackName(uint8_t track, uint8_t bank, const char* name);
    
     void updateFromDAW(uint8_t track, uint8_t bank, uint8_t value, uint16_t color);
    void updateFromDAW(uint8_t track, uint8_t bank, uint8_t value);
    void updateFromDAW(uint8_t track, uint8_t bank, uint16_t color);
};

#endif // ENCODER_MANAGER_H