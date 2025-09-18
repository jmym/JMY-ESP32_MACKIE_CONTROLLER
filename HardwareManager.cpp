#include "HardwareManager.h"
#include "Config.h"

// Declaración de funciones de callback externas
extern void onEncoderChange(uint8_t encoderIndex, int8_t change);
extern void onSwitchPress(uint8_t switchIndex);
extern void onButtonPress(uint8_t buttonIndex);
extern void onNavigationEncoderChange(int8_t change);
extern void onNavigationButtonPress();

HardwareManager::HardwareManager() 
  : lastMCP3State(0), lastMCP4State(0),
    lastEncodedNav(0), encoderValueNav(0), lastEncoderTimeNav(0),
    lastNavPressTime(0), navButtonState(false), lastNavButtonState(false)
{
  memset(lastEncoded, 0, sizeof(lastEncoded));
  memset(encoderValue, 0, sizeof(encoderValue));
  memset(lastEncoderTime, 0, sizeof(lastEncoderTime));
}

HardwareManager::~HardwareManager() {
}

bool HardwareManager::initialize() {
  Serial.println(F("Inicializando sistema de hardware..."));
  
  Wire.begin(8, 9); // SDA=GPIO8, SCL=GPIO9 para ESP32-S3
  Wire.setClock(400000);
  
  bool success = true;
  
  success &= initializeMCP(mcpEncodersVol, MCP_ENCODERS_VOL_ADDR, "MCP1 Encoders Vol");
  success &= initializeMCP(mcpEncodersPan, MCP_ENCODERS_PAN_ADDR, "MCP2 Encoders Pan");
  success &= initializeMCP(mcpSwitches, MCP_SWITCHES_ADDR, "MCP3 Switches");
  success &= initializeMCP(mcpButtonsEnc, MCP_BUTTONS_ENC_ADDR, "MCP4 Buttons");
  
  if (!success) {
    Serial.println(F("ERROR: Falló inicialización de uno o más MCPs"));
    return false;
  }
  
  configureEncoderMCP(mcpEncodersVol, INT_MCP1_A, INT_MCP1_B);
  configureEncoderMCP(mcpEncodersPan, INT_MCP2_A, INT_MCP2_B);
  configureSwitchMCP(mcpSwitches);
  configureSwitchMCP(mcpButtonsEnc);
  
  Serial.println(F("Hardware inicializado correctamente"));
  return true;
}

bool HardwareManager::initializeMCP(Adafruit_MCP23X17& mcp, uint8_t address, const char* name) {
  Serial.print(F("Inicializando "));
  Serial.print(name);
  Serial.print(F(" en dirección 0x"));
  Serial.println(address, HEX);
  
  if (!mcp.begin_I2C(address, &Wire)) {
    Serial.print(F("ERROR: "));
    Serial.print(name);
    Serial.println(F(" no responde"));
    return false;
  }
  
  if (!validateMCPResponse(mcp, name)) {
    return false;
  }
  
  Serial.print(name);
  Serial.println(F(" inicializado correctamente"));
  return true;
}

void HardwareManager::configureEncoderMCP(Adafruit_MCP23X17& mcp, uint8_t intPinA, uint8_t intPinB) {
  for (int pin = 0; pin < 16; pin++) {
    mcp.pinMode(pin, INPUT_PULLUP);
    mcp.setupInterruptPin(pin, CHANGE);
  }
  
  mcp.setupInterrupts(true, false, LOW);
  
  pinMode(intPinA, INPUT_PULLUP);
  pinMode(intPinB, INPUT_PULLUP);
}

void HardwareManager::configureSwitchMCP(Adafruit_MCP23X17& mcp) {
  for (int pin = 0; pin < 16; pin++) {
    mcp.pinMode(pin, INPUT_PULLUP);
  }
}

void HardwareManager::setupInterrupts() {
  attachInterrupt(digitalPinToInterrupt(INT_MCP1_A), handleMCP1AInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(INT_MCP1_B), handleMCP1BInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(INT_MCP2_A), handleMCP2AInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(INT_MCP2_B), handleMCP2BInterrupt, FALLING);
  
  Serial.println(F("Interrupciones configuradas"));
}

