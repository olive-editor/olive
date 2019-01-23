#ifndef RENDERFUNCTIONS_H
#define RENDERFUNCTIONS_H

#include <QOpenGLContext>
#include <QVector>

class Effect;
class Viewer;
struct Sequence;
struct Clip;

GLuint compose_sequence(Viewer* viewer,
						QOpenGLContext* ctx,
						Sequence* seq,
						QVector<Clip*>& nests,
						bool video,
						bool render_audio,
						Effect **gizmos,
						bool &texture_failed,
						bool rendering,
						int playback_speed);

void compose_audio(Viewer* viewer, Sequence* seq, bool render_audio, int playback_speed);

void viewport_render();

#endif // RENDERFUNCTIONS_H
