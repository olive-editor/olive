#ifndef CONFORMTASK_H
#define CONFORMTASK_H

#include "project/item/footage/audiostream.h"
#include "render/audioparams.h"
#include "task/task.h"

class ConformTask : public Task
{
public:
  ConformTask(AudioStreamPtr stream, const AudioRenderingParams& params);

protected:
  virtual void Action() override;

private:
  AudioStreamPtr stream_;

  AudioRenderingParams params_;

};

#endif // CONFORMTASK_H
