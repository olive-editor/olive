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

#ifndef CANCELATOM_H
#define CANCELATOM_H

#include <QMutex>

namespace olive
{

class CancelAtom {
public:
	CancelAtom()
		: cancelled_(false)
		, heard_(false)
	{
	}

	bool IsCancelled()
	{
		QMutexLocker locker(&mutex_);
		if (cancelled_) {
			heard_ = true;
		}
		return cancelled_;
	}

	void Cancel()
	{
		QMutexLocker locker(&mutex_);
		cancelled_ = true;
	}

	bool HeardCancel()
	{
		QMutexLocker locker(&mutex_);
		return heard_;
	}

private:
	QMutex mutex_;

	bool cancelled_;

	bool heard_;
};

}

#endif // CANCELATOM_H
