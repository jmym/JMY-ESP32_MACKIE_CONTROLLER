#include "DisplayManager.h"
#include "Strings.h"
#include <SPI.h>

DisplayManager::DisplayManager() 
  : tft(TFT_CS, TFT_DC, TFT_RST), initialized(false), currentBrightness(100),
    currentOrientation(ORIENT_270), needsFullRedraw(true), lastFullRedraw(0),
    transportDirty(false), mtcDirty(false)
{
  memset(vuLevels, 0, sizeof(vuLevels));
  memset(lastVuUpdate, 0, sizeof(lastVuUpdate));
  memset(channelDirty, true, sizeof(channelDirty)); // Inicializar todos como sucios
  calculateLayout();
}

DisplayManager::~DisplayManager() {
}

bool DisplayManager::initialize(DisplayOrientation orientation, uint8_t brightness) {
  Serial.println(F("Inicializando pantalla ST7796..."));
  
  initializePins();
  setupSPI();
  
  tft.init(); // Elimina la verificación de retorno
  
  // Verifica la conexión con un test personalizado
  if (!testDisplayConnection()) {
    Serial.println(F("ERROR: Pantalla no responde"));
    return false;
  }
  
  setOrientation(orientation);
  setBrightness(brightness);
  
  clearScreen(COLOR_BLACK);
  drawBootScreen();
  
  unsigned long startTime = millis();
  while (millis() - startTime < 2000) {
    // Espera no bloqueante
  }
  
  clearScreen(COLOR_BLACK);
  
  initialized = true;
  Serial.println(F("Pantalla inicializada correctamente"));
  return true;
}

void DisplayManager::initializePins() {
  pinMode(TFT_BL, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_RST, HIGH);
}

void DisplayManager::setupSPI() {
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  SPI.setFrequency(40000000); // 40MHz
}

bool DisplayManager::testDisplayConnection() {
  tft.fillScreen(COLOR_BLACK);
  unsigned long startTime = millis();
  while (millis() - startTime < 100) {
    // Espera no bloqueante
  }
  
  tft.fillScreen(COLOR_WHITE);
  startTime = millis();
  while (millis() - startTime < 100) {
    // Espera no bloqueante
  }
  
  tft.fillScreen(COLOR_BLACK);
  return true;
}

void DisplayManager::setOrientation(DisplayOrientation orientation) {
  currentOrientation = orientation;
  tft.setRotation((uint8_t)orientation);
  calculateLayout();
  needsFullRedraw = true;
}

void DisplayManager::setBrightness(uint8_t brightness) {
  currentBrightness = constrain(brightness, 0, 100);
  analogWrite(TFT_BL, map(currentBrightness, 0, 100, 0, 255));
}

void DisplayManager::calculateLayout() {
  uint16_t effectiveWidth = TFT_WIDTH - 20;
  uint16_t channelSpacing = effectiveWidth / 8;
  
  for (int i = 0; i < 8; i++) {
    layout.channelX[i] = 10 + (i * channelSpacing) + (channelSpacing - CHANNEL_WIDTH) / 2;
  }
  
  layout.headerY = 10;
  layout.volumeBarY = HEADER_HEIGHT + 10;
  layout.panBarY = layout.volumeBarY + VOLUME_BAR_HEIGHT + 10;
  layout.textY = layout.panBarY + PAN_BAR_HEIGHT + 15;
  
  layout.mtcX = TFT_WIDTH / 2 - 80;
  layout.mtcY = 15;
  layout.bankX = 20;
  layout.bankY = 15;
}

void DisplayManager::drawMainScreen(const EncoderConfig encoders[NUM_ENCODERS], 
                                  const MtcData& mtc, uint8_t currentBank, 
                                  const TransportState& transport) {
  if (!initialized) return;
  
  unsigned long currentTime = millis();
  
  if (needsFullRedraw || (currentTime - lastFullRedraw > 5000)) {
    clearScreen(COLOR_BLACK);
    drawHeader();
    needsFullRedraw = false;
    lastFullRedraw = currentTime;
  }
  
  updateVUMeters();
  
  for (int i = 0; i < 8; i++) {
    const EncoderConfig& volEncoder = encoders[i];
    const EncoderConfig& panEncoder = encoders[i + 8];
    
    drawVolumeBar(i, volEncoder.dawValue, volEncoder.trackColor, 
                 volEncoder.isSolo || volEncoder.isMute);
    drawVUMeter(i, vuLevels[i], volEncoder.trackColor);
    
    drawPanBar(i, panEncoder.dawValue, panEncoder.trackColor);
    
    drawChannelInfo(i, volEncoder);
  }
  
  drawMTCInfo(mtc);
  drawTransportInfo(transport, currentBank);
}

