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

#ifndef JOBTIME_H
#define JOBTIME_H

#include <QDebug>
#include <stdint.h>

namespace olive
{

class JobTime {
public:
	JobTime();

	void Acquire();

	uint64_t value() const
	{
		return value_;
	}

	bool operator==(const JobTime &rhs) const
	{
		return value_ == rhs.value_;
	}

	bool operator!=(const JobTime &rhs) const
	{
		return value_ != rhs.value_;
	}

	bool operator<(const JobTime &rhs) const
	{
		return value_ < rhs.value_;
	}

	bool operator>(const JobTime &rhs) const
	{
		return value_ > rhs.value_;
	}

	bool operator<=(const JobTime &rhs) const
	{
		return value_ <= rhs.value_;
	}

	bool operator>=(const JobTime &rhs) const
	{
		return value_ >= rhs.value_;
	}

private:
	uint64_t value_;
};

}

QDebug operator<<(QDebug debug, const olive::JobTime &r);

Q_DECLARE_METATYPE(olive::JobTime)

#endif // JOBTIME_H
