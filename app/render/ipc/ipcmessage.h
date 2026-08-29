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

#ifndef IPC_IPCMESSAGE_H
#define IPC_IPCMESSAGE_H

#include <cstdint>
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

class QIODevice;

namespace olive
{
namespace ipc
{

/**
 * @brief Control-plane protocol exchanged over stdio between main and render worker.
 *
 * The wire format is NDJSON: one compact QJsonObject per line, terminated by '\n'. This is
 * deliberately human-readable so the channel can be inspected live with `tee`/`cat` and test
 * messages can be injected by hand. The stdio channel carries only low-frequency control traffic;
 * bulk pixel data travels through the shared-memory FrameSlotPool, and the (potentially large)
 * serialized node graph travels via a temporary file referenced by path.
 *
 * Every message object has a "type" string field. Directionality (M = main, W = worker):
 *   "handshake"     M<->W  Negotiate protocol version and announce shared-memory key/geometry.
 *   "load_graph"    M ->W  Path to a temporary file holding the serialized node graph.
 *   "render_frame"  M ->W  Request a frame: node uuid, time, video params.
 *   "frame_ready"   W ->M  A rendered frame is published; carries the output-slot index + ticket.
 *   "cancel"        M ->W  Abandon an in-flight ticket by id.
 *   "graph_update"  M ->W  (Reserved, Phase 6) Incremental graph mutation, mirrors ProjectCopier.
 *   "shutdown"      M ->W  Finish current work and exit cleanly.
 *   "error"         W ->M  Worker-side failure report (human-readable "message" field).
 */
namespace msgtype
{
constexpr const char *kHandshake = "handshake";
constexpr const char *kLoadGraph = "load_graph";
constexpr const char *kRenderFrame = "render_frame";
constexpr const char *kFrameReady = "frame_ready";
constexpr const char *kCancel = "cancel";
constexpr const char *kGraphUpdate = "graph_update";
constexpr const char *kShutdown = "shutdown";
constexpr const char *kError = "error";
} // namespace msgtype

/**
 * @brief Write one NDJSON message line to `device`.
 *
 * Serializes `obj` to compact JSON, appends '\n', and writes the whole line in one call. Returns
 * true only if the full line was written.
 */
bool WriteMessage(QIODevice *device, const QJsonObject &obj);

/**
 * @brief Pull one complete NDJSON line out of `buffer` and parse it.
 *
 * If `buffer` contains at least one '\n', the leading line is removed, parsed as JSON, and returned
 * via `out` (true). If no complete line is buffered yet, leaves `buffer` untouched and returns
 * false. Malformed lines are skipped (removed) and reported via `*ok = false` so the reader can log
 * and continue rather than wedge. Supports the typical "append bytes as they arrive, then drain
 * complete lines" reader loop on a pipe.
 */
bool ReadMessage(QByteArray *buffer, QJsonObject *out, bool *ok = nullptr);

// ---- Typed message builders / parsers -------------------------------------------------------
//
// Thin helpers that construct or read the QJsonObject for each message type, keeping field names in
// one place so main and worker agree. Fields use plain JSON numbers/strings; 64-bit ids are stored
// as JSON numbers (doubles exactly represent integers up to 2^53, ample for our counters).

struct HandshakeMsg {
	int protocol_version = 0;
	QString shm_key; ///< Worker->main output shared-memory segment key.
	QString
		input_shm_key; ///< Main->worker input shared-memory segment key (optional).
	int input_slots = 0; ///< Number of main->worker input frame slots.
	int output_slots = 0; ///< Number of worker->main output frame slots.
	qint64 slot_data_bytes = 0; ///< Per-output-slot pixel block size.
	qint64 input_slot_data_bytes = 0; ///< Per-input-slot pixel block size.

	QJsonObject ToJson() const;
	static bool FromJson(const QJsonObject &o, HandshakeMsg *out);
};

struct RenderFrameMsg {
	qint64 ticket_id =
		0; ///< Correlates this request with the eventual frame_ready.
	QString
		node_uuid; ///< Output/viewer node to render, by stable uuid in the loaded graph.
	qint64 time_num = 0;
	qint64 time_den = 1;
	int width = 0; ///< Forced output size (0 = use graph default).
	int height = 0;
	int format = -1; ///< Forced PixelFormat::Format (-1 = default/INVALID).
	int channel_count = 0; ///< 0 = default.
	int mode = 0; ///< RenderMode::Mode.
	int input_slot =
		-1; ///< Optional main->worker decoded input slot for footage nodes.
	QVector<int>
		input_slots; ///< Optional ordered decoded input slots for footage nodes.

	// Output color transform to apply before returning the frame. When empty,
	// the worker returns the image in the project's reference space.
	bool has_color_transform = false;
	bool color_is_display = false;
	QString color_output;
	QString color_view;
	QString color_look;

	QJsonObject ToJson() const;
	static bool FromJson(const QJsonObject &o, RenderFrameMsg *out);
};

struct FrameReadyMsg {
	qint64 ticket_id = 0;
	int output_slot = 0; ///< Index into the worker->main output FrameSlotPool.

	QJsonObject ToJson() const;
	static bool FromJson(const QJsonObject &o, FrameReadyMsg *out);
};

struct CancelMsg {
	qint64 ticket_id = 0;

	QJsonObject ToJson() const;
	static bool FromJson(const QJsonObject &o, CancelMsg *out);
};

struct LoadGraphMsg {
	QString path; ///< Temporary file holding the serialized node graph.

	QJsonObject ToJson() const;
	static bool FromJson(const QJsonObject &o, LoadGraphMsg *out);
};

} // namespace ipc
} // namespace olive

#endif // IPC_IPCMESSAGE_H
