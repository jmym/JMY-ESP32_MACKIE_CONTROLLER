#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==================== VERSIÓN Y INFORMACIÓN ====================
#define FIRMWARE_VERSION_MAJOR  1
#define FIRMWARE_VERSION_MINOR  0
#define FIRMWARE_VERSION_PATCH  0
#define DEVICE_NAME            "Mackie MIDI Controller"
#define MANUFACTURER_ID         0x7B

// ==================== CONFIGURACIÓN DE HARDWARE (ESP32-S3) ====================
// Pines seguros para ESP32-S3 (evitando GPIOs de strapping y PSRAM)
#define TFT_CS          10
#define TFT_DC          7  
#define TFT_RST         6
#define TFT_BL          5
#define TFT_SCLK        12
#define TFT_MOSI        11
#define TFT_MISO        13

#define SD_CS           4     // GPIO4
#define SD_SCLK         12
#define SD_MOSI         11
#define SD_MISO         13

// Pines de interrupción para MCPs
#define INT_MCP1_A      1     // GPIO1 (TX0)
#define INT_MCP1_B      2     // GPIO2
#define INT_MCP2_A      3     // GPIO3 (RX0)
#define INT_MCP2_B      8     // GPIO8

// Direcciones I2C de los MCP23017
#define MCP_ENCODERS_VOL_ADDR   0x20
#define MCP_ENCODERS_PAN_ADDR   0x21
#define MCP_SWITCHES_ADDR       0x22
#define MCP_BUTTONS_ENC_ADDR    0x23

// Pines del encoder de navegación (conectados al MCP4)
#define ENC_NAV_A_PIN   8
#define ENC_NAV_B_PIN   9
#define ENC_NAV_SW_PIN  10

// ==================== CONFIGURACIÓN DEL SISTEMA ====================
#define NUM_ENCODERS    16
#define NUM_BANKS       4
#define NUM_SWITCHES    16
#define NUM_BUTTONS     5

// Intervalos de tiempo (ms)
#define POLL_INTERVAL_MS        5
#define DISPLAY_UPDATE_INTERVAL 50
#define NAV_ENCODER_INTERVAL    2
#define ENCODER_DEBOUNCE_MS     1
#define BUTTON_DEBOUNCE_MS      50
#define SCREENSAVER_CHECK_MS    1000

// Tiempos de actualización optimizados
#define MIDI_UPDATE_INTERVAL      2    // ms entre procesamientos MIDI
#define VU_UPDATE_INTERVAL        50   // ms entre actualizaciones de VU
#define DISPLAY_PARTIAL_UPDATE    20   // ms para actualizaciones parciales

// ==================== CONFIGURACIÓN MIDI ====================
#define MIDI_CHANNEL_DEFAULT    1
#define MTC_FRAME_RATE         30
#define ENCODER_ACCELERATION   true

#define MIDI_PLAY              0xFA
#define MIDI_STOP              0xFC  
#define MIDI_RECORD            0xFB
#define MIDI_JOG_FORWARD       60
#define MIDI_JOG_BACKWARD      61

// ==================== CONFIGURACIÓN DE PANTALLA ====================
#define TFT_WIDTH              480
#define TFT_HEIGHT             320
#define DEFAULT_BRIGHTNESS     100
#define MIN_BRIGHTNESS         0
#define MAX_BRIGHTNESS         100

#define MIDI_BUFFER_SIZE       32
#define SYSEX_BUFFER_SIZE      16
#define MAX_PRESET_NAME        10
#define MAX_FILENAME_LENGTH    12

// ==================== COLORES STUDIO ONE 7 (RGB565) ====================
#define NUM_COLORS             24

#ifndef COLOR_BLACK
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_YELLOW    0xFFE0
#define COLOR_ORANGE    0xFC00
#define COLOR_LIGHT_GRAY 0xC618
#define COLOR_DARK_GRAY  0x7BEF
#endif

#define RGB_TO_565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// Paleta de colores Studio One 7
static const uint16_t STUDIO_ONE_COLORS[NUM_COLORS] = {
  0xF800, 0xFD20, 0xFFE0, 0x87E0, 0x07E0, 0x07FF, 0x051F, 0x001F,
  0x401F, 0x801F, 0xF81F, 0xF810, 0xFC00, 0xFD20, 0xFFE0, 0x87E0,
  0x07E0, 0x07FF, 0x051F, 0x001F, 0x401F, 0x801F, 0xF81F, 0xF810
};

// ==================== ENUMERACIONES ====================
enum ControlType {
  CT_CC = 0,
  CT_NOTE = 1,
  CT_PITCH = 2
};

enum ButtonIndex {
  BTN_PLAY = 0,
  BTN_STOP = 1,  
  BTN_REC = 2,
  BTN_BANK_UP = 3,
  BTN_BANK_DOWN = 4
};

enum DisplayOrientation {
  ORIENT_0 = 0,
  ORIENT_90 = 1,
  ORIENT_180 = 2,
  ORIENT_270 = 3
};

