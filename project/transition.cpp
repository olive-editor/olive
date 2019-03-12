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

#include "transition.h"

#include "mainwindow.h"
#include "clip.h"
#include "sequence.h"
#include "debug.h"

#include "io/clipboard.h"

#include "effects/internal/crossdissolvetransition.h"
#include "effects/internal/linearfadetransition.h"
#include "effects/internal/exponentialfadetransition.h"
#include "effects/internal/logarithmicfadetransition.h"
#include "effects/internal/cubetransition.h"

#include "ui/labelslider.h"

#include "panels/panels.h"
#include "panels/timeline.h"

#include <QMessageBox>
#include <QCoreApplication>

Transition::Transition(Clip *c, Clip *s, const EffectMeta* em) :
  Effect(c, em),
  secondary_clip(s)
{
  EffectRow* length_row = new EffectRow(this, tr("Length"), false, false);
  length_field = new DoubleField(length_row, "length");
  length_field->SetDefault(30);
  length_field->SetMinimum(0);
  length_field->SetDisplayType(LabelSlider::LABELSLIDER_FRAMENUMBER);
  length_field->SetFrameRate(parent_clip->sequence == nullptr ?
                               parent_clip->cached_frame_rate() : parent_clip->sequence->frame_rate);
  connect(length_field, SIGNAL(changed()), this, SLOT(set_length_from_slider()));
}

TransitionPtr Transition::copy(Clip *c, Clip *s) {
  return Transition::Create(c, s, meta, get_true_length());
}

void Transition::save(QXmlStreamWriter &stream) {
  stream.writeAttribute("length", QString::number(get_true_length()));
  Effect::save(stream);
}

void Transition::set_length(int l) {
  length_field->SetValueAt(0, l);
}

int Transition::get_true_length() {
  return length_field->GetValueAt(0).toInt();
}

int Transition::get_length() {
  if (secondary_clip != nullptr) {
    return get_true_length() * 2;
  }
  return get_true_length();
}

Clip* Transition::get_opened_clip() {
  if (parent_clip->opening_transition.get() == this) {
    return parent_clip;
  } else if (secondary_clip != nullptr && secondary_clip->opening_transition.get() == this) {
    return secondary_clip;
  }
  return nullptr;
}

Clip* Transition::get_closed_clip() {
  if (parent_clip->closing_transition.get() == this) {
    return parent_clip;
  } else if (secondary_clip != nullptr && secondary_clip->closing_transition.get() == this) {
    return secondary_clip;
  }
  return nullptr;
}

void Transition::set_length_from_slider() {
  update_ui(false);
}

TransitionPtr Transition::CreateFromMeta(Clip* c, Clip* s, const EffectMeta* em) {
  if (!em->filename.isEmpty()) {
    // load effect from file
    return TransitionPtr(new Transition(c, s, em));
  } else if (em->internal >= 0 && em->internal < TRANSITION_INTERNAL_COUNT) {
    // must be an internal effect
    switch (em->internal) {
    case TRANSITION_INTERNAL_CROSSDISSOLVE: return TransitionPtr(new CrossDissolveTransition(c, s, em));
    case TRANSITION_INTERNAL_LINEARFADE: return TransitionPtr(new LinearFadeTransition(c, s, em));
    case TRANSITION_INTERNAL_EXPONENTIALFADE: return TransitionPtr(new ExponentialFadeTransition(c, s, em));
    case TRANSITION_INTERNAL_LOGARITHMICFADE: return TransitionPtr(new LogarithmicFadeTransition(c, s, em));
    //case TRANSITION_INTERNAL_CUBE: return TransitionPtr(new CubeTransition(c, s, em));
    }
  } else {
    qCritical() << "Invalid transition data";
    QMessageBox::critical(olive::MainWindow,
                          QCoreApplication::translate("transition", "Invalid transition"),
                          QCoreApplication::translate("transition", "No candidate for transition '%1'. This transition may be corrupt. Try reinstalling it or Olive.").arg(em->name)
                          );
  }
  return nullptr;
}

TransitionPtr Transition::Create(Clip* c, Clip* s, const EffectMeta* em, long length) {
  TransitionPtr t(CreateFromMeta(c, s, em));
  if (t != nullptr) {
    if (length > 0) {
      t->set_length(length);
    }
    return t;
  }
  return nullptr;
}
