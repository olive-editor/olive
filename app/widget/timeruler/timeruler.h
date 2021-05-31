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

#ifndef TIMERULER_H
#define TIMERULER_H

#include <QTimer>
#include <QWidget>

#include "common/timerange.h"
#include "seekablewidget.h"
#include "render/playbackcache.h"

namespace olive {

class TimeRuler : public SeekableWidget
{
  Q_OBJECT
public:
  TimeRuler(bool text_visible = true, bool cache_status_visible = false, QWidget* parent = nullptr);

  void SetCenteredText(bool c);

  void SetPlaybackCache(PlaybackCache* cache);

protected:
  virtual void paintEvent(QPaintEvent* e) override;

  virtual void TimebaseChangedEvent(const rational& tb) override;

private:
  void UpdateHeight();

  int CacheStatusHeight() const;

  int cache_status_height_;

  int minimum_gap_between_lines_;

  bool text_visible_;

  bool centered_text_;

  double timebase_flipped_dbl_;

  bool show_cache_status_;

  PlaybackCache* playback_cache_;

private slots:
  void ShowContextMenu();

};

}

#endif // TIMERULER_H
