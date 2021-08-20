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

#ifndef BLOCK_H
#define BLOCK_H

#include "node/node.h"
#include "timeline/timelinecommon.h"

namespace olive {

class TransitionBlock;

/**
 * @brief A Node that represents a block of time, also displayable on a Timeline
 */
class Block : public Node
{
  Q_OBJECT
public:
  Block();

  NODE_DEFAULT_DESTRUCTOR(Block)

  virtual QVector<CategoryID> Category() const override;

  const rational& in() const
  {
    return in_point_;
  }

  const rational& out() const
  {
    return out_point_;
  }

  void set_in(const rational& in)
  {
    in_point_ = in;
  }

  void set_out(const rational& out)
  {
    out_point_ = out;
  }

  rational length() const;
  virtual void set_length_and_media_out(const rational &length);
  virtual void set_length_and_media_in(const rational &length);

  TimeRange range() const
  {
    return TimeRange(in(), out());
  }

  Block* previous() const
  {
    return previous_;
  }

  Block* next() const
  {
    return next_;
  }

  void set_previous(Block* previous)
  {
    previous_ = previous;
  }

  void set_next(Block* next)
  {
    next_ = next;
  }

  Track* track() const
  {
    return track_;
  }

  void set_track(Track* track)
  {
    track_ = track;
  }

  bool is_enabled() const;
  void set_enabled(bool e);

  virtual void Retranslate() override;

  int index() const
  {
    return index_;
  }

  void set_index(int i)
  {
    index_ = i;
  }

  virtual void Hash(const QString& output, QCryptographicHash &hash, const rational &time, const VideoParams& video_params) const override;

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element = -1, InvalidateCacheOptions options = InvalidateCacheOptions()) override;

  static const QString kLengthInput;
  static const QString kEnabledInput;

public slots:

signals:
  void EnabledChanged();

  void LengthChanged();

  void PreviewChanged();

protected:
  virtual void InputValueChangedEvent(const QString& input, int element) override;

  bool HashPassthrough(const QString &input, const QString& output, QCryptographicHash &hash, const rational &time, const VideoParams& video_params) const;

  Block* previous_;
  Block* next_;

private:
  void set_length_internal(const rational &length);

  rational in_point_;
  rational out_point_;
  Track* track_;
  int index_;

  rational last_length_;

};

}

#endif // BLOCK_H
