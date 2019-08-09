/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "probe.h"

#include <QFileInfo>

#include "decoder/decoder.h"

ProbeTask::ProbeTask(FootagePtr footage) :
  footage_(footage)
{
  QString base_filename = QFileInfo(footage_->filename()).fileName();

  set_text(tr("Probing \"%1\"").arg(base_filename));
}

bool ProbeTask::Action()
{
  footage_->LockDeletes();

  Decoder::ProbeMedia(footage_.get());

  footage_->UnlockDeletes();

  return true;
}
