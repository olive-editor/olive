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

#ifndef TRACKVIEWSPLITTER_H
#define TRACKVIEWSPLITTER_H

#include <QSplitter>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class TrackViewSplitterHandle : public QSplitterHandle
{
  Q_OBJECT
public:
  TrackViewSplitterHandle(Qt::Orientation orientation, QSplitter *parent);

protected:
  virtual void	mousePressEvent(QMouseEvent *e) override;
  virtual void	mouseMoveEvent(QMouseEvent *e) override;
  virtual void	mouseReleaseEvent(QMouseEvent *e) override;

  virtual void	paintEvent(QPaintEvent *e) override;

private:
  int drag_y_;

  bool dragging_;
};

class TrackViewSplitter : public QSplitter
{
  Q_OBJECT
public:
  TrackViewSplitter(Qt::Alignment vertical_alignment, QWidget* parent = nullptr);

  void HandleReceiver(TrackViewSplitterHandle* h, int diff);

  void SetHeightWithSizes(QList<int> sizes);

  void Insert(int index, int height, QWidget* item);
  void Remove(int index);

  void SetSpacerHeight(int height);

public slots:
  void SetTrackHeight(int index, int h);

signals:
  void TrackHeightChanged(int index, int height);

protected:
  virtual QSplitterHandle *createHandle() override;

private:
  Qt::Alignment alignment_;

  int spacer_height_;

};

OLIVE_NAMESPACE_EXIT

#endif // TRACKVIEWSPLITTER_H
