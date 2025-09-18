#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include "Config.h"
#include "DisplayManager.h"
#include <Arduino.h>

using MenuType = ::MenuType;
using MenuValueType = ::MenuValueType;

// First, define MenuItem completely before MenuManager
struct MenuItem {
  const char* name;
  void (*action)();
  MenuValueType valueType;
  void* valuePtr;
  int16_t minValue;
  int16_t maxValue;
  const char* const* options;
  uint8_t optionCount;
  bool enabled;
  bool visible;
  
  MenuItem(const char* n, void (*a)(), MenuValueType vt, void* vp,
           int16_t min, int16_t max, const char* const* opt, uint8_t optCount,
           bool en, bool vis)
    : name(n), action(a), valueType(vt), valuePtr(vp),
      minValue(min), maxValue(max), options(opt), optionCount(optCount),
      enabled(en), visible(vis) {}
};

class MenuManager {
public:
  static MenuManager* instance;
  
  static const char* const orientationOptions[4];
  static const char* const timeoutOptions[6];
  static const char* const controlTypeOptions[3];
  static const char* const encoderSelectOptions[16];
  
  MenuManager();
  ~MenuManager();
  
  bool initialize(AppConfig* config, SystemState* state);
  
  void enter();
  void exit();
  void navigate(int8_t direction);
  void select();
  void back();
  
  void draw(DisplayManager& display);
  
  void showMessage(const char* message, uint16_t duration = 2000);
  void showConfirmDialog(const char* message, void (*onConfirm)());
  
  static void actionEnterEncoderMenu();
  static void actionEnterDisplayMenu();
  static void actionEnterMidiMenu();
  static void actionEnterGlobalMenu();
  static void actionExitMenu();
  static void actionSelectEncoder();
  static void actionToggleEncoderMode();
  static void actionSetControlType();
  static void actionSetMidiChannel();
  static void actionSetControlNumber();
  static void actionSetEncoderRange();
  static void actionResetEncoder();
  static void actionSetBrightness();
  static void actionSetScreensaver();
  static void actionSetOrientation();
  static void actionDisplayTest();
  static void actionSetGlobalMidiChannel();
  static void actionToggleEncoderAccel();
  static void actionSetMtcOffset();
  static void actionTestMidi();
  static void actionResetMidi();
  static void actionSaveConfig();
  static void actionLoadConfig();
  static void actionResetConfiguration();
  static void actionSaveBank();
  static void actionLoadBank();
  static void actionCalibrateMcp();
  static void actionSystemTest();
  static void actionBackMenu();

private:
  MenuItem mainMenu[6];
  MenuItem encoderMenu[8];
  MenuItem displayMenu[5];
  MenuItem midiMenu[6];
  MenuItem globalMenu[7];
  
  bool menuActive;
  uint8_t currentMenuLevel;
  MenuType currentMenuType[5];
  uint8_t currentPosition[5];
  bool editingValue;
  
  AppConfig* appConfig;
  SystemState* systemState;
  
  int16_t tempEncoderIndex;
  int16_t tempBrightness;
  int16_t tempMidiChannel;
  int16_t tempMtcOffset;
  int16_t tempScreensaverTimeout;
  int16_t tempOrientation;
  
  int16_t scrollOffset;
  uint8_t visibleItems;
  
  bool showingMessage;
  const char* currentMessage;
  unsigned long messageStartTime;
  bool showingConfirmDialog;
  void (*confirmCallback)();
  
  MenuItem* getCurrentMenu();
  uint8_t getCurrentMenuSize();
  const char* formatValue(const MenuItem& item, int16_t value);
  void startValueEdit();
  void stopValueEdit();
  void changeValue(int8_t change);
  void applyTempValues();
  void refreshFromConfig();
  void executeAction();
  
  void drawMenuTitle(const char* title);
  void drawMenuItems();
  void drawValueEditor();
  void drawScrollIndicator();
  
  void executeMainMenuAction(uint8_t actionIndex);
  void executeEncoderMenuAction(uint8_t actionIndex);
  void executeDisplayMenuAction(uint8_t actionIndex);
  void executeMidiMenuAction(uint8_t actionIndex);
  void executeSystemMenuAction(uint8_t actionIndex);
  static void confirmResetMidiCallback();
  static void confirmResetConfigCallback();
  static void confirmCalibrateMcpCallback();
  static void confirmLoadBankCallback();
  
  int16_t constrainValue(int16_t value, int16_t min, int16_t max) {
    return (value < min) ? min : (value > max) ? max : value;
  }
};

#endif