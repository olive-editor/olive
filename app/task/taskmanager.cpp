#include "taskmanager.h"

#include <QThread>

TaskManager olive::task_manager;

TaskManager::TaskManager()
{
  maximum_task_count_ = QThread::idealThreadCount();
}