enum MenuValueType {
  MENU_ACTION = 0,
  MENU_INTEGER = 1,
  MENU_BOOLEAN = 2,
  MENU_OPTION = 3
};

enum class MenuType {
    MAIN_MENU,
    ENCODER_SETTINGS,
    BANK_MANAGEMENT,
    SYSTEM_SETTINGS,
    MIDI_SETTINGS,
    DISPLAY_SETTINGS,
    PRESET_MANAGEMENT,
    NONE
};

enum TaskPriority {
  PRIORITY_CRITICAL,
  PRIORITY_HIGH,
  PRIORITY_NORMAL,
  PRIORITY_LOW,
  PRIORITY_BACKGROUND
};
// Estados de transporte mejorados
enum TransportStateEx {
    TRANSPORT_STOPPED,
    TRANSPORT_PLAYING,
    TRANSPORT_RECORDING,
    TRANSPORT_PAUSED,
    TRANSPORT_FAST_FORWARD,
    TRANSPORT_REWIND
};
// ==================== ESTRUCTURAS DE DATOS ====================
struct EncoderConfig {
  uint8_t channel : 4;
  uint8_t control : 7;
  uint8_t controlType : 2;
  bool isPan : 1;
  int8_t value;
  int8_t dawValue;
  int8_t minValue;
  int8_t maxValue;
  bool isMute : 1;
  bool isSolo : 1;
  uint16_t trackColor;
  char trackName[3];

  EncoderConfig() : channel(1), control(7), controlType(CT_CC), isPan(false),
                   value(64), dawValue(64), minValue(0), maxValue(127),
                   isMute(false), isSolo(false), trackColor(0xFFFF) {
    strncpy(trackName, "Trk", 3);
    trackName[2] = '\0';
  }
};

struct AppConfig {
  uint8_t brightness;
  uint16_t screensaverTimeout;
  uint8_t currentBank;
  int8_t mtcOffset;
  bool encoderAcceleration;
  uint8_t midiChannel;
  DisplayOrientation orientation;
  bool autoSave;
  uint8_t encoderSensitivity;
  uint16_t vuMeterDecay;

  AppConfig() {
    brightness = DEFAULT_BRIGHTNESS;
    screensaverTimeout = 300;
    currentBank = 0;
    mtcOffset = 0;
    encoderAcceleration = true;
    midiChannel = MIDI_CHANNEL_DEFAULT;
    orientation = ORIENT_270;
    autoSave = true;
    encoderSensitivity = 5;
    vuMeterDecay = 1000;
  }
};

struct MtcData {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t frames;
  bool isRunning;
  
  MtcData() : hours(0), minutes(0), seconds(0), frames(0), isRunning(false) {}
};

struct TransportState {
  bool isPlaying;
  bool isRecording;
  bool isPaused;
  
  TransportState() : isPlaying(false), isRecording(false), isPaused(false) {}
};

struct SystemState {
  unsigned long lastActivityTime;
  unsigned long lastPollTime;
  unsigned long lastDisplayUpdate;
  unsigned long lastDiagnostic;
  bool screensaverActive;
  bool inMenu;
  uint8_t currentBank;
  uint16_t freeMemory;
  bool sdCardPresent;
  bool displayNeedsUpdate;
  bool menuActive;
  
  SystemState() {
    lastActivityTime = 0;
    lastPollTime = 0;
    lastDisplayUpdate = 0;
    lastDiagnostic = 0;
    screensaverActive = false;
    inMenu = false;
    currentBank = 0;
    freeMemory = 0;
    sdCardPresent = false;
    displayNeedsUpdate = false;
    menuActive = false;
  }
};

struct InterruptFlags {
  volatile bool mcp1A;
  volatile bool mcp1B;
  volatile bool mcp2A;
  volatile bool mcp2B;
  
  InterruptFlags() : mcp1A(false), mcp1B(false), mcp2A(false), mcp2B(false) {}
};

// ==================== DECLARACIONES EXTERNAS ====================
extern SystemState systemState;
extern AppConfig appConfig;
extern volatile InterruptFlags interruptFlags;

// Callbacks del sistema
extern void onEncoderChange(uint8_t encoderIndex, int8_t change);
extern void onSwitchPress(uint8_t switchIndex);
extern void onButtonPress(uint8_t buttonIndex);
extern void onNavigationEncoderChange(int8_t change);
extern void onNavigationButtonPress();
extern void onMenuExit();
extern void resetActivity();

// ISRs
extern void handleMCP1AInterrupt();
extern void handleMCP1BInterrupt(); 
extern void handleMCP2AInterrupt();
extern void handleMCP2BInterrupt();

// Callbacks para Studio One
extern void syncEncoderColorFromDAW(uint8_t track, uint8_t bank, uint16_t color);
extern void updateVUMeterLevel(uint8_t track, uint8_t level);
extern void syncEncoderValueFromDAW(uint8_t track, uint8_t bank, uint8_t value);
extern void syncEncoderNameFromDAW(uint8_t track, uint8_t bank, const char* name);

#endif // CONFIG_H