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

#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <QVector>

#include "project/marker.h"
#include "project/selection.h"

class Clip;
class Transition;
class Media;

struct Sequence {
	Sequence();
	~Sequence();
	Sequence* copy();
	QString name;
	void getTrackLimits(int* video_tracks, int* audio_tracks);
	long getEndFrame();
	void hard_delete_transition(Clip *c, int type);
	int width;
	int height;
	double frame_rate;
	int audio_frequency;
	int audio_layout;

	QVector<Selection> selections;
	long playhead;

	bool using_workarea;
	long workarea_in;
	long workarea_out;

	bool wrapper_sequence;

	int save_id;

	QVector<Marker> markers;
	QVector<Clip*> clips;
	QVector<Transition*> transitions;
};

// static variable for the currently active sequence
namespace olive {
    extern Sequence* ActiveSequence;
}

#endif // SEQUENCE_H
