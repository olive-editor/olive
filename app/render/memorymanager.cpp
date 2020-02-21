#include "memorymanager.h"

#include <QDebug>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

MemoryManager* MemoryManager::instance_ = nullptr;

// FIXME: Hardcoded, should be a preference
const uint64_t MemoryManager::minimum_available_memory_ = 2147483648;

MemoryManager::MemoryManager(QObject* parent) :
  QObject(parent)
{
}

void MemoryManager::CreateInstance()
{
  instance_ = new MemoryManager();
}

MemoryManager *MemoryManager::instance()
{
  return instance_;
}

void MemoryManager::DestroyInstance()
{
  delete instance_;
  instance_ = nullptr;
}

bool MemoryManager::ShouldFreeMemory()
{
  return (GetAvailableMemory() < minimum_available_memory_);
}

bool MemoryManager::RegisterMemory()
{
  if (ShouldFreeMemory()) {
    emit FreeMemory();

    return true;
  }

  return false;
}

uint64_t MemoryManager::GetAvailableMemory()
{
#ifdef Q_OS_WINDOWS
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  GlobalMemoryStatusEx(&status);
  return status.ullAvailPhys;
#else
  long pages = sysconf(_SC_AVPHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  return pages * page_size;
#endif
}