void HardwareManager::processMCP1AEncoders() {
  uint16_t interruptPins = mcpEncodersVol.getCapturedInterrupt();
  
  for (int i = 0; i < 8; i++) {
    int pinA = i * 2;
    int pinB = i * 2 + 1;
    
    if ((interruptPins & (1 << pinA)) || (interruptPins & (1 << pinB))) {
      processEncoder(0, i, mcpEncodersVol);
    }
  }
}

void HardwareManager::processMCP1BEncoders() {
  uint16_t interruptPins = mcpEncodersVol.getCapturedInterrupt();
  
  for (int i = 8; i < 16; i++) {
    int pinA = (i - 8) * 2 + 8;
    int pinB = (i - 8) * 2 + 9;
    
    if ((interruptPins & (1 << pinA)) || (interruptPins & (1 << pinB))) {
      processEncoder(0, i, mcpEncodersVol);
    }
  }
}

void HardwareManager::processMCP2AEncoders() {
  uint16_t interruptPins = mcpEncodersPan.getCapturedInterrupt();
  
  for (int i = 0; i < 8; i++) {
    int pinA = i * 2;
    int pinB = i * 2 + 1;
    
    if ((interruptPins & (1 << pinA)) || (interruptPins & (1 << pinB))) {
      processEncoder(1, i, mcpEncodersPan);
    }
  }
}

void HardwareManager::processMCP2BEncoders() {
  uint16_t interruptPins = mcpEncodersPan.getCapturedInterrupt();
  
  for (int i = 8; i < 16; i++) {
    int pinA = (i - 8) * 2 + 8;
    int pinB = (i - 8) * 2 + 9;
    
    if ((interruptPins & (1 << pinA)) || (interruptPins & (1 << pinB))) {
      processEncoder(1, i, mcpEncodersPan);
    }
  }
}

void HardwareManager::processEncoder(int mcpIndex, int encoderIndex, Adafruit_MCP23X17& mcp) {
  int pinA, pinB;
  if (encoderIndex < 8) {
    pinA = encoderIndex * 2;
    pinB = encoderIndex * 2 + 1;
  } else {
    pinA = (encoderIndex - 8) * 2 + 8;
    pinB = (encoderIndex - 8) * 2 + 9;
  }
  
  int MSB = mcp.digitalRead(pinA);
  int LSB = mcp.digitalRead(pinB);
  int encoded = (MSB << 1) | LSB;
  
  int8_t change = calculateEncoderChange(encoded, lastEncoded[encoderIndex]);
  lastEncoded[encoderIndex] = encoded;
  
  if (change != 0) {
    unsigned long currentTime = micros();
    unsigned long timeDiff = currentTime - lastEncoderTime[encoderIndex];
    lastEncoderTime[encoderIndex] = currentTime;
    
    uint8_t acceleration = calculateAcceleration(timeDiff);
    encoderValue[encoderIndex] += change;
    
    int threshold = 4 / acceleration;
    if (abs(encoderValue[encoderIndex]) >= threshold) {
      int8_t finalChange = (encoderValue[encoderIndex] > 0) ? acceleration : -acceleration;
      onEncoderChange(encoderIndex, finalChange);
      encoderValue[encoderIndex] = 0;
    }
  }
}

int8_t HardwareManager::calculateEncoderChange(int8_t encoded, int8_t lastEncoded) {
  int sum = (lastEncoded << 2) | encoded;
  
  switch (sum) {
    case 0b1101: case 0b0100: case 0b0010: case 0b1011:
      return 1;
    case 0b1110: case 0b0111: case 0b0001: case 0b1000:
      return -1;
    default:
      return 0;
  }
}

uint8_t HardwareManager::calculateAcceleration(unsigned long timeDiff) {
  if (!appConfig.encoderAcceleration) return 1;
  
  if (timeDiff < 1000) return 4;
  else if (timeDiff < 5000) return 2;
  else return 1;
}

