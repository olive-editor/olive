/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef CONFORMTASK_H
#define CONFORMTASK_H

#include "codec/decoder.h"
#include "node/project/footage/footage.h"
#include "render/audioparams.h"
#include "task/task.h"

namespace olive {

class ConformTask : public Task
{
  Q_OBJECT
public:
  ConformTask(const QString &decoder_id, const Decoder::CodecStream &stream, const AudioParams& params, const QString &output_filename);

protected:
  virtual bool Run() override;

private:
  QString decoder_id_;

  Decoder::CodecStream stream_;

  AudioParams params_;

  QString output_filename_;

};

}

#endif // CONFORMTASK_H
