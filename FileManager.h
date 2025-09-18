#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "Config.h"
#include <SD.h>
#include <SPI.h>

#define CONFIG_FILENAME        "config.cfg"
#define BACKUP_EXTENSION       ".bak"
#define PRESET_DIRECTORY       "/presets"
#define LOG_DIRECTORY          "/logs"
#define TEMP_DIRECTORY         "/temp"

#define MAX_FILENAME_LENGTH    12
#define MAX_PRESET_NAME        12
#define CONFIG_VERSION         1
#define SD_RETRY_COUNT         3

struct ConfigFileHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t checksum;
  uint32_t dataSize;
  uint32_t timestamp;
  char description[32];
  
  ConfigFileHeader() : magic(0x4D434B52), version(CONFIG_VERSION), 
                      checksum(0), dataSize(0), timestamp(0) {
    strcpy(description, "Mackie MIDI Config");
  }
};

struct FileInfo {
  char name[MAX_FILENAME_LENGTH];
  uint32_t size;
  uint32_t timestamp;
  bool isDirectory;
  
  FileInfo() : size(0), timestamp(0), isDirectory(false) {
    name[0] = '\0';
  }
};

class FileManager {
private:
  bool sdInitialized;
  bool sdCardPresent;
  uint32_t totalSpace;
  uint32_t freeSpace;
  
  uint8_t workBuffer[64];
  char pathBuffer[32];
  
  bool initializeDirectories();
  bool validateSDCard();
  uint16_t calculateChecksum(const void* data, size_t size);
  bool writeFileHeader(File& file, const ConfigFileHeader& header);
  bool readFileHeader(File& file, ConfigFileHeader& header);
  bool verifyFileIntegrity(const char* filename);
  
  void createBackup(const char* filename);
  bool restoreFromBackup(const char* filename);
  
  void logError(const char* operation, const char* filename = nullptr);
  void logSuccess(const char* operation, const char* filename = nullptr);

public:
  FileManager();
  ~FileManager();
  
  bool initialize();
  bool checkSDHealth();
  void reinitializeSD();
  
  bool saveConfiguration(const AppConfig& config, 
                        const EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]);
  bool loadConfiguration(AppConfig& config, 
                        EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]);
  bool resetConfiguration();
  
  bool savePreset(const char* presetName, 
                 const EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]);
  bool loadPreset(const char* presetName, 
                 EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]);
  bool deletePreset(const char* presetName);
  bool renamePreset(const char* oldName, const char* newName);
  
  uint8_t listPresets(char presetNames[][MAX_PRESET_NAME], uint8_t maxPresets);
  uint8_t listDirectory(const char* path, FileInfo* files, uint8_t maxFiles);
  bool fileExists(const char* filename);
  uint32_t getFileSize(const char* filename);
  
  bool writeFile(const char* filename, const void* data, size_t size);
  bool readFile(const char* filename, void* data, size_t maxSize, size_t* actualSize = nullptr);
  bool appendToFile(const char* filename, const void* data, size_t size);
  bool deleteFile(const char* filename);
  bool copyFile(const char* source, const char* dest);
  
  bool createDirectory(const char* path);
  bool deleteDirectory(const char* path);
  bool directoryExists(const char* path);
 //  bool writeFile(const char* filename, const void* data, size_t size);
  //  bool readFile(const char* filename, void* data, size_t maxSize, size_t* actualSize = nullptr);
  //  bool deleteFile(const char* filename);
  //  bool fileExists(const char* filename);
    //void updateSpaceInfo();
  //  bool writeFileHeader(File& file, const ConfigFileHeader& header);
  //  bool readFileHeader(File& file, ConfigFileHeader& header);
  //  bool verifyFileIntegrity(const char* filename);
  //  void createBackup(const char* filename);
  //  bool restoreFromBackup(const char* filename);
   // bool writeLogEntry(const char* message);
  //  bool loadPreset(const char* presetName, EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]);
  //  bool savePreset(const char* presetName, const EncoderConfig encoders[NUM_ENCODERS][NUM_BANKS]);
 //   bool resetConfiguration();
 //   bool checkSDHealth();

  uint32_t getTotalSpace() const { return totalSpace; }
  uint32_t getFreeSpace() const { return freeSpace; }
  uint32_t getUsedSpace() const { return totalSpace - freeSpace; }
  uint8_t getUsagePercent() const { 
    return totalSpace > 0 ? (100 * getUsedSpace()) / totalSpace : 0; 
  }
  
  bool isSDPresent() const { return sdCardPresent; }
  bool isInitialized() const { return sdInitialized; }
  
  void updateSpaceInfo();
  bool repairFileSystem();
  void clearTempFiles();
  
  bool enableLogging(bool enable);
  bool writeLogEntry(const char* message);
  void printSystemInfo() const;
  void printDirectoryTree(const char* path = "/") const;
  
  bool exportConfiguration(const char* filename);
  bool importConfiguration(const char* filename);
  
  bool isValidFilename(const char* filename) const;
  bool isValidPresetName(const char* name) const;
  void sanitizeFilename(char* filename) const;
  
  bool recoverConfiguration();
  bool verifyAllFiles();
  uint8_t scanForCorruption();
  
private:
  bool loggingEnabled;
  char logFilename[MAX_FILENAME_LENGTH];
};

#endif // FILE_MANAGER_H