void DisplayManager::drawVolumeBar(uint8_t channel, uint8_t value, uint16_t color, bool highlighted) {
  uint16_t x = layout.channelX[channel];
  uint16_t y = layout.volumeBarY;
  
  tft.drawRect(x, y, VOLUME_BAR_WIDTH, VOLUME_BAR_HEIGHT, COLOR_LIGHT_GRAY);
  tft.fillRect(x + 1, y + 1, VOLUME_BAR_WIDTH - 2, VOLUME_BAR_HEIGHT - 2, COLOR_BLACK);
  
  uint16_t barHeight = map(value, 0, 127, 0, VOLUME_BAR_HEIGHT - 4);
  if (barHeight > 0) {
    uint16_t barColor = highlighted ? COLOR_RED : color;
    tft.fillRect(x + 2, y + VOLUME_BAR_HEIGHT - 2 - barHeight, 
                VOLUME_BAR_WIDTH - 4, barHeight, barColor);
  }
  
  uint16_t zeroDbY = y + VOLUME_BAR_HEIGHT - map(100, 0, 127, 0, VOLUME_BAR_HEIGHT - 4);
  tft.drawFastHLine(x - 2, zeroDbY, VOLUME_BAR_WIDTH + 4, COLOR_YELLOW);
}

void DisplayManager::drawPanBar(uint8_t channel, uint8_t value, uint16_t color) {
  uint16_t x = layout.channelX[channel] + 25;
  uint16_t y = layout.panBarY;
  
  tft.drawRect(x, y, PAN_BAR_WIDTH, PAN_BAR_HEIGHT, COLOR_LIGHT_GRAY);
  tft.fillRect(x + 1, y + 1, PAN_BAR_WIDTH - 2, PAN_BAR_HEIGHT - 2, COLOR_BLACK);
  
  uint16_t centerY = y + PAN_BAR_HEIGHT / 2;
  tft.drawFastHLine(x - 2, centerY, PAN_BAR_WIDTH + 4, COLOR_WHITE);
  
  uint16_t panY = map(value, 0, 127, y + PAN_BAR_HEIGHT - 6, y + 2);
  tft.fillRect(x + 2, panY, PAN_BAR_WIDTH - 4, 4, color);
}

// Mejora la función drawVUMeter para ser más responsive
void DisplayManager::drawVUMeter(uint8_t channel, uint8_t level, uint16_t color) {
    if (channel >= 8) return;
    
    uint16_t x = layout.channelX[channel] + 42;
    uint16_t y = layout.volumeBarY;
    
    // Limpiar el área del VU meter
    tft.fillRect(x, y, 3, VOLUME_BAR_HEIGHT, COLOR_BLACK);
    
    if (level > 0) {
        uint16_t meterHeight = map(level, 0, 127, 0, VOLUME_BAR_HEIGHT);
        
        // Dibujar el VU meter con colores según el nivel
        for (int i = 0; i < meterHeight; i++) {
            uint16_t vuColor;
            uint8_t levelPercent = map(i, 0, VOLUME_BAR_HEIGHT, 0, 100);
            
            if (levelPercent < 60) vuColor = COLOR_GREEN;
            else if (levelPercent < 80) vuColor = COLOR_YELLOW;
            else vuColor = COLOR_RED;
            
            tft.drawFastHLine(x, y + VOLUME_BAR_HEIGHT - i - 1, 3, vuColor);
        }
    }
}

