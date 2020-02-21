#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <QObject>

class MemoryManager : public QObject
{
  Q_OBJECT
public:
  MemoryManager(QObject* parent = nullptr);

  static void CreateInstance();
  static MemoryManager* instance();
  static void DestroyInstance();

  static bool ShouldFreeMemory();

  bool RegisterMemory();

signals:
  void FreeMemory();

private:
  static uint64_t GetAvailableMemory();

  static MemoryManager* instance_;

  static const uint64_t minimum_available_memory_;

};

#endif // MEMORYMANAGER_H
