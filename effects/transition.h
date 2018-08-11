#ifndef TRANSITION_H
#define TRANSITION_H

#include <QString>

struct Clip;

enum VideoTransitions {
    VIDEO_DISSOLVE_TRANSITION,
    VIDEO_TRANSITION_COUNT
};

enum AudioTransitions {
    AUDIO_LINEAR_FADE_TRANSITION,
    AUDIO_TRANSITION_COUNT
};

class Transition
{
public:
	Transition(int);
	~Transition();
	int id;
	QString name;
	int length;
	Transition* link;
	virtual void process_transition(double);
	virtual void process_audio(double, double, quint8*, int, bool);
	virtual Transition* copy();
};

void init_transitions();
Transition* create_transition(int transition_id, Clip* c);
extern QVector<QString> video_transition_names;
extern QVector<QString> audio_transition_names;

// Video Transitions
class CrossDissolveTransition : public Transition {
public:
    CrossDissolveTransition();
	void process_transition(double);
    Transition* copy();
};

// Audio Transitions
class LinearFadeTransition : public Transition {
public:
	LinearFadeTransition();
	void process_audio(double, double, quint8*, int, bool);
	Transition* copy();
};

#endif // TRANSITION_H
