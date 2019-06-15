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

#ifndef PROXYGENERATOR_H
#define PROXYGENERATOR_H

#include <QThread>
#include <QVector>
#include <QMutex>
#include <QWaitCondition>

#include "project/media.h"

struct ProxyInfo {
  Media* media;
  double size_multiplier;
  int codec_type;
  QString path;
};

class ProxyGenerator : public QThread {
  Q_OBJECT
public:
  ProxyGenerator();
  void run();
  void queue(const ProxyInfo& info);
  void cancel();
  double get_proxy_progress(Media *f);
private:
  // queue of footage to process proxies for
  QVector<ProxyInfo> proxy_queue;

  // threading objects
  QWaitCondition waitCond;
  QMutex mutex;

  // set to true if you want to permanently close ProxyGenerator
  bool cancelled;

  // set to true if you want to abort the footage currently being processed
  bool skip;

  // stores progress in percent of proxy currently being processed
  double current_progress;

  // function that performs the actual transcode
  void transcode(const ProxyInfo& info);
};

namespace olive {
  // proxy generator is a global omnipotent entity
  extern ProxyGenerator proxy_generator;
}

#endif // PROXYGENERATOR_H
