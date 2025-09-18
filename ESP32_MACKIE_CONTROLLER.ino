/*
 * Controlador MIDI Mackie para ESP32-S3-DevKitC-1-N16R8
 * Versión optimizada para ESP32-S3 con MIDI USB
 */

#include "Config.h"
#include "SystemManager.h"
#include "HardwareManager.h"
#include "DisplayManager.h"
#include "MidiManager.h"
#include "EncoderManager.h"
#include "MenuManager.h"
#include "FileManager.h"

// Instancias globales de los managers
SystemManager systemManager;
HardwareManager hardwareManager;
DisplayManager displayManager;
MidiManager midiManager;
EncoderManager encoderManager;
MenuManager menuManager;
FileManager fileManager;

// Variables globales
AppConfig appConfig;
SystemState systemState;
volatile InterruptFlags interruptFlags;

// ==================== CALLBACKS DEL SISTEMA ====================
void onEncoderChange(uint8_t encoderIndex, int8_t change) {
  encoderManager.processEncoderChange(encoderIndex, change, systemState.currentBank);
  resetActivity();
}

void onSwitchPress(uint8_t switchIndex) {
  encoderManager.processSwitchPress(switchIndex, systemState.currentBank);
  resetActivity();
}

void onButtonPress(uint8_t buttonIndex) {
  switch (buttonIndex) {
    case BTN_PLAY: midiManager.sendTransportCommand(MIDI_PLAY); break;
    case BTN_STOP: midiManager.sendTransportCommand(MIDI_STOP); break;
    case BTN_REC: midiManager.sendTransportCommand(MIDI_RECORD); break;
    case BTN_BANK_UP: changeBankSafely(1); break;
    case BTN_BANK_DOWN: changeBankSafely(-1); break;
  }
  resetActivity();
}

void onNavigationEncoderChange(int8_t change) {
  if (systemState.inMenu) {
    menuManager.navigate(change);
  } else {
    midiManager.sendJogWheel(change);
  }
  resetActivity();
}

void onNavigationButtonPress() {
  if (systemState.inMenu) {
    menuManager.select();
  } else {
    systemState.inMenu = true;
    menuManager.enter();
  }
  resetActivity();
}

void onMenuExit() {
  systemState.inMenu = false;
  if (appConfig.autoSave) {
    fileManager.saveConfiguration(appConfig, encoderManager.getEncoderBanks());
  }
}

void resetActivity() {
  systemState.lastActivityTime = millis();
  if (systemState.screensaverActive) {
    systemState.screensaverActive = false;
    displayManager.setBrightness(appConfig.brightness);
  }
}

void changeBankSafely(int8_t direction) {
  uint8_t newBank = (systemState.currentBank + direction + NUM_BANKS) % NUM_BANKS;
  if (newBank != systemState.currentBank) {
    systemState.currentBank = newBank;
    appConfig.currentBank = newBank;
    encoderManager.setCurrentBank(newBank);
    
    // Solicitar actualización de valores al DAW
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
      midiManager.sendStudioOneValueRequest(i, newBank);
      midiManager.sendStudioOneColorRequest(i, newBank);
    }
  }
}

// ==================== CALLBACKS DE STUDIO ONE ====================
void syncEncoderColorFromDAW(uint8_t track, uint8_t bank, uint16_t color) {
   encoderManager.updateFromDAW(track, bank, color);
}

void updateVUMeterLevel(uint8_t track, uint8_t level) {
    encoderManager.updateVULevel(track, level);
}

void syncEncoderValueFromDAW(uint8_t track, uint8_t bank, uint8_t value) {
    encoderManager.updateFromDAW(track, bank, value);
}

// Función combinada para valor y color (si se reciben juntos)
void syncEncoderFromDAW(uint8_t track, uint8_t bank, uint8_t value, uint16_t color) {
    encoderManager.updateFromDAW(track, bank, value, color);
}


void syncEncoderNameFromDAW(uint8_t track, uint8_t bank, const char* name) {
    encoderManager.syncNameFromDAW(track, bank, name);
}

// ==================== ISRs ====================
void handleMCP1AInterrupt() { interruptFlags.mcp1A = true; }
void handleMCP1BInterrupt() { interruptFlags.mcp1B = true; }
void handleMCP2AInterrupt() { interruptFlags.mcp2A = true; }  
void handleMCP2BInterrupt() { interruptFlags.mcp2B = true; }

