/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "waveoutput.h"

#include "render/audioparams.h"

namespace olive {

const int16_t kWAVIntegerFormat = 1;
const int16_t kWAVFloatFormat = 3;

WaveOutput::WaveOutput(const QString &f,
                       const AudioParams& params) :
  file_(f),
  params_(params)
{
  Q_ASSERT(params_.is_valid());
}

WaveOutput::~WaveOutput()
{
  close();
}

bool WaveOutput::open()
{
  data_length_ = 0;

  if (file_.open(QFile::WriteOnly)) {
    // RIFF header
    file_.write("RIFF");

    // Total file size minus RIFF and this integer (minus 8 bytes, filled in later)
    write_int<int32_t>(&file_, 0);

    // File type header
    file_.write("WAVE");

    // Begin format descriptor chunk
    file_.write("fmt ");

    // Format chunk size
    write_int<int32_t>(&file_, 16);

    // Type of format
    switch (params_.format()) {
    case AudioParams::kFormatUnsigned8:
    case AudioParams::kFormatSigned16:
    case AudioParams::kFormatSigned32:
    case AudioParams::kFormatSigned64:
      write_int<int16_t>(&file_, kWAVIntegerFormat);
      break;
    case AudioParams::kFormatFloat32:
    case AudioParams::kFormatFloat64:
      write_int<int16_t>(&file_, kWAVFloatFormat);
      break;
    case AudioParams::kFormatInvalid:
    case AudioParams::kFormatCount:
      qWarning() << "Invalid sample format for WAVE audio";
      file_.close();
      return false;
    }

    // Number of channels
    write_int<int16_t>(&file_, static_cast<int16_t>(params_.channel_count()));

    // Sample rate
    write_int<int32_t>(&file_, params_.sample_rate());

    // Bytes per second
    write_int<int32_t>(&file_, params_.samples_to_bytes(params_.sample_rate()));

    // Bytes per sample
    write_int<int16_t>(&file_, static_cast<int16_t>(params_.samples_to_bytes(1)));

    // Bits per sample per channel
    write_int<int16_t>(&file_, static_cast<int16_t>(params_.bits_per_sample()));

    // Data chunk header
    file_.write("data");

    // Size of data chunk (filled in later)
    write_int<int32_t>(&file_, 0);

    return true;
  }

  return false;
}

void WaveOutput::write(const QByteArray &bytes)
{
  if (file_.isOpen()) {
    file_.write(bytes);

    data_length_ += bytes.size();
  }
}

void WaveOutput::write(const char *bytes, int length)
{
  if (file_.isOpen()) {
    file_.write(bytes, length);

    data_length_ += length;
  }
}

void WaveOutput::close()
{
  if (file_.isOpen()) {

    // Write file sizes
    file_.seek(4);
    write_int<int32_t>(&file_, data_length_ + 36);

    file_.seek(40);
    write_int<int32_t>(&file_, data_length_);

    file_.close();
  }
}

const int& WaveOutput::data_length() const
{
  return data_length_;
}

const AudioParams &WaveOutput::params() const
{
  return params_;
}

void WaveOutput::switch_endianness(QByteArray& array)
{
  int half_sz = array.size()/2;

  for (int i=0;i<half_sz;i++) {
    int oppose_index = array.size() - i - 1;

    char temp = array[i];
    array[i] = array[oppose_index];
    array[oppose_index] = temp;
  }
}

template<typename T>
void WaveOutput::write_int(QFile *file, T integer)
{
  QByteArray bytes;
  bytes.resize(sizeof(T));
  memcpy(bytes.data(), &integer, static_cast<size_t>(bytes.size()));

  // WAV expects little-endian, so if the integer is big endian we need to switch
  if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
    switch_endianness(bytes);
  }

  file->write(bytes);
}

}
