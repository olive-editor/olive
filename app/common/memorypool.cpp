#include "memorypool.h"

OLIVE_NAMESPACE_ENTER

size_t memory_pool_consumption = 0;
QMutex memory_pool_consumption_lock;

bool MemoryPoolLimitReached()
{
  QMutexLocker locker(&memory_pool_consumption_lock);
  return (memory_pool_consumption >= 2147483648);
}

OLIVE_NAMESPACE_EXIT
