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

#ifndef HUMANSTRINGS_H
#define HUMANSTRINGS_H

#include <olive/core/core.h>
#include <QObject>

namespace olive
{

using namespace core;

class HumanStrings : public QObject {
	Q_OBJECT
public:
	HumanStrings() = default;

	static QString SampleRateToString(const int &sample_rate);

	static QString ChannelLayoutToString(const uint64_t &layout);

	static QString FormatToString(const SampleFormat &f);
};

}

#endif // HUMANSTRINGS_H
