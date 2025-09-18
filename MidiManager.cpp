#include "MidiManager.h"
#include "Config.h"
#include <USB.h>
#include "EncoderManager.h"  // Add this include

// Inicializar la instancia estática
MidiManager* MidiManager::instance = nullptr;
extern EncoderManager encoderManager;  // Add this declaration
extern void syncEncoderFromDAW(uint8_t track, uint8_t bank, uint8_t value, uint16_t color);


MidiManager::MidiManager()
  : currentMidiChannel(MIDI_CHANNEL_DEFAULT), mtcSync(true),
    mtcQuarterFrame(0), lastMtcTime(0), mtcTimebaseValid(false),
    midiMessagesReceived(0), midiMessagesSent(0),
    sysExMessagesProcessed(0), mtcFramesReceived(0), errorCount(0),
    midiThruEnabled(false), sysExAutoResponse(true), lastActivityTime(0)
{
  instance = this;
  memset(&currentMtc, 0, sizeof(currentMtc));
  memset(&currentTransport, 0, sizeof(currentTransport));
}

MidiManager::~MidiManager() {
  instance = nullptr;
}

bool MidiManager::initialize(uint8_t midiChannel) {
  Serial.println(F("Inicializando controlador MIDI USB..."));
  
  currentMidiChannel = midiChannel;
  USB.begin();
  
  midiOutHead = 0;
  midiOutTail = 0;
  
  Serial.println(F("Controlador MIDI USB inicializado"));
  return true;
}

void MidiManager::setMidiChannel(uint8_t channel) {
  if (channel >= 1 && channel <= 16) {
    currentMidiChannel = channel;
  }
}

void MidiManager::processMidiInput() {
  // Process USB MIDI input
  if (tud_midi_available()) {
    uint8_t packet[4];
    while (tud_midi_available()) {
      if (tud_midi_packet_read(packet)) {
        incrementMessageCount();
        updateLastActivityTime();
        processUsbMidiPacket(packet);
      }
    }
  }
}

void MidiManager::processMidiOutput() {
  if (midiOutHead == midiOutTail) return;
  
  MidiMessage msg = midiOutBuffer[midiOutTail];
  uint8_t midiData[3];
  
  switch (msg.type) {
    case MIDI_TYPE_CC:
      midiData[0] = 0xB0 | (msg.channel - 1);
      midiData[1] = msg.data1;
      midiData[2] = msg.data2;
      tud_midi_stream_write(0, midiData, 3);
      break;
      
    case MIDI_TYPE_NOTE_ON:
      midiData[0] = 0x90 | (msg.channel - 1);
      midiData[1] = msg.data1;
      midiData[2] = msg.data2;
      tud_midi_stream_write(0, midiData, 3);
      break;
      
    case MIDI_TYPE_NOTE_OFF:
      midiData[0] = 0x80 | (msg.channel - 1);
      midiData[1] = msg.data1;
      midiData[2] = msg.data2;
      tud_midi_stream_write(0, midiData, 3);
      break;
      
    case MIDI_TYPE_PITCH_BEND:
      {
        uint8_t lsb = msg.data1;
        uint8_t msb = msg.data2;
        midiData[0] = 0xE0 | (msg.channel - 1);
        midiData[1] = lsb;
        midiData[2] = msb;
        tud_midi_stream_write(0, midiData, 3);
      }
      break;
      
    case MIDI_TYPE_SYSEX:
      tud_midi_stream_write(0, sysExOutBuffer, msg.data1);
      break;
      
    case MIDI_TYPE_REALTIME:
      tud_midi_stream_write(0, &msg.data1, 1);
      break;
  }
  
  midiOutTail = (midiOutTail + 1) % MIDI_BUFFER_SIZE;
  midiMessagesSent++;
  lastActivityTime = millis();
}

bool MidiManager::enqueueMidiMessage(uint8_t type, uint8_t channel, uint8_t data1, uint8_t data2) {
  uint8_t nextHead = (midiOutHead + 1) % MIDI_BUFFER_SIZE;
  
  if (nextHead == midiOutTail) {
    errorCount++;
    return false;
  }
  
  midiOutBuffer[midiOutHead].type = type;
  midiOutBuffer[midiOutHead].channel = channel;
  midiOutBuffer[midiOutHead].data1 = data1;
  midiOutBuffer[midiOutHead].data2 = data2;
  
  midiOutHead = nextHead;
  return true;
}

