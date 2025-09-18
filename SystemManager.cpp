#include "SystemManager.h"
#include "Config.h"

SystemManager::SystemManager() 
  : watchdogEnabled(false), lastDiagnosticTime(0), loopCount(0),
    midiMessagesSent(0), midiMessagesReceived(0), freeMemory(0)
{
}

SystemManager::~SystemManager() {
}

bool SystemManager::initialize() {
  // Inicializar diagnóstico
  lastDiagnosticTime = millis();
  
  return true;
}

void SystemManager::resetWatchdog() {
  // ESP32 tiene watchdog automático, no necesita reset manual
}

void SystemManager::enableWatchdog(bool enable) {
  watchdogEnabled = enable;
}

bool SystemManager::shouldRunTask(TaskPriority priority, unsigned long lastRun, unsigned long interval) {
  unsigned long currentTime = millis();
  
  // Tareas críticas siempre se ejecutan
  if (priority == PRIORITY_CRITICAL) return true;
  
  // Verificar si es tiempo de ejecutar la tarea
  if (currentTime - lastRun < interval) return false;
  
  // Para tareas de baja prioridad, verificar si el sistema está ocupado
  if (priority >= PRIORITY_LOW) {
    if (midiMessagesReceived > 10) return false;
  }
  
  return true;
}

void SystemManager::updateDiagnostics() {
  loopCount++;
  freeMemory = ESP.getFreeHeap();
  
  unsigned long currentTime = millis();
  if (currentTime - lastDiagnosticTime >= 1000) {
    Serial.print(F("Free RAM: "));
    Serial.print(freeMemory);
    Serial.print(F(" | Loops/s: "));
    Serial.print(loopCount);
    Serial.print(F(" | MIDI Out: "));
    Serial.print(midiMessagesSent);
    Serial.print(F(" | MIDI In: "));
    Serial.println(midiMessagesReceived);
    
    // Reiniciar contadores
    loopCount = 0;
    midiMessagesSent = 0;
    midiMessagesReceived = 0;
    
    lastDiagnosticTime = currentTime;
  }
}

uint16_t SystemManager::getFreeMemory() {
  return ESP.getFreeHeap();
}

void SystemManager::checkSystemHealth() {
  uint16_t freeMem = getFreeMemory();
  
  if (freeMem < 5000) {
    Serial.print(F("ADVERTENCIA: Memoria crítica: "));
    Serial.println(freeMem);
  }
  
  if (loopCount == 0 && millis() > 5000) {
    Serial.println(F("ADVERTENCIA: Posible bloqueo del sistema"));
  }
}