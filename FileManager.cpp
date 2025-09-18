#include "FileManager.h"
#include "Config.h"

FileManager::FileManager() 
  : sdInitialized(false), sdCardPresent(false), totalSpace(0), freeSpace(0),
    loggingEnabled(false)
{
  strcpy(logFilename, "sys.log");
}

FileManager::~FileManager() {}

bool FileManager::initialize() {
  Serial.println(F("Inicializando sistema de archivos SD..."));
  
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  
  for (int retry = 0; retry < SD_RETRY_COUNT; retry++) {
    if (SD.begin(SD_CS, SPI, 4000000)) {
      sdInitialized = true;
      sdCardPresent = true;
      break;
    }
    delay(100);
    Serial.print(F("Reintento SD: "));
    Serial.println(retry + 1);
  }
  
  if (!sdInitialized) {
    Serial.println(F("ERROR: No se pudo inicializar tarjeta SD"));
    return false;
  }
  
  if (!validateSDCard() || !initializeDirectories()) {
    Serial.println(F("ERROR: Validación de SD falló"));
    return false;
  }
  
  updateSpaceInfo();
  Serial.print(F("SD inicializada - Espacio total: "));
  Serial.print(totalSpace / 1024);
  Serial.println(F(" KB"));
  
  return true;
}

bool FileManager::validateSDCard() {
  const char testFile[] = "/test.tmp";
  const char testData[] = "MACKIE_TEST";
  
  if (!writeFile(testFile, testData, strlen(testData))) {
    logError("SD write test");
    return false;
  }
  
  char readData[32];
  size_t readSize;
  if (!readFile(testFile, readData, sizeof(readData), &readSize)) {
    logError("SD read test");
    return false;
  }
  
  deleteFile(testFile);
  
  if (readSize != strlen(testData) || strcmp(readData, testData) != 0) {
    logError("SD data integrity test");
    return false;
  }
  
  return true;
}

bool FileManager::initializeDirectories() {
  const char* dirs[] = {PRESET_DIRECTORY, LOG_DIRECTORY, TEMP_DIRECTORY};
  
  for (int i = 0; i < 3; i++) {
    if (!createDirectory(dirs[i])) {
      Serial.print(F("ADVERTENCIA: No se pudo crear directorio "));
      Serial.println(dirs[i]);
    }
  }
  
  return true;
}

bool FileManager::saveConfiguration(const AppConfig& config, const EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]) {
  Serial.println(F("Guardando configuración..."));
  
  if (fileExists(CONFIG_FILENAME)) {
    createBackup(CONFIG_FILENAME);
  }
  
  ConfigFileHeader header;
  header.dataSize = sizeof(AppConfig) + sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
  header.timestamp = millis() / 1000;
  
  File configFile = SD.open(CONFIG_FILENAME, FILE_WRITE);
  if (!configFile) {
    logError("open config for write");
    return false;
  }
  
  if (!writeFileHeader(configFile, header)) {
    configFile.close();
    logError("write config header");
    return false;
  }
  
  if (configFile.write((const uint8_t*)&config, sizeof(config)) != sizeof(config)) {
    configFile.close();
    logError("write app config");
    return false;
  }
  
  size_t encoderDataSize = sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
  if (configFile.write((const uint8_t*)encoders, encoderDataSize) != encoderDataSize) {
    configFile.close();
    logError("write encoder config");
    return false;
  }
  
  configFile.close();
  
  if (!verifyFileIntegrity(CONFIG_FILENAME)) {
    Serial.println(F("ERROR: Verificación de integridad falló"));
    restoreFromBackup(CONFIG_FILENAME);
    return false;
  }
  
  logSuccess("save configuration");
  return true;
}

bool FileManager::loadConfiguration(AppConfig& config, EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]) {
  Serial.println(F("Cargando configuración..."));
  
  if (!fileExists(CONFIG_FILENAME)) {
    Serial.println(F("Archivo de configuración no existe"));
    return false;
  }
  
  if (!verifyFileIntegrity(CONFIG_FILENAME)) {
    Serial.println(F("Archivo corrupto, intentando restaurar desde backup"));
    if (!restoreFromBackup(CONFIG_FILENAME)) {
      return false;
    }
  }
  
  File configFile = SD.open(CONFIG_FILENAME, FILE_READ);
  if (!configFile) {
    logError("open config for read");
    return false;
  }
  
  ConfigFileHeader header;
  if (!readFileHeader(configFile, header)) {
    configFile.close();
    logError("read config header");
    return false;
  }
  
  if (header.version != CONFIG_VERSION) {
    Serial.println(F("Versión de configuración incompatible"));
    configFile.close();
    return false;
  }
  
  if (configFile.read((uint8_t*)&config, sizeof(config)) != sizeof(config)) {
    configFile.close();
    logError("read app config");
    return false;
  }
  
  size_t encoderDataSize = sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
  if (configFile.read((uint8_t*)encoders, encoderDataSize) != encoderDataSize) {
    configFile.close();
    logError("read encoder config");
    return false;
  }
  
  configFile.close();
  logSuccess("load configuration");
  return true;
}

// ... (resto de métodos de FileManager)

void FileManager::logError(const char* operation, const char* filename) {
  Serial.print(F("ERROR: "));
  Serial.print(operation);
  if (filename) {
    Serial.print(F(" ("));
    Serial.print(filename);
    Serial.print(F(")"));
  }
  Serial.println();
  
  if (loggingEnabled) {
    char logMsg[64];
    snprintf(logMsg, sizeof(logMsg), "ERROR: %s", operation);
    writeLogEntry(logMsg);
  }
}