bool MidiManager::enqueueSysExMessage(const uint8_t* data, uint16_t length) {
  if (length > SYSEX_BUFFER_SIZE) {
    errorCount++;
    return false;
  }
  
  uint8_t nextHead = (midiOutHead + 1) % MIDI_BUFFER_SIZE;
  
  if (nextHead == midiOutTail) {
    errorCount++;
    return false;
  }
  
  memcpy(sysExOutBuffer, data, length);
  midiOutBuffer[midiOutHead] = {MIDI_TYPE_SYSEX, 0, length, 0};
  midiOutHead = nextHead;
  return true;
}

void MidiManager::sendControlChange(uint8_t channel, uint8_t cc, uint8_t value) {
  if (!isValidMidiChannel(channel) || !isValidControlNumber(cc)) return;
  
  if (!enqueueMidiMessage(MIDI_TYPE_CC, channel, cc, value)) {
    logMidiError("Buffer MIDI lleno");
  }
}

void MidiManager::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (!isValidMidiChannel(channel) || !isValidNoteNumber(note)) return;
  
  if (!enqueueMidiMessage(MIDI_TYPE_NOTE_ON, channel, note, velocity)) {
    logMidiError("Buffer MIDI lleno");
  }
}

void MidiManager::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (!isValidMidiChannel(channel) || !isValidNoteNumber(note)) return;
  
  if (!enqueueMidiMessage(MIDI_TYPE_NOTE_OFF, channel, note, velocity)) {
    logMidiError("Buffer MIDI lleno");
  }
}

void MidiManager::sendPitchBend(uint8_t channel, int16_t value) {
  if (!isValidMidiChannel(channel)) return;
  
  uint8_t lsb = value & 0x7F;
  uint8_t msb = (value >> 7) & 0x7F;
  
  if (!enqueueMidiMessage(MIDI_TYPE_PITCH_BEND, channel, lsb, msb)) {
    logMidiError("Buffer MIDI lleno");
  }
}

void MidiManager::sendTransportCommand(uint8_t command) {
  if (!enqueueMidiMessage(MIDI_TYPE_REALTIME, 0, command, 0)) {
    logMidiError("Buffer MIDI lleno");
  }
}

void MidiManager::sendJogWheel(int8_t direction) {
  uint8_t cc = (direction > 0) ? MIDI_JOG_FORWARD : MIDI_JOG_BACKWARD;
  sendControlChange(currentMidiChannel, cc, abs(direction));
}

void MidiManager::sendAllNotesOff(uint8_t channel) {
  if (!isValidMidiChannel(channel)) return;
  sendControlChange(channel, 123, 0);
}

void MidiManager::sendStudioOneColorRequest(uint8_t track, uint8_t bank) {
  uint8_t sysexData[] = {
    0xF0, 0x00, 0x21, 0x7B, 0x01, track, bank, 0xF7
  };
  
  if (!enqueueSysExMessage(sysexData, sizeof(sysexData))) {
    logMidiError("No se pudo enviar solicitud de color");
  }
}

void MidiManager::sendStudioOneValueRequest(uint8_t track, uint8_t bank) {
  uint8_t sysexData[] = {
    0xF0, 0x00, 0x21, 0x7B, 0x02, track, bank, 0xF7
  };
  
  if (!enqueueSysExMessage(sysexData, sizeof(sysexData))) {
    logMidiError("No se pudo enviar solicitud de valor");
  }
}

void MidiManager::sendCustomSysEx(const uint8_t* data, uint16_t length) {
  if (!enqueueSysExMessage(data, length)) {
    logMidiError("No se pudo enviar SysEx personalizado");
  }
}

extern "C" void tud_midi_rx_cb(uint8_t itf) {
  MidiManager* manager = MidiManager::getInstance();
  if (manager) {
    uint8_t packet[4];
    while (tud_midi_available()) {
      if (tud_midi_packet_read(packet)) {
        manager->incrementMessageCount();
        manager->updateLastActivityTime();
        manager->processUsbMidiPacket(packet);
      }
    }
  }
}

void MidiManager::processUsbMidiPacket(const uint8_t* packet) {
  if (!packet) return;
  
  uint8_t cableNumber = (packet[0] >> 4) & 0x0F;
  uint8_t codeIndexNumber = packet[0] & 0x0F;
  (void)cableNumber;
  
  switch (codeIndexNumber) {
    case 0x8: // Note Off
    case 0x9: // Note On
    case 0xA: // Poly Pressure
    case 0xB: // Control Change
    case 0xC: // Program Change
    case 0xD: // Channel Pressure
    case 0xE: // Pitch Bend
      processMidiMessage(packet[1], packet[2], packet[3]);
      break;
      
    case 0xF: // Single Byte (Real-time)
      if (packet[1] >= 0xF8) {
        processRealTimeMessage(packet[1]);
      }
      break;
      
    default:
      // Handle other message types if needed
      break;
  }
}