void DisplayManager::drawChannelInfo(uint8_t channel, const EncoderConfig& config) {
  uint16_t x = layout.channelX[channel];
  uint16_t y = layout.textY;
  
  char channelStr[3];
  snprintf(channelStr, sizeof(channelStr), "%d", channel + 1);
  drawCenteredText(channelStr, x, y, CHANNEL_WIDTH, 16, COLOR_WHITE, FONT_SIZE_MEDIUM);
  
  char valueStr[4];
  snprintf(valueStr, sizeof(valueStr), "%d", config.dawValue);
  drawCenteredText(valueStr, x, y + 20, CHANNEL_WIDTH, 12, COLOR_LIGHT_GRAY, FONT_SIZE_SMALL);
  
  char nameDisplay[7];
  snprintf(nameDisplay, sizeof(nameDisplay), "%.5s", config.trackName);
  drawCenteredText(nameDisplay, x, y + 40, CHANNEL_WIDTH, 12, 
                   COLOR_LIGHT_GRAY, FONT_SIZE_SMALL);
                   
  if (config.isMute) {
    tft.fillCircle(x + 5, y + 35, 4, COLOR_RED);
    drawCenteredText("M", x, y + 30, 12, 12, COLOR_WHITE, FONT_SIZE_SMALL);
  }
  
  if (config.isSolo) {
    tft.fillCircle(x + CHANNEL_WIDTH - 10, y + 35, 4, COLOR_YELLOW);
    drawCenteredText("S", x + CHANNEL_WIDTH - 15, y + 30, 12, 12, COLOR_BLACK, FONT_SIZE_SMALL);
  }
}

void DisplayManager::drawHeader() {
  tft.drawFastHLine(0, HEADER_HEIGHT, TFT_WIDTH, COLOR_LIGHT_GRAY);
}

void DisplayManager::drawMTCInfo(const MtcData& mtc) {
  char timeStr[12];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d:%02d", 
           mtc.hours, mtc.minutes, mtc.seconds, mtc.frames);
  
  tft.fillRect(layout.mtcX - 5, layout.mtcY - 2, 160, 25, COLOR_DARK_GRAY);
  
  tft.setTextColor(mtc.isRunning ? COLOR_GREEN : COLOR_WHITE);
  tft.setTextSize(FONT_SIZE_LARGE);
  tft.setCursor(layout.mtcX, layout.mtcY);
  tft.print(timeStr);
}

void DisplayManager::drawTransportInfo(const TransportState& transport, uint8_t currentBank) {
  char bankStr[3];
  snprintf(bankStr, sizeof(bankStr), "B%d", currentBank + 1);
  
  tft.fillRect(layout.bankX - 2, layout.bankY - 2, 40, 25, COLOR_DARK_GRAY);
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(FONT_SIZE_MEDIUM);
  tft.setCursor(layout.bankX, layout.bankY);
  tft.print(bankStr);
  
  uint16_t transportX = TFT_WIDTH - 80;
  
  if (transport.isPlaying) {
    tft.fillCircle(transportX, layout.bankY + 10, 6, COLOR_GREEN);
    drawCenteredText("P", transportX - 3, layout.bankY + 5, 8, 12, COLOR_BLACK, FONT_SIZE_SMALL);
  }
  
  if (transport.isRecording) {
    tft.fillCircle(transportX + 20, layout.bankY + 10, 6, COLOR_RED);
    drawCenteredText("R", transportX + 17, layout.bankY + 5, 8, 12, COLOR_WHITE, FONT_SIZE_SMALL);
  }
}

void DisplayManager::drawScreensaver(const MtcData& mtc, uint8_t currentBank) {
  clearScreen(COLOR_BLACK);
  drawMTCInfo(mtc);
  
  char bankStr[8];
  snprintf(bankStr, sizeof(bankStr), "Bank %d", currentBank + 1);
  tft.setTextColor(COLOR_DARK_GRAY);
  tft.setTextSize(FONT_SIZE_SMALL);
  tft.setCursor(10, TFT_HEIGHT - 20);
  tft.print(bankStr);
}

