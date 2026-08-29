/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "jobtime.h"

#include <QMutex>

namespace olive
{

uint64_t job_time_index = 0;
QMutex job_time_mutex;

JobTime::JobTime()
{
	Acquire();
}

void JobTime::Acquire()
{
	job_time_mutex.lock();

	value_ = job_time_index;
	job_time_index++;

	job_time_mutex.unlock();
}

}

QDebug operator<<(QDebug debug, const olive::JobTime &r)
{
	return debug.space() << r.value();
}
