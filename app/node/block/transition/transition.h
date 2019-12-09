#ifndef TRANSITIONBLOCK_H
#define TRANSITIONBLOCK_H

#include "node/block/block.h"

class TransitionBlock : public Block
{
public:
  TransitionBlock();

  virtual Type type() const override;
  virtual QString Category() const override;

  NodeInput* out_block_input() const;

  NodeInput* in_block_input() const;

  virtual void Retranslate() override;

  virtual void set_length(const rational &length) override;
  virtual void set_length_and_media_in(const rational &length) override;

  void set_in_offset(const rational& in_offset);
  void set_out_offset(const rational& out_offset);

private:
  void RecalculateLength();

  NodeInput* out_block_input_;

  NodeInput* in_block_input_;

  rational in_offset_;

  rational out_offset_;

};

#endif // TRANSITIONBLOCK_H
