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

#ifndef EXPORTFORMAT_H
#define EXPORTFORMAT_H

#include <QList>
#include <QString>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class ExportFormat {
public:
  ExportFormat(const QString& name, const QString& extension, const QString& encoder, const QList<int>& vcodecs, const QList<int>& acodecs) :
    name_(name),
    extension_(extension),
    encoder_(encoder),
    video_codecs_(vcodecs),
    audio_codecs_(acodecs)
  {
  }

  const QString& name() const {return name_;}
  const QString& extension() const {return extension_;}
  const QString& encoder() const {return encoder_;}
  const QList<int>& video_codecs() const {return video_codecs_;}
  const QList<int>& audio_codecs() const {return audio_codecs_;}

private:
  QString name_;
  QString extension_;
  QString encoder_;
  QList<int> video_codecs_;
  QList<int> audio_codecs_;

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTFORMAT_H
