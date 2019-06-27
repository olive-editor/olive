#ifndef TASKMANAGER_H
#define TASKMANAGER_H


class TaskManager
{
public:
  TaskManager();

  void AddTask();

private:
  int maximum_task_count_;

};

namespace olive {
extern TaskManager task_manager;
}

#endif // TASKMANAGER_H