void FileManager::logSuccess(const char* operation, const char* filename) {
  Serial.print(F("OK: "));
  Serial.print(operation);
  if (filename) {
    Serial.print(F(" ("));
    Serial.print(filename);
    Serial.print(F(")"));
  }
  Serial.println();
  
  if (loggingEnabled) {
    char logMsg[64];
    snprintf(logMsg, sizeof(logMsg), "OK: %s", operation);
    writeLogEntry(logMsg);
  }
}


bool FileManager::createDirectory(const char* path) {
    return SD.mkdir(path);
}

bool FileManager::writeFile(const char* filename, const void* data, size_t size) {
    File file = SD.open(filename, FILE_WRITE);
    if (!file) return false;
    
    size_t bytesWritten = file.write((const uint8_t*)data, size);
    file.close();
    
    return bytesWritten == size;
}

bool FileManager::readFile(const char* filename, void* data, size_t maxSize, size_t* actualSize) {
    File file = SD.open(filename, FILE_READ);
    if (!file) return false;
    
    size_t bytesRead = file.read((uint8_t*)data, maxSize);
    file.close();
    
    if (actualSize) *actualSize = bytesRead;
    return bytesRead > 0;
}

bool FileManager::deleteFile(const char* filename) {
    return SD.remove(filename);
}

bool FileManager::fileExists(const char* filename) {
    return SD.exists(filename);
}

void FileManager::updateSpaceInfo() {
    totalSpace = SD.totalBytes();
    freeSpace = SD.usedBytes();
}

bool FileManager::writeFileHeader(File& file, const ConfigFileHeader& header) {
    return file.write((const uint8_t*)&header, sizeof(header)) == sizeof(header);
}

bool FileManager::readFileHeader(File& file, ConfigFileHeader& header) {
    return file.read((uint8_t*)&header, sizeof(header)) == sizeof(header);
}

bool FileManager::verifyFileIntegrity(const char* filename) {
    File file = SD.open(filename, FILE_READ);
    if (!file) return false;
    
    ConfigFileHeader header;
    if (!readFileHeader(file, header)) {
        file.close();
        return false;
    }
    
    file.close();
    return header.magic == 0x4D434B52; // "MCKR"
}

void FileManager::createBackup(const char* filename) {
    String backupName = String(filename) + BACKUP_EXTENSION;
    if (fileExists(backupName.c_str())) {
        deleteFile(backupName.c_str());
    }
    
    if (fileExists(filename)) {
        File source = SD.open(filename, FILE_READ);
        File dest = SD.open(backupName.c_str(), FILE_WRITE);
        
        if (source && dest) {
            uint8_t buffer[64];
            while (source.available()) {
                size_t bytesRead = source.read(buffer, sizeof(buffer));
                dest.write(buffer, bytesRead);
            }
        }
        
        source.close();
        dest.close();
    }
}

bool FileManager::restoreFromBackup(const char* filename) {
    String backupName = String(filename) + BACKUP_EXTENSION;
    if (!fileExists(backupName.c_str())) return false;
    
    if (fileExists(filename)) {
        deleteFile(filename);
    }
    
    File source = SD.open(backupName.c_str(), FILE_READ);
    File dest = SD.open(filename, FILE_WRITE);
    
    if (!source || !dest) return false;
    
    uint8_t buffer[64];
    while (source.available()) {
        size_t bytesRead = source.read(buffer, sizeof(buffer));
        dest.write(buffer, bytesRead);
    }
    
    source.close();
    dest.close();
    return true;
}

bool FileManager::writeLogEntry(const char* message) {
    if (!loggingEnabled) return true;
    
    char logPath[64];
    snprintf(logPath, sizeof(logPath), "%s/%s", LOG_DIRECTORY, logFilename);
    
    File file = SD.open(logPath, FILE_APPEND);
    if (!file) return false;
    
    unsigned long timestamp = millis() / 1000;
    char logEntry[128];
    snprintf(logEntry, sizeof(logEntry), "[%lu] %s\n", timestamp, message);
    
    bool success = file.write((const uint8_t*)logEntry, strlen(logEntry)) == strlen(logEntry);
    file.close();
    
    return success;
}

bool FileManager::loadPreset(const char* presetName, EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]) {
    char presetPath[64];
    snprintf(presetPath, sizeof(presetPath), "%s/%s.prs", PRESET_DIRECTORY, presetName);
    
    if (!fileExists(presetPath)) return false;
    
    File file = SD.open(presetPath, FILE_READ);
    if (!file) return false;
    
    size_t bytesRead = file.read((uint8_t*)encoders, sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS);
    file.close();
    
    return bytesRead == sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
}

bool FileManager::savePreset(const char* presetName, const EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]) {
    char presetPath[64];
    snprintf(presetPath, sizeof(presetPath), "%s/%s.prs", PRESET_DIRECTORY, presetName);
    
    File file = SD.open(presetPath, FILE_WRITE);
    if (!file) return false;
    
    size_t bytesWritten = file.write((const uint8_t*)encoders, sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS);
    file.close();
    
    return bytesWritten == sizeof(EncoderConfig) * NUM_ENCODERS * NUM_BANKS;
}

bool FileManager::resetConfiguration() {
    // Reset configuration to defaults
    if (fileExists(CONFIG_FILENAME)) {
        deleteFile(CONFIG_FILENAME);
    }
    
    // Also delete backup
    String backupName = String(CONFIG_FILENAME) + BACKUP_EXTENSION;
    if (fileExists(backupName.c_str())) {
        deleteFile(backupName.c_str());
    }
    
    return true;
}

bool FileManager::checkSDHealth() {
    return sdInitialized && sdCardPresent;
}