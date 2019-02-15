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

#ifndef RENDERFUNCTIONS_H
#define RENDERFUNCTIONS_H

#include <QOpenGLContext>
#include <QVector>

class Effect;
class Viewer;
class QOpenGLShaderProgram;
struct Sequence;
class Clip;

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
	QOpenGLShaderProgram* premultiply_program;
	GLuint main_buffer;
	GLuint main_attachment;
	GLuint backend_buffer1;
	GLuint backend_attachment1;
	GLuint backend_buffer2;
	GLuint backend_attachment2;
};

GLuint compose_sequence(ComposeSequenceParams &params);

void compose_audio(Viewer* viewer, Sequence* seq, bool render_audio, int playback_speed);

void viewport_render();

#endif // RENDERFUNCTIONS_H
