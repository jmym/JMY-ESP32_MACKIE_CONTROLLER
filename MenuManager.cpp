#include "MenuManager.h"
#include "Strings.h"
#include "MidiManager.h"
#include "EncoderManager.h"
#include "FileManager.h"
#include "HardwareManager.h"
#include "DisplayManager.h"
#include "SystemManager.h"

#define MENU_START_Y 50
#define MENU_ITEM_HEIGHT 30
#define MESSAGE_DISPLAY_TIME 2000

extern FileManager fileManager;
extern EncoderManager encoderManager;
extern MidiManager midiManager;
extern HardwareManager hardwareManager;
extern DisplayManager displayManager;
extern SystemManager systemManager;

MenuManager* MenuManager::instance = nullptr;

const char* const MenuManager::orientationOptions[4] = {"0°", "90°", "180°", "270°"};
const char* const MenuManager::timeoutOptions[6] = {"Off", "1min", "5min", "10min", "30min", "60min"};
const char* const MenuManager::controlTypeOptions[3] = {"CC", "Note", "Pitch"};
const char* const MenuManager::encoderSelectOptions[16] = {
  "Enc 1", "Enc 2", "Enc 3", "Enc 4", "Enc 5", "Enc 6", "Enc 7", "Enc 8",
  "Enc 9", "Enc 10", "Enc 11", "Enc 12", "Enc 13", "Enc 14", "Enc 15", "Enc 16"
};

