#ifndef INDEXTASK_H
#define INDEXTASK_H

#include "project/item/footage/footage.h"
#include "task/task.h"

class IndexTask : public Task
{
public:
  IndexTask(StreamPtr stream);

protected:
  virtual void Action() override;

private:
  StreamPtr stream_;

};

#endif // INDEXTASK_H