void MidiManager::processMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
  uint8_t messageType = (status >> 4) & 0x0F;
  uint8_t channel = (status & 0x0F) + 1;
  
  // Add your MIDI message processing logic here
  // This is a basic implementation
  Serial.print(F("MIDI Message - Type: "));
  Serial.print(messageType, HEX);
  Serial.print(F(", Ch: "));
  Serial.println(channel);
}

void MidiManager::processRealTimeMessage(uint8_t status) {
  switch (status) {
    case 0xF8: // Timing Clock
      break;
    case 0xFA: // Start
      currentTransport.isPlaying = true;
      currentTransport.isPaused = false;
      break;
    case 0xFB: // Continue
      currentTransport.isPlaying = true;
      currentTransport.isPaused = false;
      break;
    case 0xFC: // Stop
      currentTransport.isPlaying = false;
      currentTransport.isPaused = false;
      break;
  }
}

void MidiManager::updateMtcFromQuarterFrame(uint8_t data) {
  uint8_t pieceType = (data >> 4) & 0x07;
  uint8_t pieceValue = data & 0x0F;
  
  switch (pieceType) {
    case 0: currentMtc.frames = (currentMtc.frames & 0xF0) | pieceValue; break;
    case 1: currentMtc.frames = (currentMtc.frames & 0x0F) | (pieceValue << 4); break;
    case 2: currentMtc.seconds = (currentMtc.seconds & 0xF0) | pieceValue; break;
    case 3: currentMtc.seconds = (currentMtc.seconds & 0x0F) | (pieceValue << 4); break;
    case 4: currentMtc.minutes = (currentMtc.minutes & 0xF0) | pieceValue; break;
    case 5: currentMtc.minutes = (currentMtc.minutes & 0x0F) | (pieceValue << 4); break;
    case 6: currentMtc.hours = (currentMtc.hours & 0xF0) | pieceValue; break;
    case 7: 
      currentMtc.hours = (currentMtc.hours & 0x0F) | (pieceValue << 4);
      currentMtc.isRunning = (pieceValue & 0x06) != 0;
      break;
  }
  
  mtcQuarterFrame = pieceType;
  lastMtcTime = millis();
  
  if (pieceType == 7) {
    reconstructMtcTime();
  }
}

void MidiManager::reconstructMtcTime() {
  if (validateMtcData()) {
    mtcTimebaseValid = true;
  } else {
    mtcTimebaseValid = false;
    errorCount++;
  }
}

bool MidiManager::validateMtcData() {
  return (currentMtc.frames < 30 && 
          currentMtc.seconds < 60 && 
          currentMtc.minutes < 60 && 
          currentMtc.hours < 24);
}

