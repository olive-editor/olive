/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC
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

#ifndef LIBOLIVECORE_VALUE_H
#define LIBOLIVECORE_VALUE_H

#include <map>
#include <stdint.h>
#include <string>
#include <string.h>
#include <vector>

namespace olive::core
{

/**
 * @brief Generic type container
 */
class Value {
public:
	enum Type {
		/// Null/no data
		NONE,

		/// Signed int64
		INT,

		/// Double-precision float
		FLOAT,

		/// UTF-8 string
		STRING
	};

	Value()
	{
		type_ = NONE;
	}

	Value(int64_t v)
	{
		data_.resize(sizeof(int64_t));
		memcpy(data_.data(), &v, sizeof(int64_t));
		type_ = INT;
	}

	Value(double v)
	{
		data_.resize(sizeof(double));
		memcpy(data_.data(), &v, sizeof(int64_t));
		type_ = FLOAT;
	}

	Value(const char *s)
	{
		size_t sz = strlen(s);
		data_.resize(sz);
		memcpy(data_.data(), s, sz);
		type_ = STRING;
	}

	Value(const std::string &s)
	{
		data_.resize(s.size());
		memcpy(data_.data(), s.data(), data_.size());
		type_ = STRING;
	}

private:
	std::vector<uint8_t> data_;
	Type type_;
};

using ValueMap = std::map<std::string, Value>;

}

#endif // LIBOLIVECORE_VALUE_H