void DisplayManager::drawBootScreen() {
  clearScreen(COLOR_BLACK);
  
  drawCenteredText("MACKIE MIDI", TFT_WIDTH/2 - 100, TFT_HEIGHT/2 - 60, 
                   200, 30, COLOR_CYAN, FONT_SIZE_LARGE);
  drawCenteredText("CONTROLLER", TFT_WIDTH/2 - 100, TFT_HEIGHT/2 - 30, 
                   200, 30, COLOR_CYAN, FONT_SIZE_LARGE);
  
  char version[20];
  snprintf(version, sizeof(version), "v%d.%d.%d", 
           FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
  drawCenteredText(version, TFT_WIDTH/2 - 50, TFT_HEIGHT/2 + 10, 
                   100, 20, COLOR_WHITE, FONT_SIZE_MEDIUM);
  
  for (int i = 0; i <= 100; i += 10) {
    drawProgressBar(TFT_WIDTH/2 - 100, TFT_HEIGHT/2 + 40, 200, 8, i, COLOR_GREEN);
    unsigned long startTime = millis();
    while (millis() - startTime < 50) {
      // Espera no bloqueante
    }
  }
}

// En DisplayManager.cpp, mejora la función updateVUMeters:
void DisplayManager::updateVUMeters() {
    unsigned long currentTime = millis();
    
    for (int i = 0; i < 8; i++) {
        if (currentTime - lastVuUpdate[i] >= 100) {
            if (vuLevels[i] > 0) {
                uint8_t decayedLevel = calculateVUDecay(vuLevels[i], currentTime - lastVuUpdate[i]);
                if (decayedLevel != vuLevels[i]) {
                    vuLevels[i] = decayedLevel;
                    markChannelDirty(i); // Marcar como sucio si cambió
                    lastVuUpdate[i] = currentTime;
                }
            }
        }
    }
}

uint8_t DisplayManager::calculateVUDecay(uint8_t currentLevel, unsigned long timeSinceUpdate) {
    // Decaimiento más rápido para niveles altos, más lento para niveles bajos
    float decayRate = METER_DECAY_RATE * (1.0 + (currentLevel / 127.0) * 2.0);
    uint8_t decayAmount = (timeSinceUpdate / 100) * decayRate;
    
    if (currentLevel <= decayAmount) {
        return 0;
    }
    
    return currentLevel - decayAmount;
}

void DisplayManager::clearScreen(uint16_t color) {
  if (!initialized) return;
  tft.fillScreen(color);
}

void DisplayManager::drawCenteredText(const char* text, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                                    uint16_t color, uint8_t size) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  
  uint16_t textW = getTextWidth(text, size);
  uint16_t textH = getTextHeight(size);
  
  uint16_t startX = x + (w - textW) / 2;
  uint16_t startY = y + (h - textH) / 2;
  
  tft.setCursor(startX, startY);
  tft.print(text);
}

void DisplayManager::drawRightAlignedText(const char* text, uint16_t x, uint16_t y, 
                                        uint16_t color, uint8_t size) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  
  uint16_t textW = getTextWidth(text, size);
  tft.setCursor(x - textW, y);
  tft.print(text);
}

void DisplayManager::drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                                   uint8_t value, uint16_t color) {
  tft.drawRect(x, y, w, h, COLOR_WHITE);
  tft.fillRect(x + 1, y + 1, w - 2, h - 2, COLOR_BLACK);
  
  uint16_t fillWidth = map(value, 0, 100, 0, w - 2);
  if (fillWidth > 0) {
    tft.fillRect(x + 1, y + 1, fillWidth, h - 2, color);
  }
}

uint16_t DisplayManager::getTextWidth(const char* text, uint8_t size) {
  return strlen(text) * 6 * size;
}

uint16_t DisplayManager::getTextHeight(uint8_t size) {
  return 8 * size;
}

void DisplayManager::setVULevel(uint8_t channel, uint8_t level) {
    if (channel < 8) {
        // Aplicar un pequeño filtro para suavizar los cambios bruscos
        uint8_t filteredLevel = (vuLevels[channel] * 0.7) + (level * 0.3);
        vuLevels[channel] = filteredLevel;
        lastVuUpdate[channel] = millis();
        
        // Marcar este canal como sucio para redibujar
        markChannelDirty(channel);
        
        // Si el nivel es significativo, forzar una actualización más rápida
        if (level > 10) {
            lastFullRedraw = millis() - 4000; // Forzar redibujado más pronto
        }
    }
}

uint8_t DisplayManager::getVULevel(uint8_t channel) const {
  return (channel < NUM_ENCODERS) ? vuLevels[channel] : 0;
}

void DisplayManager::drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
  tft.drawLine(x0, y0, x1, y1, color);
}

void DisplayManager::drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  tft.drawRect(x, y, w, h, color);
}

void DisplayManager::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  tft.fillRect(x, y, w, h, color);
}

void DisplayManager::drawMessage(const char* message) {
  uint16_t msgWidth = 300;
  uint16_t msgHeight = 80;
  uint16_t msgX = (TFT_WIDTH - msgWidth) / 2;
  uint16_t msgY = (TFT_HEIGHT - msgHeight) / 2;
  
  tft.fillRect(msgX, msgY, msgWidth, msgHeight, COLOR_DARK_GRAY);
  tft.drawRect(msgX, msgY, msgWidth, msgHeight, COLOR_WHITE);
  
  if (message) {
    drawCenteredText(message, msgX + 10, msgY + 10, msgWidth - 20, msgHeight - 20, 
                     COLOR_WHITE, FONT_SIZE_MEDIUM);
  }
}

