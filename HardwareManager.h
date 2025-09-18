#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include "Config.h"
#include <Wire.h>
#include <Adafruit_MCP23X17.h>

class HardwareManager {
private:
  Adafruit_MCP23X17 mcpEncodersVol;
  Adafruit_MCP23X17 mcpEncodersPan;
  Adafruit_MCP23X17 mcpSwitches;
  Adafruit_MCP23X17 mcpButtonsEnc;
  
  uint16_t lastMCP3State;
  uint16_t lastMCP4State;
  
  uint8_t lastEncoded[NUM_ENCODERS];
  int16_t encoderValue[NUM_ENCODERS];
  uint16_t lastEncoderTime[NUM_ENCODERS];
  
  int8_t lastEncodedNav;
  int32_t encoderValueNav;
  unsigned long lastEncoderTimeNav;
  unsigned long lastNavPressTime;
  bool navButtonState;
  bool lastNavButtonState;
  
  bool initializeMCP(Adafruit_MCP23X17& mcp, uint8_t address, const char* name);
  void configureEncoderMCP(Adafruit_MCP23X17& mcp, uint8_t intPinA, uint8_t intPinB);
  void configureSwitchMCP(Adafruit_MCP23X17& mcp);
  
  void processEncoder(int mcpIndex, int encoderIndex, Adafruit_MCP23X17& mcp);
  int8_t calculateEncoderChange(int8_t encoded, int8_t lastEncoded);
  uint8_t calculateAcceleration(unsigned long timeDiff);
  
  bool validateMCPResponse(Adafruit_MCP23X17& mcp, const char* name);
  bool validateMCPResponseConst(const Adafruit_MCP23X17& mcp, const char* name) const;
  void handleMCPError(const char* mcpName, const char* operation);

public:
  HardwareManager();
  ~HardwareManager();
  
  bool initialize();
  void setupInterrupts();
  
  void processMCP1AEncoders();
  void processMCP1BEncoders();  
  void processMCP2AEncoders();
  void processMCP2BEncoders();
  
  void pollSwitchesAndButtons();
  void readNavigationEncoder();
  
  bool checkMCPHealth();
  void resetMCPs();
  void clearAllInterrupts();
  
  bool testAllMCPs();
  void calibrateEncoders();
  
  void printDiagnostics() const;
};

#endif // HARDWARE_MANAGER_H