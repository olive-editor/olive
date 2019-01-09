/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CACHER_H
#define CACHER_H

#include <QThread>
#include <QVector>

struct Clip;

class Cacher : public QThread
{
//	Q_OBJECT
public:
	Cacher(Clip* c);
    void run();

	bool caching;

	// must be set before caching
	long playhead;
	bool reset;
    bool scrubbing;
    bool interrupt;
	QVector<Clip*> nests;

private:
	Clip* clip;
};

void open_clip_worker(Clip* clip);
void cache_clip_worker(Clip* clip, long playhead, bool reset, bool scrubbing, QVector<Clip *> nest);
void close_clip_worker(Clip* clip);

#endif // CACHER_H
