#ifndef TASKTHREAD_H
#define TASKTHREAD_H

#include <QThread>

class Task;

class TaskThread : public QThread
{
public:
  TaskThread(Task* parent);

  virtual void run() override;

  bool result();

private:
  Task* parent_;

  bool result_;
};

#endif // TASKTHREAD_H
