#ifndef PROBE_H
#define PROBE_H

#include "project/item/footage/footage.h"
#include "task/task.h"

class ProbeTask : public Task
{
  Q_OBJECT
public:
  ProbeTask(Footage* footage);

  virtual bool Action() override;

private:
  Footage* footage_;
};

#endif // PROBE_H
