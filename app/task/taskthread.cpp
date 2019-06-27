#include "taskthread.h"

#include "task/task.h"

TaskThread::TaskThread(Task *parent) :
  parent_(parent),
  result_(false)
{
}

void TaskThread::run()
{
  result_ = parent_->Action();
}

bool TaskThread::result()
{
  return result_;
}