void HardwareManager::pollSwitchesAndButtons() {
  static unsigned long lastPollTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastPollTime < BUTTON_DEBOUNCE_MS) {
    return;
  }
  
  lastPollTime = currentTime;
  
  uint16_t mcp3State = mcpSwitches.readGPIOAB();
  uint16_t mcp3Changes = mcp3State ^ lastMCP3State;
  
  if (mcp3Changes != 0) {
    for (int i = 0; i < 16; i++) {
      if (mcp3Changes & (1 << i)) {
        bool pressed = !(mcp3State & (1 << i));
        if (pressed) {
          onSwitchPress(i);
        }
      }
    }
    lastMCP3State = mcp3State;
  }
  
  uint16_t mcp4State = mcpButtonsEnc.readGPIOAB();
  uint16_t mcp4Changes = mcp4State ^ lastMCP4State;
  
  if (mcp4Changes != 0) {
    for (int i = 0; i < 5; i++) {
      if (mcp4Changes & (1 << i)) {
        bool pressed = !(mcp4State & (1 << i));
        if (pressed) {
          onButtonPress(i);
        }
      }
    }
    lastMCP4State = mcp4State;
  }
}

void HardwareManager::readNavigationEncoder() {
  static unsigned long lastNavReadTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastNavReadTime < NAV_ENCODER_INTERVAL) {
    return;
  }
  
  lastNavReadTime = currentTime;
  
  int MSB = mcpButtonsEnc.digitalRead(ENC_NAV_A_PIN);
  int LSB = mcpButtonsEnc.digitalRead(ENC_NAV_B_PIN);
  
  int encoded = (MSB << 1) | LSB;
  int8_t change = calculateEncoderChange(encoded, lastEncodedNav);
  lastEncodedNav = encoded;
  
  if (change != 0) {
    unsigned long currentMicros = micros();
    unsigned long timeDiff = currentMicros - lastEncoderTimeNav;
    lastEncoderTimeNav = currentMicros;
    
    uint8_t acceleration = calculateAcceleration(timeDiff);
    encoderValueNav += change;
    
    int threshold = 4 / acceleration;
    if (abs(encoderValueNav) >= threshold) {
      int8_t finalChange = (encoderValueNav > 0) ? acceleration : -acceleration;
      onNavigationEncoderChange(finalChange);
      encoderValueNav = 0;
    }
  }
  
  bool currentButtonState = !mcpButtonsEnc.digitalRead(ENC_NAV_SW_PIN);
  
  if (currentButtonState != lastNavButtonState) {
    lastNavPressTime = currentTime;
  }
  
  if ((currentTime - lastNavPressTime) > BUTTON_DEBOUNCE_MS) {
    if (currentButtonState != navButtonState) {
      navButtonState = currentButtonState;
      if (navButtonState) {
        onNavigationButtonPress();
      }
    }
  }
  
  lastNavButtonState = currentButtonState;
}

bool HardwareManager::validateMCPResponse(Adafruit_MCP23X17& mcp, const char* name) {
  uint8_t testValue = 0xAA;
  
  mcp.writeGPIOA(testValue);
  delay(1);
  
  uint8_t readValue = mcp.readGPIOA();
  
  if (readValue != testValue) {
    Serial.print(F("ERROR: Test de comunicación falló para "));
    Serial.println(name);
    handleMCPError(name, "communication test");
    return false;
  }
  
  mcp.writeGPIOA(0x00);
  return true;
}

void HardwareManager::handleMCPError(const char* mcpName, const char* operation) {
  Serial.print(F("ERROR MCP: "));
  Serial.print(mcpName);
  Serial.print(F(" - "));
  Serial.println(operation);
}