MenuManager::MenuManager() 
  : menuActive(false), currentMenuLevel(0), editingValue(false),
    appConfig(nullptr), systemState(nullptr), tempEncoderIndex(0),
    scrollOffset(0), visibleItems(6), showingMessage(false), 
    showingConfirmDialog(false), confirmCallback(nullptr),
    mainMenu{
        MenuItem{Strings::str_encoders, actionEnterEncoderMenu, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{Strings::str_display, actionEnterDisplayMenu, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{Strings::str_midi, actionEnterMidiMenu, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{Strings::str_global, actionEnterGlobalMenu, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{Strings::str_system_test, actionSystemTest, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{Strings::str_exit, actionExitMenu, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true}
    },
    encoderMenu{
        MenuItem{"Seleccionar Encoder", actionSelectEncoder, MENU_OPTION, &tempEncoderIndex, 0, 15, (const char**)encoderSelectOptions, 16, true, true},
        MenuItem{"Modo Vol/Pan", actionToggleEncoderMode, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Tipo Control", actionSetControlType, MENU_OPTION, nullptr, 0, 2, (const char**)controlTypeOptions, 3, true, true},
        MenuItem{"Canal MIDI", actionSetMidiChannel, MENU_INTEGER, nullptr, 1, 16, nullptr, 0, true, true},
        MenuItem{"Numero Control", actionSetControlNumber, MENU_INTEGER, nullptr, 0, 127, nullptr, 0, true, true},
        MenuItem{"Ajustar Rango", actionSetEncoderRange, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Reset Encoder", actionResetEncoder, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Volver", actionBackMenu, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true}
    },
    displayMenu{
        MenuItem{"Brillo", actionSetBrightness, MENU_INTEGER, &tempBrightness, 0, 100, nullptr, 0, true, true},
        MenuItem{"Timeout Pantalla", actionSetScreensaver, MENU_OPTION, &tempScreensaverTimeout, 0, 5, (const char**)timeoutOptions, 6, true, true},
        MenuItem{"Orientacion", actionSetOrientation, MENU_OPTION, &tempOrientation, 0, 3, (const char**)orientationOptions, 4, true, true},
        MenuItem{"Test Pantalla", actionDisplayTest, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Volver", actionBackMenu, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true}
    },
    midiMenu{
        MenuItem{"Canal MIDI", actionSetGlobalMidiChannel, MENU_INTEGER, &tempMidiChannel, 1, 16, nullptr, 0, true, true},
        MenuItem{"Aceleracion Enc", actionToggleEncoderAccel, MENU_BOOLEAN, nullptr, 0, 1, nullptr, 0, true, true},
        MenuItem{"Offset MTC", actionSetMtcOffset, MENU_INTEGER, &tempMtcOffset, -30, 30, nullptr, 0, true, true},
        MenuItem{"Test MIDI", actionTestMidi, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Reset MIDI", actionResetMidi, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Volver", actionBackMenu, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true}
    },
    globalMenu{
        MenuItem{"Guardar Config", actionSaveConfig, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Cargar Config", actionLoadConfig, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Reset Total", actionResetConfiguration, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Guardar Preset", actionSaveBank, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Cargar Preset", actionLoadBank, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Calibrar MCPs", actionCalibrateMcp, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true},
        MenuItem{"Volver", actionBackMenu, MENU_ACTION, nullptr, 0, 0, nullptr, 0, true, true}
    }
{
  instance = this;
  memset(currentPosition, 0, sizeof(currentPosition));
  memset(currentMenuType, 0, sizeof(currentMenuType));
  
  tempBrightness = 100;
  tempMidiChannel = 1;
  tempMtcOffset = 0;
  tempScreensaverTimeout = 2;
  tempOrientation = 3;
}

MenuManager::~MenuManager() {
  instance = nullptr;
}

bool MenuManager::initialize(AppConfig* config, SystemState* state) {
  Serial.println(F("Inicializando sistema de menús..."));
  
  appConfig = config;
  systemState = state;
  refreshFromConfig();
  
  Serial.println(F("Sistema de menús inicializado"));
  return true;
}

void MenuManager::enter() {
  menuActive = true;
  currentMenuLevel = 0;
  currentMenuType[0] = MenuType::MAIN_MENU;
  currentPosition[0] = 0;
  scrollOffset = 0;
  editingValue = false;
  
  Serial.println(F("Menú activado"));
}

void MenuManager::exit() {
  if (editingValue) {
    stopValueEdit();
  }
  
  applyTempValues();
  menuActive = false;
  currentMenuLevel = 0;
  
  Serial.println(F("Menú desactivado"));
}

void MenuManager::navigate(int8_t direction) {
  if (showingMessage || showingConfirmDialog) return;
  
  if (editingValue) {
    changeValue(direction);
    return;
  }
  
  uint8_t menuSize = getCurrentMenuSize();
  if (menuSize == 0) return;
  
  int16_t newPos = currentPosition[currentMenuLevel] + direction;
  
  if (newPos < 0) {
    newPos = menuSize - 1;
  } else if (newPos >= menuSize) {
    newPos = 0;
  }
  
  currentPosition[currentMenuLevel] = newPos;
  
  if (menuSize > visibleItems) {
    if (newPos < scrollOffset) {
      scrollOffset = newPos;
    } else if (newPos >= scrollOffset + visibleItems) {
      scrollOffset = newPos - visibleItems + 1;
    }
  }
}

void MenuManager::select() {
  if (showingMessage) {
    showingMessage = false;
    return;
  }
  
  if (showingConfirmDialog) {
    if (confirmCallback) {
      confirmCallback();
    }
    showingConfirmDialog = false;
    confirmCallback = nullptr;
    return;
  }
  
  MenuItem* currentMenu = getCurrentMenu();
  if (!currentMenu) return;
  
  uint8_t pos = currentPosition[currentMenuLevel];
  MenuItem& item = currentMenu[pos];
  
  if (!item.enabled) return;
  
  if (editingValue) {
    stopValueEdit();
    return;
  }
  
  switch (item.valueType) {
    case MENU_ACTION:
      if (item.action) {
        item.action();
      } else {
        executeAction();
      }
      break;
      
    case MENU_INTEGER:
    case MENU_BOOLEAN:
    case MENU_OPTION:
      if (item.valuePtr) {
        startValueEdit();
      }
      break;
  }
}

void MenuManager::back() {
  if (editingValue) {
    stopValueEdit();
    return;
  }
  
  if (showingMessage || showingConfirmDialog) {
    showingMessage = false;
    showingConfirmDialog = false;
    return;
  }
  
  if (currentMenuLevel > 0) {
    currentMenuLevel--;
    scrollOffset = 0;
  } else {
    exit();
  }
}

void MenuManager::draw(DisplayManager& display) {
  unsigned long currentTime = millis();
  
  // Verificar si el mensaje ha expirado
  if (showingMessage && (currentTime - messageStartTime >= MESSAGE_DISPLAY_TIME)) {
    showingMessage = false;
  }
  
  if (showingMessage) {
    display.drawMessage(currentMessage);
    return;
  }
  
  if (showingConfirmDialog) {
    display.drawConfirmDialog(currentMessage);
    return;
  }
  
  display.clearScreen(COLOR_BLACK);
  
  const char* title = "Menu Principal";
  switch (currentMenuType[currentMenuLevel]) {
    case MenuType::ENCODER_SETTINGS: title = "Encoders"; break;
    case MenuType::DISPLAY_SETTINGS: title = "Pantalla"; break;
    case MenuType::MIDI_SETTINGS: title = "MIDI"; break;
    case MenuType::SYSTEM_SETTINGS: title = "Global"; break;
    default: break;
  }
  
  drawMenuTitle(title);
  drawMenuItems();
  
  if (getCurrentMenuSize() > visibleItems) {
    drawScrollIndicator();
  }
  
  if (editingValue) {
    drawValueEditor();
  }
}

void MenuManager::drawMenuTitle(const char* title) {
  displayManager.fillRect(0, 0, TFT_WIDTH, 40, COLOR_DARK_GRAY);
  displayManager.drawLine(0, 40, TFT_WIDTH, 40, COLOR_WHITE);
  
  displayManager.drawCenteredText(title, 0, 5, TFT_WIDTH, 30, COLOR_WHITE, FONT_SIZE_LARGE);
  
  if (currentMenuLevel > 0) {
    displayManager.drawRightAlignedText("<<", TFT_WIDTH - 20, 12, COLOR_CYAN, FONT_SIZE_MEDIUM);
  }
}

void MenuManager::drawMenuItems() {
  MenuItem* currentMenu = getCurrentMenu();
  if (!currentMenu) return;
  
  uint8_t menuSize = getCurrentMenuSize();
  uint16_t yPos = MENU_START_Y;
  
  displayManager.fillRect(0, MENU_START_Y, TFT_WIDTH, TFT_HEIGHT - MENU_START_Y - 40, COLOR_BLACK);
  
  for (int i = 0; i < visibleItems && (i + scrollOffset) < menuSize; i++) {
    uint8_t itemIndex = i + scrollOffset;
    MenuItem& item = currentMenu[itemIndex];
    
    if (!item.visible) continue;
    
    bool selected = (itemIndex == currentPosition[currentMenuLevel]);
    bool editing = (selected && editingValue);
    
    if (selected) {
      uint16_t bgColor = editing ? COLOR_BLUE : COLOR_DARK_GRAY;
      displayManager.fillRect(5, yPos - 3, TFT_WIDTH - 10, MENU_ITEM_HEIGHT - 4, bgColor);
    }
    
    char itemText[24];
    StringUtils::readString(item.name, itemText, sizeof(itemText));
    
    uint16_t textColor = item.enabled ? COLOR_WHITE : COLOR_LIGHT_GRAY;
    if (selected) textColor = COLOR_WHITE;
    if (!item.enabled) textColor = COLOR_DARK_GRAY;
    
    displayManager.drawMenuItem(itemText, yPos, selected, editing);
    
    if (item.valuePtr && item.valueType != MENU_ACTION) {
      const char* valueStr = formatValue(item, *(int16_t*)item.valuePtr);
      displayManager.drawMenuValue(valueStr, TFT_WIDTH - 100, yPos, editing);
    }
    
    if (item.valueType == MENU_ACTION && item.action != actionExitMenu && 
        currentMenuType[currentMenuLevel] == MenuType::MAIN_MENU) {
      displayManager.drawRightAlignedText(">", TFT_WIDTH - 20, yPos + 5, COLOR_CYAN, FONT_SIZE_SMALL);
    }
    
    yPos += MENU_ITEM_HEIGHT;
  }
}

void MenuManager::drawValueEditor() {
  uint16_t editorY = TFT_HEIGHT - 60;
  displayManager.fillRect(10, editorY, TFT_WIDTH - 20, 50, COLOR_DARK_GRAY);
  displayManager.drawRect(10, editorY, TFT_WIDTH - 20, 50, COLOR_WHITE);
  
  displayManager.drawCenteredText("Usar encoder nav para cambiar", 15, editorY + 5, 
                          TFT_WIDTH - 30, 20, COLOR_YELLOW, FONT_SIZE_SMALL);
  displayManager.drawCenteredText("Presionar para confirmar", 15, editorY + 25, 
                          TFT_WIDTH - 30, 20, COLOR_WHITE, FONT_SIZE_SMALL);
}

void MenuManager::drawScrollIndicator() {
  uint8_t menuSize = getCurrentMenuSize();
  uint16_t scrollBarHeight = (TFT_HEIGHT - MENU_START_Y - 20) * visibleItems / menuSize;
  uint16_t scrollBarY = MENU_START_Y + ((TFT_HEIGHT - MENU_START_Y - 20) * scrollOffset / menuSize);
  
  displayManager.fillRect(TFT_WIDTH - 8, MENU_START_Y, 6, TFT_HEIGHT - MENU_START_Y - 20, COLOR_DARK_GRAY);
  displayManager.fillRect(TFT_WIDTH - 7, scrollBarY, 4, scrollBarHeight, COLOR_WHITE);
}

void MenuManager::startValueEdit() {
  MenuItem* currentMenu = getCurrentMenu();
  if (!currentMenu) return;
  
  MenuItem& item = currentMenu[currentPosition[currentMenuLevel]];
  if (!item.valuePtr) return;
  
  editingValue = true;
  Serial.print(F("Editando: "));
  Serial.println(item.name);
}

void MenuManager::stopValueEdit() {
  editingValue = false;
  applyTempValues();
  Serial.println(F("Edición terminada"));
}

void MenuManager::changeValue(int8_t change) {
  MenuItem* currentMenu = getCurrentMenu();
  if (!currentMenu) return;
  
  MenuItem& item = currentMenu[currentPosition[currentMenuLevel]];
 if (!item.valuePtr) return;
  
  int16_t* value = (int16_t*)item.valuePtr;
  int16_t newValue = *value + change;
  
  switch (item.valueType) {
    case MENU_INTEGER:
      newValue = constrainValue(newValue, item.minValue, item.maxValue);
      break;
      
    case MENU_BOOLEAN:
      newValue = (newValue < 0) ? 1 : (newValue > 1) ? 0 : newValue;
      break;
      
    case MENU_OPTION:
      if (newValue < item.minValue) newValue = item.maxValue;
      if (newValue > item.maxValue) newValue = item.minValue;
      break;
  }
  
  *value = newValue;
  
  // Aplicar cambios en tiempo real
  if (item.valuePtr == &tempBrightness) {
    displayManager.setBrightness(tempBrightness);
  } else if (item.valuePtr == &tempOrientation) {
    displayManager.setOrientation((DisplayOrientation)tempOrientation);
  }
}

void MenuManager::applyTempValues() {
  if (!appConfig) return;
  
  appConfig->brightness = tempBrightness;
  appConfig->midiChannel = tempMidiChannel;
  appConfig->mtcOffset = tempMtcOffset;
  appConfig->orientation = (DisplayOrientation)tempOrientation;
  
  uint32_t timeouts[] = {0, 60000, 300000, 600000, 1800000, 3600000};
  appConfig->screensaverTimeout = timeouts[constrainValue(tempScreensaverTimeout, 0, 5)];
  
  Serial.println(F("Configuración aplicada"));
}

void MenuManager::refreshFromConfig() {
  if (!appConfig) return;
  
  tempBrightness = appConfig->brightness;
  tempMidiChannel = appConfig->midiChannel;
  tempMtcOffset = appConfig->mtcOffset;
  tempOrientation = (int16_t)appConfig->orientation;
  
  uint32_t timeout = appConfig->screensaverTimeout;
  if (timeout == 0) tempScreensaverTimeout = 0;
  else if (timeout <= 60000) tempScreensaverTimeout = 1;
  else if (timeout <= 300000) tempScreensaverTimeout = 2;
  else if (timeout <= 600000) tempScreensaverTimeout = 3;
  else if (timeout <= 1800000) tempScreensaverTimeout = 4;
  else tempScreensaverTimeout = 5;
}

MenuItem* MenuManager::getCurrentMenu() {
  switch (currentMenuType[currentMenuLevel]) {
    case MenuType::MAIN_MENU: return mainMenu;
    case MenuType::ENCODER_SETTINGS: return encoderMenu;
    case MenuType::DISPLAY_SETTINGS: return displayMenu;
    case MenuType::MIDI_SETTINGS: return midiMenu;
    case MenuType::SYSTEM_SETTINGS: return globalMenu;
    default: return nullptr;
  }
}

uint8_t MenuManager::getCurrentMenuSize() {
  switch (currentMenuType[currentMenuLevel]) {
    case MenuType::MAIN_MENU: return 6;
    case MenuType::ENCODER_SETTINGS: return 8;
    case MenuType::DISPLAY_SETTINGS: return 5;
    case MenuType::MIDI_SETTINGS: return 6;
    case MenuType::SYSTEM_SETTINGS: return 7;
    default: return 0;
  }
}

const char* MenuManager::formatValue(const MenuItem& item, int16_t value) {
  static char buffer[16];
  
  switch (item.valueType) {
    case MENU_INTEGER:
      snprintf(buffer, sizeof(buffer), "%d", value);
      break;
      
    case MENU_BOOLEAN:
      strcpy(buffer, value ? "On" : "Off");
      break;
      
    case MENU_OPTION:
      if (item.options && value >= 0 && value < item.optionCount) {
        strncpy(buffer, item.options[value], sizeof(buffer));
        return buffer;
      } else {
        snprintf(buffer, sizeof(buffer), "%d", value);
      }
      break;
      
    default:
      buffer[0] = '\0';
      break;
  }
  
  return buffer;
}

void MenuManager::showMessage(const char* message, uint16_t duration) {
  currentMessage = message;
  messageStartTime = millis();
  showingMessage = true;
}

void MenuManager::showConfirmDialog(const char* message, void (*onConfirm)()) {
  currentMessage = message;
  confirmCallback = onConfirm;
  showingConfirmDialog = true;
}

void MenuManager::executeAction() {
  MenuItem* currentMenu = getCurrentMenu();
  if (!currentMenu) return;
  
  uint8_t pos = currentPosition[currentMenuLevel];
  
  switch (currentMenuType[currentMenuLevel]) {
    case MenuType::MAIN_MENU:
      executeMainMenuAction(pos);
      break;
    case MenuType::ENCODER_SETTINGS:
      executeEncoderMenuAction(pos);
      break;
    case MenuType::DISPLAY_SETTINGS:
      executeDisplayMenuAction(pos);
      break;
    case MenuType::MIDI_SETTINGS:
      executeMidiMenuAction(pos);
      break;
    case MenuType::SYSTEM_SETTINGS:
      executeSystemMenuAction(pos);
      break;
    default:
      break;
  }
}

// ==================== ACCIONES MENÚ PRINCIPAL ====================
void MenuManager::actionEnterEncoderMenu() {
  if (!instance) return;
  if (instance->currentMenuLevel >= 4) return;
  
  instance->currentMenuLevel++;
  instance->currentMenuType[instance->currentMenuLevel] = MenuType::ENCODER_SETTINGS;
  instance->currentPosition[instance->currentMenuLevel] = 0;
  instance->scrollOffset = 0;
}

void MenuManager::actionEnterDisplayMenu() {
  if (!instance) return;
  if (instance->currentMenuLevel >= 4) return;
  
  instance->currentMenuLevel++;
  instance->currentMenuType[instance->currentMenuLevel] = MenuType::DISPLAY_SETTINGS;
  instance->currentPosition[instance->currentMenuLevel] = 0;
  instance->scrollOffset = 0;
}

void MenuManager::actionEnterMidiMenu() {
  if (!instance) return;
  if (instance->currentMenuLevel >= 4) return;
  
  instance->currentMenuLevel++;
  instance->currentMenuType[instance->currentMenuLevel] = MenuType::MIDI_SETTINGS;
  instance->currentPosition[instance->currentMenuLevel] = 0;
  instance->scrollOffset = 0;
}

void MenuManager::actionEnterGlobalMenu() {
  if (!instance) return;
  if (instance->currentMenuLevel >= 4) return;
  
  instance->currentMenuLevel++;
  instance->currentMenuType[instance->currentMenuLevel] = MenuType::SYSTEM_SETTINGS;
  instance->currentPosition[instance->currentMenuLevel] = 0;
  instance->scrollOffset = 0;
}

void MenuManager::actionSystemTest() {
  if (!instance) return;
  
  Serial.println(F("Ejecutando test del sistema..."));
  
  // Test de pantalla
  displayManager.runDisplayTest();
  
  // Test de MCPs
  if (hardwareManager.testAllMCPs()) {
    instance->showMessage("Test hardware: OK", 2000);
  } else {
    instance->showMessage("Test hardware: ERROR", 3000);
  }
  
  // Test de MIDI
  midiManager.sendControlChange(1, 7, 64);
  instance->showMessage("Test MIDI enviado", 1500);
  
  // Test de SD
  if (fileManager.checkSDHealth()) {
    instance->showMessage("Test SD: OK", 2000);
  } else {
    instance->showMessage("Test SD: ERROR", 3000);
  }
  
  Serial.println(F("Test del sistema completado"));
}

void MenuManager::actionExitMenu() {
  if (!instance) return;
  instance->exit();
}

// ==================== ACCIONES MENÚ ENCODERS ====================
void MenuManager::actionSelectEncoder() {
  if (!instance) return;
  Serial.print(F("Encoder seleccionado: "));
  Serial.println(instance->tempEncoderIndex + 1);
}

void MenuManager::actionToggleEncoderMode() {
  if (!instance) return;
  
  uint8_t encIndex = instance->tempEncoderIndex;
  uint8_t bank = instance->systemState ? instance->systemState->currentBank : 0;
  
  EncoderConfig& config = encoderManager.getEncoderConfigMutable(encIndex, bank);
  config.isPan = !config.isPan;
  
  instance->showMessage(config.isPan ? "Modo: Pan" : "Modo: Volumen", 1500);
  
  Serial.print(F("Encoder "));
  Serial.print(encIndex + 1);
  Serial.print(F(" cambiado a modo: "));
  Serial.println(config.isPan ? F("Pan") : F("Volumen"));
}

void MenuManager::actionSetControlType() {
  if (!instance) return;
  
  uint8_t encIndex = instance->tempEncoderIndex;
  uint8_t bank = instance->systemState ? instance->systemState->currentBank : 0;
  
  EncoderConfig& config = encoderManager.getEncoderConfigMutable(encIndex, bank);
  
  switch (config.controlType) {
    case CT_CC:
      config.controlType = CT_NOTE;
      instance->showMessage("Tipo: Note", 1500);
      break;
    case CT_NOTE:
      config.controlType = CT_PITCH;
      instance->showMessage("Tipo: Pitch Bend", 1500);
      break;
    case CT_PITCH:
      config.controlType = CT_CC;
      instance->showMessage("Tipo: Control Change", 1500);
      break;
  }
}

void MenuManager::actionSetMidiChannel() {
  if (!instance) return;
  
  uint8_t encIndex = instance->tempEncoderIndex;
  uint8_t bank = instance->systemState ? instance->systemState->currentBank : 0;
  
  EncoderConfig& config = encoderManager.getEncoderConfigMutable(encIndex, bank);
  config.channel = instance->tempMidiChannel;
  
  char msg[32];
  snprintf(msg, sizeof(msg), "Canal MIDI: %d", config.channel);
  instance->showMessage(msg, 1500);
}

void MenuManager::actionSetControlNumber() {
  if (!instance) return;
  
  uint8_t encIndex = instance->tempEncoderIndex;
  uint8_t bank = instance->systemState ? instance->systemState->currentBank : 0;
  
  EncoderConfig& config = encoderManager.getEncoderConfigMutable(encIndex, bank);
  config.control = (config.control + 1) % 128;
  
  char msg[20];
  snprintf(msg, sizeof(msg), "Control: %d", config.control);
  instance->showMessage(msg, 1500);
}

void MenuManager::actionSetEncoderRange() {
  if (!instance) return;
  
  uint8_t encIndex = instance->tempEncoderIndex;
  uint8_t bank = instance->systemState ? instance->systemState->currentBank : 0;
  
  EncoderConfig& config = encoderManager.getEncoderConfigMutable(encIndex, bank);
  
  if (config.minValue == 0 && config.maxValue == 127) {
    config.minValue = 0;
    config.maxValue = 100;
    instance->showMessage("Rango: 0-100", 1500);
  } else if (config.minValue == 0 && config.maxValue == 100) {
    config.minValue = 64;
    config.maxValue = 127;
    instance->showMessage("Rango: 64-127", 1500);
  } else {
    config.minValue = 0;
    config.maxValue = 127;
    instance->showMessage("Rango: 0-127", 1500);
  }
}

void MenuManager::actionResetEncoder() {
  if (!instance) return;
  
  uint8_t encIndex = instance->tempEncoderIndex;
  uint8_t bank = instance->systemState ? instance->systemState->currentBank : 0;
  
  // Declare extern reference to the global encoderManager
  extern EncoderManager encoderManager;
  encoderManager.resetEncoderConfig(encIndex, bank);
  
  instance->showMessage("Encoder reseteado", 1500);
}

void MenuManager::actionBackMenu() {
  if (!instance) return;
  instance->back();
}

// ==================== ACCIONES MENÚ PANTALLA ====================
void MenuManager::actionSetBrightness() {
  if (!instance) return;
  
  instance->appConfig->brightness = instance->tempBrightness;
  displayManager.setBrightness(instance->tempBrightness);
  
  char msg[20];
  snprintf(msg, sizeof(msg), "Brillo: %d%%", instance->tempBrightness);
  instance->showMessage(msg, 1500);
}

void MenuManager::actionSetScreensaver() {
  if (!instance) return;
  
  uint32_t timeouts[] = {0, 60000, 300000, 600000, 1800000, 3600000};
  instance->appConfig->screensaverTimeout = timeouts[instance->tempScreensaverTimeout];
  
  const char* labels[] = {"Desactivado", "1 minuto", "5 minutos", "10 minutos", "30 minutos", "60 minutos"};
  
  char msg[32];
  snprintf(msg, sizeof(msg), "Timeout: %s", labels[instance->tempScreensaverTimeout]);
  instance->showMessage(msg, 1500);
}

void MenuManager::actionSetOrientation() {
  if (!instance) return;
  
  instance->appConfig->orientation = (DisplayOrientation)instance->tempOrientation;
  displayManager.setOrientation(instance->appConfig->orientation);
  
  const char* orientations[] = {"0°", "90°", "180°", "270°"};
  
  char msg[32];
  snprintf(msg, sizeof(msg), "Orientación: %s", orientations[instance->tempOrientation]);
  instance->showMessage(msg, 1500);
}

void MenuManager::actionDisplayTest() {
  if (!instance) return;
  
  instance->showMessage("Ejecutando test...", 1000);
  displayManager.runDisplayTest();
}

// ==================== ACCIONES MENÚ MIDI ====================
void MenuManager::actionSetGlobalMidiChannel() {
  if (!instance) return;
  
  instance->appConfig->midiChannel = instance->tempMidiChannel;
  midiManager.setMidiChannel(instance->tempMidiChannel);
  
  char msg[32];
  snprintf(msg, sizeof(msg), "Canal global: %极", instance->tempMidiChannel);
  instance->showMessage(msg, 1500);
}

void MenuManager::actionToggleEncoderAccel() {
  if (!instance) return;
  
  instance->appConfig->encoderAcceleration = !instance->appConfig->encoderAcceleration;
  encoderManager.setEncoderAcceleration(instance->appConfig->encoderAcceleration);
  
  instance->showMessage(instance->appConfig->encoderAcceleration ? "Aceleración: ON" : "Aceleración: OFF", 1500);
}

void MenuManager::actionTestMidi() {
  if (!instance) return;
  
  instance->showMessage("Test MIDI...", 1000);
  midiManager.sendTestSequence();
  instance->showMessage("Test MIDI enviado", 2000);
}

void MenuManager::actionResetMidi() {
  if (!instance) return;
  instance->showConfirmDialog("¿Reset configuración MIDI?", confirmResetMidiCallback);
}

// ==================== ACCIONES MENÚ SISTEMA ====================
void MenuManager::actionSaveConfig() {
  if (!instance) return;
  
  instance->showMessage("Guardando...", 1000);
  
  if (fileManager.saveConfiguration(*instance->appConfig, encoderManager.getEncoderBanks())) {
    instance->showMessage("Configuración guardada", 2000);
    Serial.println(F("Configuración guardada exitosamente"));
  } else {
    instance->showMessage("Error al guardar", 3000);
    Serial.println(F("ERROR: No se pudo guardar configuración"));
  }
}

void MenuManager::actionLoadConfig() {
  if (!instance) return;
  instance->showConfirmDialog("¿Cargar configuración guardada?", []() {
    if (!instance) return;
    
    instance->showMessage("Cargando...", 1000);
    
    if (fileManager.loadConfiguration(*instance->appConfig, encoderManager.getEncoderBanks())) {
      instance->refreshFromConfig();
      instance->showMessage("Configuración cargada", 2000);
      Serial.println(F("Configuración cargada exitosamente"));
    } else {
      instance->showMessage("Error al cargar", 3000);
      Serial.println(F("ERROR: No se pudo cargar configuración"));
    }
  });
}

void MenuManager::actionResetConfiguration() {
  if (!instance) return;
  instance->showConfirmDialog("¿RESET TOTAL del sistema?", confirmResetConfigCallback);
}

void MenuManager::actionSaveBank() {
  if (!instance) return;
  
  uint8_t currentBank = instance->systemState ? instance->systemState->currentBank : 0;
  
  char presetName[16];
  snprintf(presetName, sizeof(presetName), "Bank_%d", currentBank + 1);
  
  instance->showMessage("Guardando banco...", 1000);
  
  if (fileManager.savePreset(presetName, encoderManager.getEncoderBanks())) {
    char msg[32];
    snprintf(msg, sizeof(msg), "Banco %d guardado", currentBank + 1);
    instance->showMessage(msg, 2000);
    Serial.print(F("Banco "));
    Serial.print(currentBank + 1);
    Serial.println(F(" guardado como preset"));
  } else {
    instance->showMessage("Error guardando banco", 3000);
  }
}

void MenuManager::actionLoadBank() {
  if (!instance) return;
  
  uint8_t currentBank = instance->systemState ? instance->systemState->currentBank : 0;
  
  char presetName[16];
  snprintf(presetName, sizeof(presetName), "Bank_%d", currentBank + 1);
  
  char confirmMsg[32];
  snprintf(confirmMsg, sizeof(confirmMsg), "¿Cargar preset para Banco %d?", currentBank + 1);
  
  instance->showConfirmDialog(confirmMsg, confirmLoadBankCallback);
}

void MenuManager::actionCalibrateMcp() {
  if (!instance) return;
  instance->showConfirmDialog("¿Calibrar expansores I/O?", confirmCalibrateMcpCallback);
}

// ==================== CALLBACKS DE CONFIRMACIÓN ====================
void MenuManager::confirmResetMidiCallback() {
  if (!instance) return;
  
  instance->appConfig->midiChannel = 1;
  instance->appConfig->encoderAcceleration = true;
  instance->appConfig->mtcOffset = 0;
  
  instance->tempMidiChannel = 1;
  instance->tempMtcOffset = 0;
  
  instance->showMessage("MIDI reseteado", 2000);
  Serial.println(F("Configuración MIDI reseteada"));
}

void MenuManager::confirmResetConfigCallback() {
  if (!instance) return;
  
  instance->showMessage("Reseteando sistema...", 2000);
  
  fileManager.resetConfiguration();
  encoderManager.resetAllBanks();
  
  *instance->appConfig = AppConfig();
  instance->refreshFromConfig();
  
  instance->showMessage("Sistema reseteado", 3000);
  Serial.println(F("Sistema completamente reseteado"));
}

void MenuManager::confirmCalibrateMcpCallback() {
  if (!instance) return;
  
  instance->showMessage("Calibrando MCPs...", 2000);
  hardwareManager.calibrateEncoders();
  
  if (hardwareManager.testAllMCPs()) {
    instance->showMessage("Calibración exitosa", 2000);
    Serial.println(F("Calibración de MCPs completada"));
  } else {
    instance->showMessage("Error en calibración", 3000);
    Serial.println(F("ERROR: Falló calibración de MCPs"));
  }
}

void MenuManager::confirmLoadBankCallback() {
  if (!instance) return;
  
  uint8_t currentBank = instance->systemState ? instance->systemState->currentBank : 0;
  char presetName[16];
  snprintf(presetName, sizeof(presetName), "Bank_%d", currentBank + 1);
  
  instance->showMessage("Cargando banco...", 1000);
  
  if (fileManager.loadPreset(presetName, encoderManager.getEncoderBanks())) {
    char msg[32];
    snprintf(msg, sizeof(msg), "Banco %d cargado", currentBank + 1);
    instance->showMessage(msg, 2000);
    Serial.print(F("Preset cargado en banco "));
    Serial.println(currentBank + 1);
  } else {
    instance->showMessage("Preset no encontrado", 3000);
  }
}

// ==================== EJECUCIÓN DE ACCIONES POR ÍNDICE ====================
void MenuManager::executeMainMenuAction(uint8_t actionIndex) {
  switch (actionIndex) {
    case 0: actionEnterEncoderMenu(); break;
    case 1: actionEnterDisplayMenu(); break;
    case 2: actionEnterMidiMenu(); break;
    case 3: actionEnterGlobalMenu(); break;
    case 4: actionSystemTest(); break;
    case 5: actionExitMenu(); break;
    default: break;
  }
}

void MenuManager::executeEncoderMenuAction(uint8_t actionIndex) {
  switch (actionIndex) {
    case 0: actionSelectEncoder(); break;
    case 1: actionToggleEncoderMode(); break;
    case 2: actionSetControlType(); break; // Fixed name
    case 3: actionSetMidiChannel(); break;
    case 4: actionSetControlNumber(); break;
    case 5: actionSetEncoderRange(); break;
    case 6: actionResetEncoder(); break;
    case 7: actionBackMenu(); break;
    default: break;
  }
}

void MenuManager::executeDisplayMenuAction(uint8_t actionIndex) {
  switch (actionIndex) {
    case 0: actionSetBrightness(); break;
    case 1: actionSetScreensaver(); break;
    case 2: actionSetOrientation(); break;
    case 3: actionDisplayTest(); break;
    case 4: actionBackMenu(); break;
    default: break;
  }
}

void MenuManager::executeMidiMenuAction(uint8_t actionIndex) {
  switch (actionIndex) {
    case 0: actionSetGlobalMidiChannel(); break;
    case 1: actionToggleEncoderAccel(); break;
    case 2: actionSetMtcOffset(); break;
    case 3: actionTestMidi(); break;
    case 4: actionResetMidi(); break;
    case 5: actionBackMenu(); break;
    default: break;
  }
}
void MenuManager::actionSetMtcOffset() {
  if (!instance) return;
  
  instance->appConfig->mtcOffset = instance->tempMtcOffset;
  
  char msg[32];
  snprintf(msg, sizeof(msg), "Offset MTC: %d", instance->tempMtcOffset);
  instance->showMessage(msg, 1500);
}
void MenuManager::executeSystemMenuAction(uint8_t actionIndex) {
  switch (actionIndex) {
    case 0: actionSaveConfig(); break;
    case 1: actionLoadConfig(); break;
    case 2: actionResetConfiguration(); break;
    case 3: actionSaveBank(); break;
    case 4: actionLoadBank(); break;
    case 5: actionCalibrateMcp(); break;
    case 6: actionBackMenu(); break;
    default: break;
  }
}