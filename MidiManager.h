#ifndef MIDI_MANAGER_H
#define MIDI_MANAGER_H

#include "Config.h"
#include <Arduino.h>
#include "esp32-hal-tinyusb.h"

// Definiciones de tipos de mensajes MIDI
#define MIDI_TYPE_CC          0
#define MIDI_TYPE_NOTE_ON     1
#define MIDI_TYPE_NOTE_OFF    2
#define MIDI_TYPE_PITCH_BEND  3
#define MIDI_TYPE_SYSEX       4
#define MIDI_TYPE_REALTIME    5

#define SYSEX_COLOR_UPDATE    0x01
#define SYSEX_VALUE_UPDATE    0x02
#define SYSEX_NAME_UPDATE     0x03
#define SYSEX_VU_UPDATE       0x04
#define SYSEX_TRANSPORT       0x05

struct MidiMessage {
    uint8_t type;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
};

class MidiManager {
private:
    int midiOutHead = 0;
    int midiOutTail = 0;
    MidiMessage midiOutBuffer[MIDI_BUFFER_SIZE];
    uint8_t sysExOutBuffer[SYSEX_BUFFER_SIZE];
    
    MtcData currentMtc;
    TransportState currentTransport;
    uint8_t currentMidiChannel;
    bool mtcSync;
    
    uint8_t mtcQuarterFrame;
    uint32_t lastMtcTime;
    bool mtcTimebaseValid;
    
    uint32_t midiMessagesReceived;
    uint32_t midiMessagesSent;
    uint32_t sysExMessagesProcessed;
    uint32_t mtcFramesReceived;
    uint32_t errorCount;
    
    bool midiThruEnabled;
    bool sysExAutoResponse;
    unsigned long lastActivityTime;
    
    void processSysExMessage(const uint8_t* data, uint16_t length);
    void processStudioOneMessage(const uint8_t* data, uint16_t length);
    void processColorUpdate(uint8_t track, uint8_t bank, const uint8_t* colorData);
    void processValueUpdate(uint8_t track, uint8_t bank, uint8_t value);
    void processVUUpdate(uint8_t track, uint8_t level);
    void processNameUpdate(uint8_t track, uint8_t bank, const char* name);
    void processTransportState(uint8_t state);
    
    void updateMtcFromQuarterFrame(uint8_t data);
    void reconstructMtcTime();
    bool validateMtcData();
    
    uint16_t rgb24ToRgb565(uint32_t rgb24);
    uint32_t rgb565ToRgb24(uint16_t rgb565);
    void logMidiError(const char* error);
    bool enqueueMidiMessage(uint8_t type, uint8_t channel, uint8_t data1, uint8_t data2);
    bool enqueueSysExMessage(const uint8_t* data, uint16_t length);
    void processMidiMessage(uint8_t status, uint8_t data1, uint8_t data2);
    void processSystemMessage(uint8_t status, uint8_t data1, uint8_t data2);
    void processRealTimeMessage(uint8_t status);

public:
    MidiManager();
    ~MidiManager();
    
    static MidiManager* getInstance() { return instance; }

    bool initialize(uint8_t midiChannel = MIDI_CHANNEL_DEFAULT);
    void setMidiChannel(uint8_t channel);
    uint8_t getMidiChannel() const { return currentMidiChannel; }
    
    void processMidiInput();
    void processMidiOutput();
    void processUsbMidiPacket(const uint8_t* packet);

    void sendControlChange(uint8_t channel, uint8_t cc, uint8_t value);
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendPitchBend(uint8_t channel, int16_t value);
    void sendTransportCommand(uint8_t command);
    void sendJogWheel(int8_t direction);
    void sendAllNotesOff(uint8_t channel);
    
    void sendStudioOneColorRequest(uint8_t track, uint8_t bank);
    void sendStudioOneValueRequest(uint8_t track, uint8_t bank);
    void sendCustomSysEx(const uint8_t* data, uint16_t length);
    
    const MtcData& getMtcData() const { return currentMtc; }
    const TransportState& getTransportState() const { return currentTransport; }
    bool isMtcSynced() const { return mtcSync && mtcTimebaseValid; }
    
    void enableMtcSync(bool enable) { mtcSync = enable; }
    bool isMtcSyncEnabled() const { return mtcSync; }
    void resetMtcTimebase();
    
    void printMidiStatistics() const;
    void resetStatistics();
    uint32_t getMessagesReceived() const { return midiMessagesReceived; }
    uint32_t getMessagesSent() const { return midiMessagesSent; }
    uint32_t getErrorCount() const { return errorCount; }
    
    bool testMidiConnection();
    void sendTestSequence();
    
    void setMidiThru(bool enable);
    void setSysExAutoResponse(bool enable);
    void incrementMessageCount() { midiMessagesReceived++; }
    void updateLastActivityTime() { lastActivityTime = millis(); }
    
private:
    static MidiManager* instance;
    
    //void processSystemMessage(uint8_t status, uint8_t data1, uint8_t data2);
    bool isValidMidiChannel(uint8_t channel) const;
    bool isValidControlNumber(uint8_t cc) const;
    bool isValidNoteNumber(uint8_t note) const;
};

#endif // MIDI_MANAGER_H