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

#ifndef TRANSITIONBLOCK_H
#define TRANSITIONBLOCK_H

#include "node/block/block.h"

namespace olive {

class TransitionBlock : public Block
{
  Q_OBJECT
public:
  TransitionBlock();

  virtual Type type() const override;

  NodeInput* out_block_input() const;
  NodeInput* in_block_input() const;

  virtual void Retranslate() override;

  rational in_offset() const;
  rational out_offset() const;

  Block* connected_out_block() const;
  Block* connected_in_block() const;

  double GetTotalProgress(const double &time) const;
  double GetOutProgress(const double &time) const;
  double GetInProgress(const double &time) const;

  virtual void Hash(QCryptographicHash& hash, const rational &time) const override;

  virtual NodeValueTable Value(NodeValueDatabase &value) const override;

  static TransitionBlock* GetBlockInTransition(Block* block);

  static TransitionBlock* GetBlockOutTransition(Block* block);

protected:
  virtual void ShaderJobEvent(NodeValueDatabase &value, ShaderJob& job) const;

  virtual void SampleJobEvent(SampleBufferPtr from_samples, SampleBufferPtr to_samples, SampleBufferPtr out_samples, double time_in) const;

  double TransformCurve(double linear) const;

private:
  enum CurveType {
    kLinear,
    kExponential,
    kLogarithmic
  };

  double GetInternalTransitionTime(const double &time) const;

  void InsertTransitionTimes(AcceleratedJob* job, const double& time) const;

  NodeInput* out_block_input_;

  NodeInput* in_block_input_;

  NodeInput* curve_input_;

  Block* connected_out_block_;

  Block* connected_in_block_;

private slots:
  void OutBlockConnected(Node* node);

  void OutBlockDisconnected();

  void InBlockConnected(Node* node);

  void InBlockDisconnected();

};

}

#endif // TRANSITIONBLOCK_H
