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

#define TA_NO_TRANSITION 0
#define TA_OPENING_TRANSITION 1
#define TA_CLOSING_TRANSITION 2

#define TRANSITION_INTERNAL_CROSSDISSOLVE 0
#define TRANSITION_INTERNAL_LINEARFADE 1
#define TRANSITION_INTERNAL_EXPONENTIALFADE	2
#define TRANSITION_INTERNAL_LOGARITHMICFADE 3
#define TRANSITION_INTERNAL_CUBE 4
#define TRANSITION_INTERNAL_COUNT 5

int create_transition(Clip* c, Clip* s, const EffectMeta* em, long length = -1);

class Transition : public Effect {
	Q_OBJECT
public:
	Transition(Clip* c, Clip* s, const EffectMeta* em);
	int copy(Clip* c, Clip* s);
	Clip* secondary_clip;
	void set_length(long l);
	long get_true_length();
	long get_length();
private slots:
	void set_length_from_slider();
private:
	long length; // used only for transitions
	EffectField* length_field;
};

#endif // TRANSITION_H