uint16_t MidiManager::rgb24ToRgb565(uint32_t rgb24) {
  uint8_t r = (rgb24 >> 16) & 0xFF;
  uint8_t g = (rgb24 >> 8) & 0xFF;
  uint8_t b = rgb24 & 0xFF;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint32_t MidiManager::rgb565ToRgb24(uint16_t rgb565) {
  uint8_t r = (rgb565 >> 8) & 0xF8;
  uint8_t g = (rgb565 >> 3) & 0xFC;
  uint8_t b = (rgb565 << 3) & 0xF8;
  return (r << 16) | (g << 8) | b;
}

void MidiManager::resetMtcTimebase() {
  memset(&currentMtc, 0, sizeof(currentMtc));
  mtcQuarterFrame = 0;
  mtcTimebaseValid = false;
}

void MidiManager::printMidiStatistics() const {
  Serial.println(F("\n=== ESTADÍSTICAS MIDI ==="));
  Serial.print(F("Mensajes recibidos: ")); Serial.println(midiMessagesReceived);
  Serial.print(F("Mensajes enviados: ")); Serial.println(midiMessagesSent);
  Serial.print(F("Mensajes SysEx: ")); Serial.println(sysExMessagesProcessed);
  Serial.print(F("Frames MTC: ")); Serial.println(mtcFramesReceived);
  Serial.print(F("Errores: ")); Serial.println(errorCount);
  Serial.println(F("========================\n"));
}

void MidiManager::resetStatistics() {
  midiMessagesReceived = 0;
  midiMessagesSent = 0;
  sysExMessagesProcessed = 0;
  mtcFramesReceived = 0;
  errorCount = 0;
}

bool MidiManager::testMidiConnection() {
  sendControlChange(currentMidiChannel, 7, 64);
  return true;
}

void MidiManager::sendTestSequence() {
  for (int i = 0; i < 8; i++) {
    sendControlChange(currentMidiChannel, 7 + i, 0);
    sendControlChange(currentMidiChannel, 7 + i, 127);
    sendControlChange(currentMidiChannel, 7 + i, 64);
  }
}

void MidiManager::setMidiThru(bool enable) {
  midiThruEnabled = enable;
}

void MidiManager::setSysExAutoResponse(bool enable) {
  sysExAutoResponse = enable;
}

bool MidiManager::isValidMidiChannel(uint8_t channel) const {
  return (channel >= 1 && channel <= 16);
}

bool MidiManager::isValidControlNumber(uint8_t cc) const {
  return (cc <= 127);
}

bool MidiManager::isValidNoteNumber(uint8_t note) const {
  return (note <= 127);
}

void MidiManager::logMidiError(const char* error) {
  errorCount++;
  Serial.print(F("ERROR MIDI: "));
  Serial.println(error);
}

// Añade estas funciones a MidiManager.cpp

void MidiManager::processStudioOneMessage(const uint8_t* data, uint16_t length) {
    if (length < 8 || data[0] != 0xF0 || data[1] != 0x00 || data[2] != 0x21 || data[3] != 0x7B) {
        return;
    }

    uint8_t messageType = data[4];
    uint8_t track = data[5];
    uint8_t bank = data[6];

    switch (messageType) {
        case SYSEX_COLOR_UPDATE:
            if (length >= 11) {
                uint32_t rgb24 = (data[7] << 16) | (data[8] << 8) | data[9];
                uint16_t color = rgb24ToRgb565(rgb24);
                //syncEncoderFromDAW(track, bank, encoderManager.getEncoderDAWValue(track, bank), color);
                // Use separate functions instead of combined one
                syncEncoderColorFromDAW(track, bank, color);
            }
            break;
            
        case SYSEX_VALUE_UPDATE:
            if (length >= 9) {
                //syncEncoderFromDAW(track, bank, data[7], encoderManager.getEncoderConfig(track, bank).trackColor);
                syncEncoderValueFromDAW(track, bank, data[7]);
            }
            break;
            
        case SYSEX_VU_UPDATE:
            if (length >= 9) {
                updateVUMeterLevel(track, data[7]);
            }
            break;
            
        case SYSEX_NAME_UPDATE:
            if (length >= 9) {
                char name[6] = {0};
                uint8_t nameLength = min((uint8_t)(length - 8), (uint8_t)5);
                memcpy(name, &data[7], nameLength);
                syncEncoderNameFromDAW(track, bank, name);
            }
            break;
            
        case SYSEX_TRANSPORT:
            if (length >= 8) {
                processTransportState(data[5]);
            }
            break;
            
        case 0x06: // Mensaje combinado valor + color
            if (length >= 12) {
                uint8_t value = data[7];
                uint32_t rgb24 = (data[8] << 16) | (data[9] << 8) | data[10];
                uint16_t color = rgb24ToRgb565(rgb24);
                //syncEncoderFromDAW(track, bank, value, color);
                syncEncoderValueFromDAW(track, bank, value);
                syncEncoderColorFromDAW(track, bank, color);
            }
            break;
    }
    
    sysExMessagesProcessed++;
}

void MidiManager::processSystemMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    switch (status) {
        case 0xF1: // MTC Quarter Frame
            updateMtcFromQuarterFrame(data1);
            mtcFramesReceived++;
            break;
            
        case 0xF2: // Song Position Pointer
            // No implementado para Studio One
            break;
            
        case 0xF3: // Song Select
            // No implementado para Studio One
            break;
            
        case 0xF8: // Timing Clock
            currentTransport.isPlaying = true;
            break;
            
        case 0xFA: // Start
            currentTransport.isPlaying = true;
            currentTransport.isPaused = false;
            break;
            
        case 0xFB: // Continue
            currentTransport.isPlaying = true;
            currentTransport.isPaused = false;
            break;
            
        case 0xFC: // Stop
            currentTransport.isPlaying = false;
            currentTransport.isPaused = false;
            break;
            
        case 0xFE: // Active Sensing
            // Mantener conexión activa
            lastActivityTime = millis();
            break;
            
        case 0xFF: // System Reset
            resetMtcTimebase();
            memset(&currentTransport, 0, sizeof(currentTransport));
            break;
    }
}