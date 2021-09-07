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

#ifndef CLIPBLOCK_H
#define CLIPBLOCK_H

#include "audio/audiovisualwaveform.h"
#include "node/block/block.h"

namespace olive {

/**
 * @brief Node that represents a block of Media
 */
class ClipBlock : public Block
{
  Q_OBJECT
public:
  ClipBlock();

  NODE_DEFAULT_DESTRUCTOR(ClipBlock)

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Description() const override;

  virtual void set_length_and_media_out(const rational &length) override;
  virtual void set_length_and_media_in(const rational &length) override;

  rational media_in() const;
  void set_media_in(const rational& media_in);

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element, InvalidateCacheOptions options) override;

  virtual TimeRange InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual TimeRange OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual void Value(const QString& output, const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual void Retranslate() override;

  virtual void Hash(const QString& output, QCryptographicHash &hash, const NodeGlobals &globals, const VideoParams& video_params) const override;

  double speed() const
  {
    return GetStandardValue(kSpeedInput).toDouble();
  }

  bool reverse() const
  {
    return GetStandardValue(kReverseInput).toBool();
  }

  TransitionBlock* in_transition()
  {
    return in_transition_;
  }

  void set_in_transition(TransitionBlock* t)
  {
    in_transition_ = t;
  }

  TransitionBlock* out_transition()
  {
    return out_transition_;
  }

  void set_out_transition(TransitionBlock* t)
  {
    out_transition_ = t;
  }

  const QVector<Block*>& block_links() const
  {
    return block_links_;
  }

  AudioVisualWaveform& waveform()
  {
    return waveform_;
  }

  static const QString kBufferIn;
  static const QString kMediaInInput;
  static const QString kSpeedInput;
  static const QString kReverseInput;

protected:
  virtual void LinkChangeEvent() override;

  virtual void InputValueChangedEvent(const QString &input, int element) override;

private:
  rational SequenceToMediaTime(const rational& sequence_time, bool ignore_reverse = false) const;

  rational MediaToSequenceTime(const rational& media_time) const;

  QVector<Block*> block_links_;

  TransitionBlock* in_transition_;
  TransitionBlock* out_transition_;

private:
  AudioVisualWaveform waveform_;

  rational last_media_in_;


};

}

#endif // TIMELINEBLOCK_H
