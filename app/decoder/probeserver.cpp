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

#include "probeserver.h"

#include <QApplication>
#include <QDebug>
#include <QFileInfo>

#include "decoder/ffmpeg/ffmpegdecoder.h"

bool olive::ProbeMedia(Footage *f)
{
  // Check for a valid filename
  if (f->filename().isEmpty()) {
    qWarning() << QCoreApplication::translate("ProbeMedia", "Tried to probe media with an empty filename");
    return false;
  }

  // Check file exists
  if (!QFileInfo::exists(f->filename())) {
    qWarning() << QCoreApplication::translate("ProbeMedia", "Tried to probe file that doesn't exist");
    return false;
  }

  // Reset Footage state for probing
  f->Clear();

  // Create decoder instance
  FFmpegDecoder ff_dec;

  // Create list to iterate through
  QList<Decoder*> decoder_list;
  decoder_list.append(&ff_dec);

  // Pass Footage through each Decoder's probe function
  for (int i=0;i<decoder_list.size();i++) {
    if (decoder_list.at(i)->Probe(f)) {

      // TODO Some way of "attaching" the Footage to the Decoder without having to iterate through Decoders like this

      f->set_ready(true);
      return true;
    }
  }

  return false;
}
