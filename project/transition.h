/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#ifndef TRANSITION_H
#define TRANSITION_H

#include "effect.h"

enum TransitionType {
  kTransitionNone,
  kTransitionOpening,
  kTransitionClosing
};

enum TransitionInternal {
  TRANSITION_INTERNAL_CROSSDISSOLVE,
  TRANSITION_INTERNAL_LINEARFADE,
  TRANSITION_INTERNAL_EXPONENTIALFADE,
  TRANSITION_INTERNAL_LOGARITHMICFADE,
  TRANSITION_INTERNAL_CUBE,
  TRANSITION_INTERNAL_COUNT
};

class Transition;
using TransitionPtr = std::shared_ptr<Transition>;

TransitionPtr get_transition_from_meta(ClipPtr c, ClipPtr s, const EffectMeta* em);

TransitionPtr create_transition(ClipPtr c, ClipPtr s, const EffectMeta* em, long length = 0);

class Transition : public Effect {
  Q_OBJECT
public:
  Transition(ClipPtr c, ClipPtr s, const EffectMeta* em);
  virtual TransitionPtr copy(ClipPtr c, ClipPtr s);
  ClipPtr secondary_clip;
  void set_length(long l);
  long get_true_length();
  long get_length();

  ClipPtr get_opened_clip();
  ClipPtr get_closed_clip();
private slots:
  void set_length_from_slider();
private:
  long length; // used only for transitions
  EffectField* length_field;
};

#endif // TRANSITION_H
