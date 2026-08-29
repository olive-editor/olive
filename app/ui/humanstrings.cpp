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

#include "humanstrings.h"

#include <QCoreApplication>

namespace olive
{

QString HumanStrings::SampleRateToString(const int &sample_rate)
{
	return QCoreApplication::translate("AudioParams", "%1 Hz").arg(sample_rate);
}

QString HumanStrings::ChannelLayoutToString(const uint64_t &layout)
{
	switch (layout) {
	case kChannelLayoutMono:
		return QCoreApplication::translate("AudioParams", "Mono");
	case kChannelLayoutStereo:
		return QCoreApplication::translate("AudioParams", "Stereo");
	case kChannelLayout2_1:
		return QCoreApplication::translate("AudioParams", "2.1");
	case kChannelLayout5Point1:
		return QCoreApplication::translate("AudioParams", "5.1");
	case kChannelLayout7Point1:
		return QCoreApplication::translate("AudioParams", "7.1");
	default:
		return QCoreApplication::translate("AudioParams", "Unknown (0x%1)")
			.arg(layout, 1, 16);
	}
}

QString HumanStrings::FormatToString(const SampleFormat &f)
{
	switch (f) {
	case SampleFormat::U8:
		return QCoreApplication::translate("AudioParams",
										   "Unsigned 8-bit (Packed)");
	case SampleFormat::S16:
		return QCoreApplication::translate("AudioParams",
										   "Signed 16-bit (Packed)");
	case SampleFormat::S32:
		return QCoreApplication::translate("AudioParams",
										   "Signed 32-bit (Packed)");
	case SampleFormat::S64:
		return QCoreApplication::translate("AudioParams",
										   "Signed 64-bit (Packed)");
	case SampleFormat::F32:
		return QCoreApplication::translate("AudioParams",
										   "Float 32-bit (Packed)");
	case SampleFormat::F64:
		return QCoreApplication::translate("AudioParams",
										   "Float 64-bit (Packed)");
	case SampleFormat::U8P:
		return QCoreApplication::translate("AudioParams",
										   "Unsigned 8-bit (Planar)");
	case SampleFormat::S16P:
		return QCoreApplication::translate("AudioParams",
										   "Signed 16-bit (Planar)");
	case SampleFormat::S32P:
		return QCoreApplication::translate("AudioParams",
										   "Signed 32-bit (Planar)");
	case SampleFormat::S64P:
		return QCoreApplication::translate("AudioParams",
										   "Signed 64-bit (Planar)");
	case SampleFormat::F32P:
		return QCoreApplication::translate("AudioParams",
										   "Float 32-bit (Planar)");
	case SampleFormat::F64P:
		return QCoreApplication::translate("AudioParams",
										   "Float 64-bit (Planar)");

	case SampleFormat::INVALID:
	case SampleFormat::COUNT:
		break;
	}

	return QCoreApplication::translate("AudioParams", "Unknown (0x%1)")
		.arg(static_cast<int>(f), 1, 16);
}

}