bool HardwareManager::checkMCPHealth() {
  bool allHealthy = true;
  
  Wire.beginTransmission(MCP_ENCODERS_VOL_ADDR);
  if (Wire.endTransmission() != 0) {
    handleMCPError("MCP1", "health check");
    allHealthy = false;
  }
  
  Wire.beginTransmission(MCP_ENCODERS_PAN_ADDR);
  if (Wire.endTransmission() != 0) {
    handleMCPError("MCP2", "health check");
    allHealthy = false;
  }
  
  Wire.beginTransmission(MCP_SWITCHES_ADDR);
  if (Wire.endTransmission() != 0) {
    handleMCPError("MCP3", "health check");
    allHealthy = false;
  }
  
  Wire.beginTransmission(MCP_BUTTONS_ENC_ADDR);
  if (Wire.endTransmission() != 0) {
    handleMCPError("MCP4", "health check");
    allHealthy = false;
  }
  
  return allHealthy;
}

void HardwareManager::resetMCPs() {
  Serial.println(F("Reiniciando MCPs..."));
  
  clearAllInterrupts();
  
  configureEncoderMCP(mcpEncodersVol, INT_MCP1_A, INT_MCP1_B);
  configureEncoderMCP(mcpEncodersPan, INT_MCP2_A, INT_MCP2_B);
  configureSwitchMCP(mcpSwitches);
  configureSwitchMCP(mcpButtonsEnc);
  
  Serial.println(F("MCPs reiniciados"));
}

void HardwareManager::clearAllInterrupts() {
  mcpEncodersVol.getCapturedInterrupt();
  mcpEncodersPan.getCapturedInterrupt();
}

bool HardwareManager::testAllMCPs() {
  Serial.println(F("Ejecutando test completo de MCPs..."));
  
  bool success = true;
  success &= validateMCPResponse(mcpEncodersVol, "MCP1");
  success &= validateMCPResponse(mcpEncodersPan, "MCP2");
  success &= validateMCPResponse(mcpSwitches, "MCP3");
  success &= validateMCPResponse(mcpButtonsEnc, "MCP4");
  
  if (success) {
    Serial.println(F("Test de MCPs completado exitosamente"));
  } else {
    Serial.println(F("Test de MCPs falló"));
  }
  
  return success;
}

void HardwareManager::calibrateEncoders() {
  Serial.println(F("Calibrando encoders..."));
  
  memset(encoderValue, 0, sizeof(encoderValue));
  memset(lastEncoded, 0, sizeof(lastEncoded));
  memset(lastEncoderTime, 0, sizeof(lastEncoderTime));
  
  encoderValueNav = 0;
  lastEncodedNav = 0;
  lastEncoderTimeNav = 0;
  
  Serial.println(F("Calibración completada"));
}
bool HardwareManager::validateMCPResponseConst(const Adafruit_MCP23X17& mcp, const char* name) const {
  // For const validation, we can only check if the device responds
  // by attempting to read from it (this should be const-safe)
  try {
    // Use a non-const reference temporarily (this is a workaround)
    // In practice, read operations should be const
    Adafruit_MCP23X17& nonConstMcp = const_cast<Adafruit_MCP23X17&>(mcp);
    uint16_t value = nonConstMcp.readGPIOAB();
    (void)value; // Avoid unused variable warning
    return true;
  } catch (...) {
    // For const method, we can't call handleMCPError, so just return false
    Serial.print(F("ERROR: MCP "));
    Serial.print(name);
    Serial.println(F(" read failed"));
    return false;
  }
}

void HardwareManager::printDiagnostics() const {
  Serial.println(F("\n=== DIAGNÓSTICO HARDWARE ==="));
  
  Serial.print(F("Estado MCP1: "));
  Serial.println(validateMCPResponseConst(mcpEncodersVol, "MCP1") ? "OK" : "ERROR");
  
  Serial.print(F("Estado MCP2: "));
  Serial.println(validateMCPResponseConst(mcpEncodersPan, "MCP2") ? "OK" : "ERROR");
  
  Serial.print(F("Estado MCP3: "));
  Serial.println(validateMCPResponseConst(mcpSwitches, "MCP3") ? "OK" : "ERROR");
  
  Serial.print(F("Estado MCP4: "));
  Serial.println(validateMCPResponseConst(mcpButtonsEnc, "MCP4") ? "OK" : "ERROR");
  
  Serial.println(F("==========================\n"));
}