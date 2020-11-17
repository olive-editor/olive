#include "memorypool.h"

namespace olive {

size_t memory_pool_consumption = 0;
QMutex memory_pool_consumption_lock;

bool MemoryPoolLimitReached()
{
  QMutexLocker locker(&memory_pool_consumption_lock);
  return (memory_pool_consumption >= 2147483648);
}

}
