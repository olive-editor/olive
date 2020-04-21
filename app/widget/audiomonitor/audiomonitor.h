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

#ifndef AUDIOMONITORWIDGET_H
#define AUDIOMONITORWIDGET_H

#include <QFile>
#include <QTimer>
#include <QWidget>

#include "common/define.h"
#include "render/audioparams.h"

OLIVE_NAMESPACE_ENTER

class AudioMonitor : public QWidget
{
  Q_OBJECT
public:
  AudioMonitor(QWidget* parent = nullptr);

  virtual ~AudioMonitor() override;

public slots:
  void SetParams(const AudioRenderingParams& params);

  void OutputDeviceSet(const QString& filename, qint64 offset, int playback_speed);

  void Stop();

protected:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;

private:
  void UpdateValuesFromFile(QVector<double> &v);

  void PushValue(const QVector<double>& v);

  QVector<double> GetAverages() const;

  AudioRenderingParams params_;

  QFile file_;
  qint64 last_time_;

  int playback_speed_;

  QVector< QVector<double> > values_;
  QVector<bool> peaked_;

  QPixmap cached_background_;
  int cached_channels_;

  QTimer update_timer_;

};

OLIVE_NAMESPACE_EXIT

#endif // AUDIOMONITORWIDGET_H
