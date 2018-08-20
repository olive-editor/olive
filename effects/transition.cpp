#include "transition.h"

#include "project/clip.h"
#include "io/config.h"

#include <QDebug>

QVector<QString> video_transition_names;
QVector<QString> audio_transition_names;

void init_transitions() {
	video_transition_names.resize(VIDEO_TRANSITION_COUNT);
	audio_transition_names.resize(AUDIO_TRANSITION_COUNT);

	video_transition_names[VIDEO_DISSOLVE_TRANSITION] = "Cross Dissolve";

	audio_transition_names[AUDIO_LINEAR_FADE_TRANSITION] = "Linear Fade";
}

Transition::Transition(int i) : id(i), name(video_transition_names[i]), length(config.default_transition_length), link(NULL) {}

Transition::~Transition() {}

Transition* Transition::copy() {
	return NULL;
}

void Transition::process_transition(double) {}
void Transition::process_audio(double, double, quint8*, int, bool) {}

Transition* create_transition(int transition_id, Clip* c) {
    if (c->track < 0) {
        switch (transition_id) {
        case VIDEO_DISSOLVE_TRANSITION: return new CrossDissolveTransition();
        }
    } else {
        switch (transition_id) {
        case AUDIO_LINEAR_FADE_TRANSITION: return new LinearFadeTransition();
        }
    }
    qDebug() << "[ERROR] Invalid transition ID";
    return NULL;
}
