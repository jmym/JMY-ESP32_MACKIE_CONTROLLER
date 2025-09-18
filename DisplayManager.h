#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "Config.h"
#include <Adafruit_ST7796S.h>
#include <Adafruit_GFX.h>
#include <SPI.h>

#define VOLUME_BAR_WIDTH    20
#define VOLUME_BAR_HEIGHT   120
#define PAN_BAR_WIDTH       15
#define PAN_BAR_HEIGHT      50
#define CHANNEL_WIDTH       55
#define HEADER_HEIGHT       40
#define FOOTER_HEIGHT       30
#define METER_DECAY_RATE    2

#define FONT_SIZE_SMALL     1
#define FONT_SIZE_MEDIUM    2
#define FONT_SIZE_LARGE     3
#define FONT_SIZE_XLARGE    4

class DisplayManager {
private:
  Adafruit_ST7796S tft;
  bool initialized;
  uint8_t currentBrightness;
  DisplayOrientation currentOrientation;
  
  uint8_t vuLevels[8];
  unsigned long lastVuUpdate[8];
  bool channelDirty[8];
    bool transportDirty;
    bool mtcDirty;

  bool needsFullRedraw;
  unsigned long lastFullRedraw;
  
  struct LayoutPositions {
    uint16_t channelX[8];
    uint16_t volumeBarY;
    uint16_t panBarY;
    uint16_t textY;
    uint16_t headerY;
    uint16_t mtcX, mtcY;
    uint16_t bankX, bankY;
  } layout;
  
  void calculateLayout();
  void drawVolumeBar(uint8_t channel, uint8_t value, uint16_t color, bool highlighted);
  void drawPanBar(uint8_t channel, uint8_t value, uint16_t color);
  void drawVUMeter(uint8_t channel, uint8_t level, uint16_t color);
  void drawChannelInfo(uint8_t channel, const EncoderConfig& config);
  void drawHeader();
  void drawFooter(const TransportState& transport);
  
  void updateVUMeters();
  uint8_t calculateVUDecay(uint8_t currentLevel, unsigned long timeSinceUpdate);
  
  bool shouldRedrawElement(uint8_t elementId, unsigned long currentTime);
  void markElementForRedraw(uint8_t elementId);

public:
  DisplayManager();
  ~DisplayManager();
  
  bool initialize(DisplayOrientation orientation = ORIENT_270, uint8_t brightness = 100);
  void setOrientation(DisplayOrientation orientation);
  void setBrightness(uint8_t brightness);
  uint8_t getBrightness() const { return currentBrightness; }
  
  void drawMainScreen(const EncoderConfig encoders[NUM_ENCODERS], 
                     const MtcData& mtc, uint8_t currentBank, 
                     const TransportState& transport);
  void drawScreensaver(const MtcData& mtc, uint8_t currentBank);
  void drawBootScreen();
  void drawErrorScreen(const char* error);
  
  void drawCenteredText(const char* text, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                       uint16_t color, uint8_t size = FONT_SIZE_MEDIUM);
  void drawRightAlignedText(const char* text, uint16_t x, uint16_t y, 
                           uint16_t color, uint8_t size = FONT_SIZE_MEDIUM);
  uint16_t getTextWidth(const char* text, uint8_t size = FONT_SIZE_MEDIUM);
  uint16_t getTextHeight(uint8_t size = FONT_SIZE_MEDIUM);
  
  void clearScreen(uint16_t color = COLOR_BLACK);
  void drawMenuItem(const char* text, uint16_t y, bool selected, bool editing = false);
  void drawMenuValue(const char* value, uint16_t x, uint16_t y, bool editing = false);
  void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                      uint8_t value, uint16_t color = COLOR_GREEN);
  
  bool isInitialized() const { return initialized; }
  void forceFullRedraw() { needsFullRedraw = true; }
  void drawPixel(uint16_t x, uint16_t y, uint16_t color);
  void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
  void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
  void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
  
  void drawMTCInfo(const MtcData& mtc);
  void drawTransportInfo(const TransportState& transport, uint8_t currentBank);
  void drawMessage(const char* message);
  void drawConfirmDialog(const char* message);
  
  void setVULevel(uint8_t channel, uint8_t level);
  uint8_t getVULevel(uint8_t channel) const;
  
  void runDisplayTest();
  void showDiagnostics();
  void benchmarkDisplay();
  void setForceRedraw(bool force) { needsFullRedraw = force; lastFullRedraw = 0; }
  void markChannelDirty(uint8_t channel);
    void markTransportDirty();
    void markMtcDirty();


  uint16_t getWidth() const { return TFT_WIDTH; }
  uint16_t getHeight() const { return TFT_HEIGHT; }
  DisplayOrientation getOrientation() const { return currentOrientation; }
  
private:
  //bool channelDirty[8]; // Añade este miembro privado

  void initializePins();
  bool testDisplayConnection();
  void setupSPI();
  

  int16_t textWidth(const char* text);
};

#endif