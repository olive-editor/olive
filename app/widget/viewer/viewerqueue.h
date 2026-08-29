/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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

#ifndef VIEWERQUEUE_H
#define VIEWERQUEUE_H

#include <QVariant>
#include <QMutex>
#include <QMutexLocker>

#include "codec/frame.h"

namespace olive
{

struct ViewerPlaybackFrame {
	rational timestamp;
	QVariant frame;
};

class ViewerQueue : public std::list<ViewerPlaybackFrame> {
public:
	ViewerQueue()
		: mutex_(new QMutex())
	{
	}
	ViewerQueue(const ViewerQueue &other)
		: std::list<ViewerPlaybackFrame>(other)
		, mutex_(new QMutex())
	{
	}
	~ViewerQueue()
	{
		delete mutex_;
	}
	ViewerQueue &operator=(const ViewerQueue &other)
	{
		std::list<ViewerPlaybackFrame>::operator=(other);
		return *this;
	}

	void AppendTimewise(const ViewerPlaybackFrame &f, int playback_speed)
	{
		QMutexLocker locker(mutex_);
		if (this->empty() ||
			(this->back().timestamp < f.timestamp) == (playback_speed > 0)) {
			this->push_back(f);
		} else {
			for (auto i = this->begin(); i != this->end(); i++) {
				if ((i->timestamp > f.timestamp) == (playback_speed > 0)) {
					this->insert(i, f);
					break;
				}
			}
		}
	}

	void PurgeBefore(const rational &time, int playback_speed)
	{
		QMutexLocker locker(mutex_);
		while (!this->empty() &&
			   ((playback_speed > 0 && this->front().timestamp < time) ||
				(playback_speed < 0 && this->front().timestamp > time))) {
			this->pop_front();
		}
	}

private:
	QMutex *mutex_;
};

}

Q_DECLARE_METATYPE(olive::ViewerPlaybackFrame)

#endif // VIEWERQUEUE_H
