#ifndef TRANSITION_H
#define TRANSITION_H

#include <QString>

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
    Transition();
    int id;
    int length;
    QString name;
    Transition* link;
    virtual void process_transition(float);
    virtual Transition* copy();
};

class CrossDissolveTransition : public Transition {
public:
    CrossDissolveTransition();
    void process_transition(float);
    Transition* copy();
};

#endif // TRANSITION_H
