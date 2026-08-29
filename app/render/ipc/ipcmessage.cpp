/***

  Oak - Non-Linear Video Editor
  Copyright (C) 2026 Oak Team

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

#include "ipcmessage.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QIODevice>

namespace olive
{
namespace ipc
{

bool WriteMessage(QIODevice *device, const QJsonObject &obj)
{
	QByteArray line = QJsonDocument(obj).toJson(QJsonDocument::Compact);
	line.append('\n');
	return device->write(line) == line.size();
}

bool ReadMessage(QByteArray *buffer, QJsonObject *out, bool *ok)
{
	while (true) {
		const int newline = buffer->indexOf('\n');
		if (newline < 0) {
			// No complete line buffered yet.
			return false;
		}

		const QByteArray line = buffer->left(newline);
		buffer->remove(0, newline + 1);

		// Skip blank lines silently (e.g. a stray newline) without flagging an error.
		if (line.trimmed().isEmpty()) {
			continue;
		}

		QJsonParseError err;
		const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
		if (err.error != QJsonParseError::NoError || !doc.isObject()) {
			if (ok) {
				*ok = false;
			}
			return false;
		}

		*out = doc.object();
		if (ok) {
			*ok = true;
		}
		return true;
	}
}

// ---- HandshakeMsg ---------------------------------------------------------------------------

QJsonObject HandshakeMsg::ToJson() const
{
	QJsonObject o;
	o["type"] = msgtype::kHandshake;
	o["protocol_version"] = protocol_version;
	o["shm_key"] = shm_key;
	o["input_shm_key"] = input_shm_key;
	o["input_slots"] = input_slots;
	o["output_slots"] = output_slots;
	o["slot_data_bytes"] = double(slot_data_bytes);
	o["input_slot_data_bytes"] = double(input_slot_data_bytes);
	return o;
}

bool HandshakeMsg::FromJson(const QJsonObject &o, HandshakeMsg *out)
{
	if (o["type"].toString() != QLatin1String(msgtype::kHandshake)) {
		return false;
	}
	out->protocol_version = o["protocol_version"].toInt();
	out->shm_key = o["shm_key"].toString();
	out->input_shm_key = o["input_shm_key"].toString();
	out->input_slots = o["input_slots"].toInt();
	out->output_slots = o["output_slots"].toInt();
	out->slot_data_bytes = qint64(o["slot_data_bytes"].toDouble());
	out->input_slot_data_bytes = qint64(o["input_slot_data_bytes"].toDouble());
	return true;
}

// ---- RenderFrameMsg -------------------------------------------------------------------------

QJsonObject RenderFrameMsg::ToJson() const
{
	QJsonObject o;
	o["type"] = msgtype::kRenderFrame;
	o["ticket"] = double(ticket_id);
	o["node"] = node_uuid;
	o["time_num"] = double(time_num);
	o["time_den"] = double(time_den);
	o["width"] = width;
	o["height"] = height;
	o["format"] = format;
	o["channels"] = channel_count;
	o["mode"] = mode;
	o["input_slot"] = input_slot;
	QJsonArray input_slot_array;
	for (int slot : input_slots) {
		input_slot_array.append(slot);
	}
	o["input_slots"] = input_slot_array;

	if (has_color_transform) {
		o["has_color_transform"] = true;
		o["color_is_display"] = color_is_display;
		o["color_output"] = color_output;
		o["color_view"] = color_view;
		o["color_look"] = color_look;
	}
	return o;
}

bool RenderFrameMsg::FromJson(const QJsonObject &o, RenderFrameMsg *out)
{
	if (o["type"].toString() != QLatin1String(msgtype::kRenderFrame)) {
		return false;
	}
	out->ticket_id = qint64(o["ticket"].toDouble());
	out->node_uuid = o["node"].toString();
	out->time_num = qint64(o["time_num"].toDouble());
	out->time_den = qint64(o["time_den"].toDouble(1));
	out->width = o["width"].toInt();
	out->height = o["height"].toInt();
	out->format = o["format"].toInt(-1);
	out->channel_count = o["channels"].toInt();
	out->mode = o["mode"].toInt();
	out->input_slot = o["input_slot"].toInt(-1);
	out->input_slots.clear();
	const QJsonArray input_slot_array = o["input_slots"].toArray();
	for (const QJsonValue &slot : input_slot_array) {
		out->input_slots.append(slot.toInt(-1));
	}
	if (out->input_slots.isEmpty() && out->input_slot >= 0) {
		out->input_slots.append(out->input_slot);
	}

	out->has_color_transform = o["has_color_transform"].toBool(false);
	if (out->has_color_transform) {
		out->color_is_display = o["color_is_display"].toBool(false);
		out->color_output = o["color_output"].toString();
		out->color_view = o["color_view"].toString();
		out->color_look = o["color_look"].toString();
	}
	return true;
}

// ---- FrameReadyMsg --------------------------------------------------------------------------

QJsonObject FrameReadyMsg::ToJson() const
{
	QJsonObject o;
	o["type"] = msgtype::kFrameReady;
	o["ticket"] = double(ticket_id);
	o["slot"] = output_slot;
	return o;
}

bool FrameReadyMsg::FromJson(const QJsonObject &o, FrameReadyMsg *out)
{
	if (o["type"].toString() != QLatin1String(msgtype::kFrameReady)) {
		return false;
	}
	out->ticket_id = qint64(o["ticket"].toDouble());
	out->output_slot = o["slot"].toInt();
	return true;
}

// ---- CancelMsg ------------------------------------------------------------------------------

QJsonObject CancelMsg::ToJson() const
{
	QJsonObject o;
	o["type"] = msgtype::kCancel;
	o["ticket"] = double(ticket_id);
	return o;
}

bool CancelMsg::FromJson(const QJsonObject &o, CancelMsg *out)
{
	if (o["type"].toString() != QLatin1String(msgtype::kCancel)) {
		return false;
	}
	out->ticket_id = qint64(o["ticket"].toDouble());
	return true;
}

// ---- LoadGraphMsg ---------------------------------------------------------------------------

QJsonObject LoadGraphMsg::ToJson() const
{
	QJsonObject o;
	o["type"] = msgtype::kLoadGraph;
	o["path"] = path;
	return o;
}

bool LoadGraphMsg::FromJson(const QJsonObject &o, LoadGraphMsg *out)
{
	if (o["type"].toString() != QLatin1String(msgtype::kLoadGraph)) {
		return false;
	}
	out->path = o["path"].toString();
	return true;
}

} // namespace ipc
} // namespace olive
