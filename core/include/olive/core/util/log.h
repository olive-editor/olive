/*
 * Olive Community Edition - Non-Linear Video Editor
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

#ifndef LOG_H
#define LOG_H

#include <iostream>

namespace olive::core
{

class Log {
public:
	Log(const char *type)
	{
		std::cerr << "[" << type << "]";
	}

	~Log()
	{
		std::cerr << std::endl;
	}

	template <typename T> Log &operator<<(const T &t)
	{
		std::cerr << " " << t;
		return *this;
	}

	static Log Debug()
	{
		return Log("DEBUG");
	}

	static Log Info()
	{
		return Log("INFO");
	}

	static Log Warning()
	{
		return Log("WARNING");
	}

	static Log Error()
	{
		return Log("ERROR");
	}
};

}

#endif // LOG_H
