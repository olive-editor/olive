#include "project/sequence/clip.h"

namespace olive {

namespace {
  int __next_id = 0;
}

Clip::Clip(int in, int out):
  in_(in), out_(out), id_(__next_id++) {

}

void Clip::setTime(int in, int out) {
  int oldIn = in_;
  int oldOut = out_;
  in_ = in;
  out_ = out;
  emit onDidChangeTime(oldIn, oldOut);
}

int Clip::in() const { return in_; }
int Clip::out() const { return out_; }
int Clip::id() const { return id_; }

bool Clip::operator<(const Clip& rhs) const {
  return id_ < rhs.id_;
}

}