// ==================== SETUP ====================
void setup() {
  // Inicializar USB primero
  USB.begin();
  // Inicializar comunicación serie
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println(F("Inicializando Controlador MIDI Mackie para ESP32-S3..."));

  // Inicializar sistema
  systemManager.initialize();

  // Inicializar hardware
  if (!hardwareManager.initialize()) {
    Serial.println(F("ERROR: Fallo en inicialización del hardware"));
    while (1) { 
      systemManager.resetWatchdog();
      delay(1000); 
    }
  }

  // Inicializar pantalla
  if (!displayManager.initialize(appConfig.orientation, appConfig.brightness)) {
    Serial.println(F("ERROR: Fallo en inicialización de la pantalla"));
  }

  // Inicializar MIDI
  midiManager.initialize(appConfig.midiChannel);

  // Inicializar encoders
  encoderManager.initialize(&appConfig);

  // Inicializar menú
  menuManager.initialize(&appConfig, &systemState);

  // Inicializar sistema de archivos
  if (!fileManager.initialize()) {
    Serial.println(F("ERROR: Fallo en inicialización del sistema de archivos"));
  }

  // Cargar configuración
  if (!fileManager.loadConfiguration(appConfig, encoderManager.getEncoderBanks())) {
    Serial.println(F("ERROR: No se pudo cargar la configuración"));
  }

  // Configurar interrupciones
  hardwareManager.setupInterrupts();

  // Estado inicial
  systemState.lastActivityTime = millis();
  systemState.screensaverActive = false;
  systemState.inMenu = false;
  systemState.currentBank = appConfig.currentBank;

  Serial.println(F("Sistema inicializado correctamente"));
  Serial.print(F("Memoria libre: "));
  Serial.println(systemManager.getFreeMemory());
}

// ==================== LOOP PRINCIPAL OPTIMIZADO ====================
void loop() {
  unsigned long currentTime = millis();
  static unsigned long lastLoopTime = 0;
  const unsigned long LOOP_INTERVAL = 1; // 1ms entre loops

  // 1. Reset del watchdog
  systemManager.resetWatchdog();

  // 2. Procesar interrupciones pendientes
  if (interruptFlags.mcp1A) {
    hardwareManager.processMCP1AEncoders();
    interruptFlags.mcp1A = false;
  }
  if (interruptFlags.mcp1B) {
    hardwareManager.processMCP1BEncoders();
    interruptFlags.mcp1B = false;
  }
  if (interruptFlags.mcp2A) {
    hardwareManager.processMCP2AEncoders();
    interruptFlags.mcp2A = false;
  }
  if (interruptFlags.mcp2B) {
    hardwareManager.processMCP2BEncoders();
    interruptFlags.mcp2B = false;
  }

  // 3. Polling de hardware
  if (currentTime - systemState.lastPollTime >= POLL_INTERVAL_MS) {
    hardwareManager.pollSwitchesAndButtons();
    systemState.lastPollTime = currentTime;
  }

  // 4. Procesar encoder de navegación
  hardwareManager.readNavigationEncoder();

  // 5. Procesar entrada MIDI
  midiManager.processMidiInput();

  // 6. Procesar salida MIDI
  midiManager.processMidiOutput();

if (systemState.displayNeedsUpdate) {
        systemState.displayNeedsUpdate = false;
        systemState.lastDisplayUpdate = 0; // Forzar redibujado
    }

  // 7. Actualizar pantalla
  if (currentTime - systemState.lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    if (systemState.screensaverActive) {
      displayManager.drawScreensaver(midiManager.getMtcData(), systemState.currentBank);
    } else if (systemState.inMenu) {
      menuManager.draw(displayManager);
    } else {
      displayManager.drawMainScreen(
        encoderManager.getEncoderBanks()[systemState.currentBank],
        midiManager.getMtcData(),
        systemState.currentBank,
        midiManager.getTransportState()
      );
    }
    systemState.lastDisplayUpdate = currentTime;
  }

  // 8. Gestión del salvapantallas
  if (appConfig.screensaverTimeout > 0) {
    if (!systemState.screensaverActive && 
        (currentTime - systemState.lastActivityTime) > appConfig.screensaverTimeout) {
      systemState.screensaverActive = true;
      displayManager.setBrightness(0);
    }
  }

  // 9. Actualizar diagnósticos
  if (currentTime - systemState.lastDiagnostic >= 1000) {
    systemState.freeMemory = systemManager.getFreeMemory();
    systemManager.updateDiagnostics();
    systemState.lastDiagnostic = currentTime;
  }

  // 10. Control de frecuencia de ejecución
  unsigned long elapsedTime = currentTime - lastLoopTime;
  if (elapsedTime < LOOP_INTERVAL) {
    // Esperar el tiempo restante sin bloquear
    delayMicroseconds((LOOP_INTERVAL - elapsedTime) * 1000);
  }

  lastLoopTime = millis();
}