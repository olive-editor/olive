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

#ifndef MEDIAICONSERVICE_H
#define MEDIAICONSERVICE_H

#include <QObject>
#include <QTimer>
#include <QMutex>

#include "project/media.h"

enum IconType {
  ICON_TYPE_VIDEO,
  ICON_TYPE_AUDIO,
  ICON_TYPE_IMAGE,
  ICON_TYPE_LOADING,
  ICON_TYPE_ERROR
};

class MediaIconService : public QObject {
  Q_OBJECT
public:
  MediaIconService();
public slots:
  void SetMediaIcon(Media* media, IconType icon_type);
signals:
  void IconChanged();
private slots:
  void AnimationUpdate();
private:
  int throbber_animation_frame_;
  QVector<Media*> throbber_items_;
  QTimer throbber_animator_;
  QPixmap throbber_pixmap_;
  QMutex throbber_lock_;
};

namespace olive {
extern std::unique_ptr<MediaIconService> media_icon_service;
}

#endif // MEDIAICONSERVICE_H
