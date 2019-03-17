/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#ifndef PREVIEWGENERATOR_H
#define PREVIEWGENERATOR_H

#include <QThread>
#include <QSemaphore>
#include <QDir>

#include "project/footage.h"
#include "project/media.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class PreviewGenerator : public QThread
{
  Q_OBJECT
public:
  PreviewGenerator(Media*);
  void run();
  void cancel();

  static void AnalyzeMedia(Media*);
private:
  void parse_media();
  bool retrieve_preview(const QString &hash);
  void generate_waveform();
  void finalize_media();
  void invalidate_media(const QString& error_msg);
  QString get_thumbnail_path(const QString &hash, const FootageStream &ms);
  QString get_waveform_path(const QString& hash, const FootageStream &ms);

  AVFormatContext* fmt_ctx_;
  Media* media_;
  Footage* footage_;
  bool retrieve_duration_;
  bool contains_still_image_;
  bool cancelled_;
  QDir data_dir_;
  bool append_;
};

#endif // PREVIEWGENERATOR_H
