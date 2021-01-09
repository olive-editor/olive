/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef TRACKVIEW_H
#define TRACKVIEW_H

#include <QScrollArea>
#include <QSplitter>

#include "node/output/track/tracklist.h"
#include "trackviewitem.h"
#include "trackviewsplitter.h"

namespace olive {

class TrackView : public QScrollArea
{
  Q_OBJECT
public:
  TrackView(Qt::Alignment vertical_alignment = Qt::AlignTop,
            QWidget* parent = nullptr);

  void ConnectTrackList(TrackList* list);
  void DisconnectTrackList();

protected:
  virtual void resizeEvent(QResizeEvent *e) override;

private:
  TrackList* list_;

  TrackViewSplitter* splitter_;

  Qt::Alignment alignment_;

  int last_scrollbar_max_;

private slots:
  void ScrollbarRangeChanged(int min, int max);

  void TrackHeightChanged(int index, int height);

  void InsertTrack(Track* track);

  void RemoveTrack(Track* track);

};

}

#endif // TRACKVIEW_H