void DisplayManager::drawConfirmDialog(const char* message) {
  uint16_t dlgWidth = 350;
  uint16_t dlgHeight = 120;
  uint16_t dlgX = (TFT_WIDTH - dlgWidth) / 2;
  uint16_t dlgY = (TFT_HEIGHT - dlgHeight) / 2;
  
  tft.fillRect(dlgX, dlgY, dlgWidth, dlgHeight, COLOR_DARK_GRAY);
  tft.drawRect(dlgX, dlgY, dlgWidth, dlgHeight, COLOR_WHITE);
  
  if (message) {
    drawCenteredText(message, dlgX + 10, dlgY + 10, dlgWidth - 20, 40, 
                     COLOR_WHITE, FONT_SIZE_MEDIUM);
  }
  
  drawCenteredText("Presiona para confirmar", dlgX + 10, dlgY + 60, dlgWidth - 20, 20, 
                   COLOR_YELLOW, FONT_SIZE_SMALL);
  drawCenteredText("Volver para cancelar", dlgX + 10, dlgY + 80, dlgWidth - 20, 20, 
                   COLOR_LIGHT_GRAY, FONT_SIZE_SMALL);
}

void DisplayManager::runDisplayTest() {
  Serial.println(F("Ejecutando test de pantalla..."));
  
  const uint16_t testColors[] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_WHITE, 
                                COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA};
  const char* colorNames[] = {"ROJO", "VERDE", "AZUL", "BLANCO", 
                             "AMARILLO", "CIAN", "MAGENTA"};
  
  for (int i = 0; i < 7; i++) {
    clearScreen(testColors[i]);
    
    uint16_t textColor = (testColors[i] == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    drawCenteredText(colorNames[i], 0, TFT_HEIGHT/2 - 20, TFT_WIDTH, 40, textColor, FONT_SIZE_LARGE);
    
    unsigned long startTime = millis();
    while (millis() - startTime < 800) {
      // Espera no bloqueante
    }
  }
  
  clearScreen(COLOR_BLACK);
  drawCenteredText("TEST GEOMETRIA", 0, 20, TFT_WIDTH, 30, COLOR_WHITE, FONT_SIZE_MEDIUM);
  
  for (int i = 0; i < 5; i++) {
    uint16_t color = STUDIO_ONE_COLORS[(i) % NUM_COLORS];
    tft.drawRect(50 + i * 15, 80, 60, 40, color);
    tft.fillRect(50 + i * 15 + 5, 85, 50, 30, color);
  }
  
  for (int i = 0; i < 8; i++) {
    uint16_t color = STUDIO_ONE_COLORS[(i + 8) % NUM_COLORS];
    tft.drawCircle(60 + i * 45, 180, 20, color);
    tft.fillCircle(60 + i * 45, 180, 15, color);
  }
  
  for (int i = 0; i < 10; i++) {
    uint16_t color = STUDIO_ONE_COLORS[(i + 16) % NUM_COLORS];
    tft.drawLine(0, 220 + i * 8, TFT_WIDTH, 220 + i * 8, color);
  }
  
  unsigned long startTime = millis();
  while (millis() - startTime < 3000) {
    // Espera no bloqueante
  }
  
  clearScreen(COLOR_BLACK);
  const char* testText[] = {"Font Size 1", "Font Size 2", "Font Size 3", "Font Size 4"};
  
  for (int i = 0; i < 4; i++) {
    uint16_t color = STUDIO_ONE_COLORS[(i * 4) % NUM_COLORS];
    tft.setTextColor(color);
    tft.setTextSize(i + 1);
    tft.setCursor(10, 20 + i * 40);
    tft.print(testText[i]);
  }
  
  startTime = millis();
  while (millis() - startTime < 2000) {
    // Espera no bloqueante
  }
  
  clearScreen(COLOR_BLACK);
  Serial.println(F("Test de pantalla completado"));
}

