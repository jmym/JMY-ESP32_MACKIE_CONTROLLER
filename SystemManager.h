#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include "Config.h"

class SystemManager {
private:
    bool watchdogEnabled;
    unsigned long lastDiagnosticTime;
    uint32_t loopCount;
    uint32_t midiMessagesSent;
    uint32_t midiMessagesReceived;
    uint16_t freeMemory;

public:
  SystemManager();
  ~SystemManager();
  
  bool initialize();
  void resetWatchdog();
  void enableWatchdog(bool enable);
  
  bool shouldRunTask(TaskPriority priority, unsigned long lastRun, unsigned long interval);
  void updateDiagnostics();
  
  uint16_t getFreeMemory();
  void checkSystemHealth();
  
  uint32_t getLoopCount() const { return loopCount; }
  uint32_t getMidiMessagesSent() const { return midiMessagesSent; }
  uint32_t getMidiMessagesReceived() const { return midiMessagesReceived; }
};

#endif // SYSTEM_MANAGER_H