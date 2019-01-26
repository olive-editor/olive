#ifndef RENDERFUNCTIONS_H
#define RENDERFUNCTIONS_H

#include <QOpenGLContext>
#include <QVector>

class Effect;
class Viewer;
class QOpenGLShaderProgram;
struct Sequence;
struct Clip;

struct ComposeSequenceParams {
	Viewer* viewer;
	QOpenGLContext* ctx;
	Sequence* seq;
	QVector<Clip*> nests;
	bool video;
	bool render_audio;
	Effect** gizmos;
	bool texture_failed;
	bool rendering;
	int playback_speed;
	QOpenGLShaderProgram* blend_mode_program;
	GLuint main_buffer;
	GLuint backend_buffer1;
	GLuint backend_attachment1;
	GLuint backend_buffer2;
	GLuint backend_attachment2;
};

GLuint compose_sequence(ComposeSequenceParams &params);

void compose_audio(Viewer* viewer, Sequence* seq, bool render_audio, int playback_speed);

void viewport_render();

#endif // RENDERFUNCTIONS_H