void DisplayManager::showDiagnostics() {
  clearScreen(COLOR_BLACK);
  
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(FONT_SIZE_MEDIUM);
  
  tft.setCursor(10, 20);
  tft.print("DIAGNOSTICOS PANTALLA");
  
  tft.setCursor(10, 50);
  tft.print("Resolucion: ");
  tft.print(TFT_WIDTH);
  tft.print("x");
  tft.print(TFT_HEIGHT);
  
  tft.setCursor(10, 80);
  tft.print("Brillo: ");
  tft.print(currentBrightness);
  tft.print("%");
  
  tft.setCursor(10, 110);
  tft.print("Orientacion: ");
  tft.print((int)currentOrientation);
  
  tft.setCursor(10, 140);
  tft.print("Inicializada: ");
  tft.print(initialized ? "Si" : "No");
  
  tft.setCursor(10, 180);
  tft.print("Test Brillo:");
  for (int i = 0; i < 10; i++) {
    uint8_t gray = map(i, 0, 9, 0, 255);
    uint16_t grayColor = ((gray & 0xF8) << 8) | ((gray & 0xFC) << 3) | (gray >> 3);
    tft.fillRect(10 + i * 40, 200, 35, 20, grayColor);
  }
  
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    // Espera no bloqueante
  }
}

void DisplayManager::drawMenuItem(const char* text, uint16_t y, bool selected, bool editing) {
  char buffer[50];
  strncpy(buffer, text, sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';
  
  uint16_t bgColor = selected ? COLOR_DARK_GRAY : COLOR_BLACK;
  if (editing) bgColor = COLOR_BLUE;
  
  fillRect(15, y - 5, TFT_WIDTH - 30, 25, bgColor);
  
  if (selected) {
    drawRect(15, y - 5, TFT_WIDTH - 30, 25, editing ? COLOR_YELLOW : COLOR_WHITE);
  }
  
  uint16_t textColor = editing ? COLOR_YELLOW : COLOR_WHITE;
  tft.setTextColor(textColor);
  tft.setTextSize(FONT_SIZE_MEDIUM);
  tft.setCursor(20, y);
  tft.print(buffer);
  
  if (selected) {
    tft.setCursor(5, y);
    tft.setTextColor(COLOR_WHITE);
    tft.print("▶");
  }
}

void DisplayManager::drawMenuValue(const char* value, uint16_t x, uint16_t y, bool editing) {
  char buffer[50];
  strncpy(buffer, value, sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';
  
  uint16_t color = editing ? COLOR_YELLOW : COLOR_CYAN;
  tft.setTextColor(color);
  tft.setTextSize(FONT_SIZE_MEDIUM);
  
  uint16_t textW = getTextWidth(buffer, FONT_SIZE_MEDIUM);
  uint16_t drawX = x - textW;
  
  fillRect(drawX - 5, y - 5, textW + 10, 25, COLOR_BLACK);
  tft.setCursor(drawX, y);
  tft.print(buffer);
}

void DisplayManager::benchmarkDisplay() {
  Serial.println(F("Ejecutando benchmark de pantalla..."));
  
  unsigned long startTime, endTime;
  uint32_t operations;
  
  startTime = millis();
  operations = 0;
  while (millis() - startTime < 1000) {
    clearScreen(COLOR_RED);
    clearScreen(COLOR_BLUE);
    operations += 2;
  }
  endTime = millis();
  
  Serial.print(F("Llenados por segundo: "));
  Serial.println(operations);
  
  clearScreen(COLOR_BLACK);
  startTime = millis();
  operations = 0;
  
  while (millis() - startTime < 1000) {
    for (int i = 0; i < 100; i++) {
      tft.drawPixel(random(TFT_WIDTH), random(TFT_HEIGHT), random(0x10000));
      operations++;
    }
  }
  
  Serial.print(F("Pixeles por segundo: "));
  Serial.println(operations);
  
  clearScreen(COLOR_BLACK);
  startTime = millis();
  operations = 0;
  
  while (millis() - startTime < 1000) {
    tft.drawLine(random(TFT_WIDTH), random(TFT_HEIGHT), 
                random(TFT_WIDTH), random(TFT_HEIGHT), 
                random(0x10000));
    operations++;
  }
  
  Serial.print(F("Lineas por segundo: "));
  Serial.println(operations);
  
  clearScreen(COLOR_BLACK);
  Serial.println(F("Benchmark completado"));
}

void DisplayManager::markChannelDirty(uint8_t channel) {
    if (channel < 8) channelDirty[channel] = true;
}

void DisplayManager::markTransportDirty() {
    transportDirty = true;
}

void DisplayManager::markMtcDirty() {
    mtcDirty = true;
}
/*void DisplayManager::updateFromMidi(const EncoderConfig encoders[NUM_ENCODERS]) {
    // Esta función debería ser llamada cuando se reciben actualizaciones MIDI
    needsFullRedraw = true;
}*/
