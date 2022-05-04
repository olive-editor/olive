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

#ifndef TRANSITIONBLOCK_H
#define TRANSITIONBLOCK_H

#include "node/block/block.h"

namespace olive {

class ClipBlock;

class TransitionBlock : public Block
{
  Q_OBJECT
public:
  TransitionBlock();

  virtual void Retranslate() override;

  rational in_offset() const;
  rational out_offset() const;

  /**
   * @brief Return the "middle point" of the transition, relative to the transition
   *
   * Used to calculate in/out offsets.
   *
   * 0 means the center of the transition is right in the middle and the in and out offsets will
   * be equal.
   */
  rational offset_center() const;
  void set_offset_center(const rational &r);

  void set_offsets_and_length(const rational &in_offset, const rational &out_offset);

  bool is_dual_transition() const
  {
    return connected_out_block() && connected_in_block();
  }

  Block* connected_out_block() const;
  Block* connected_in_block() const;

  double GetTotalProgress(const double &time) const;
  double GetOutProgress(const double &time) const;
  double GetInProgress(const double &time) const;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element = -1, InvalidateCacheOptions options = InvalidateCacheOptions()) override;

  static const QString kOutBlockInput;
  static const QString kInBlockInput;
  static const QString kCurveInput;
  static const QString kCenterInput;

protected:
  virtual void ShaderJobEvent(const NodeValueRow &value, ShaderJob& job) const;

  virtual void SampleJobEvent(SampleBufferPtr from_samples, SampleBufferPtr to_samples, SampleBufferPtr out_samples, double time_in) const;

  double TransformCurve(double linear) const;

  virtual void InputConnectedEvent(const QString& input, int element, Node *output) override;

  virtual void InputDisconnectedEvent(const QString& input, int element, Node *output) override;

  virtual TimeRange InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual TimeRange OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual void Hash(QCryptographicHash& hash, const NodeGlobals &globals, const VideoParams& video_params) const override;

private:
  enum CurveType {
    kLinear,
    kExponential,
    kLogarithmic
  };

  double GetInternalTransitionTime(const double &time) const;

  void InsertTransitionTimes(AcceleratedJob* job, const double& time) const;

  ClipBlock* connected_out_block_;

  ClipBlock* connected_in_block_;

};

}

#endif // TRANSITIONBLOCK_H
