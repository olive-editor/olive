/*
 * Oak Video Editor - Render Worker Pool & IPC Coverage Tests
 * Copyright (C) 2026 Oak Team
 *
 * CPU-only, headless coverage for the render worker IPC stack that
 * render_ipc_test.cpp and render_worker_footage_test.cpp do not reach:
 * - SharedMemoryRegion  (named shared memory create/attach/lifetime)
 * - FrameSlotPool       (layout math, invalid attach, metadata fields, FIFO order)
 * - NDJSON messages     (color transform, legacy input_slot fallback, defaults,
 *                        blank/non-object lines, closed-device writes)
 * - RenderWorkerPool    (early validation paths that need no worker process)
 * - DecoderCache        (DecoderPair defaults and insert/value round trip)
 *
 * No worker processes, GPU, audio devices, or network access are required; the
 * shared-memory tests use real OS segments with per-run unique keys.
 */

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include <QBuffer>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <olive/core/core.h>

#include "codec/decoder.h"
#include "node/output/track/track.h"
#include "render/ipc/frameslotpool.h"
#include "render/ipc/ipcmessage.h"
#include "render/ipc/sharedmemoryregion.h"
#include "render/rendercache.h"
#include "render/renderworkerpool.h"

namespace
{

// Unique-per-run segment key so stale POSIX segments left by earlier runs can
// never collide with a test (MakeKey() bakes the pid into the key).
QString TestShmKey(const char *tag)
{
	return olive::ipc::SharedMemoryRegion::MakeKey(
			   QCoreApplication::applicationPid(), 99) +
		   QStringLiteral("-") + QLatin1String(tag);
}

olive::RenderManager::RenderVideoParams
MakeVideoParams(olive::Node *node, const olive::VideoParams &video_params)
{
	return olive::RenderManager::RenderVideoParams(
		node, video_params, olive::core::AudioParams(),
		olive::core::rational(0), nullptr, olive::RenderMode::kOnline);
}

} // namespace

// ============================================================================
// SharedMemoryRegion
// ============================================================================

TEST(SharedMemoryRegion, MakeKeyFormat)
{
	EXPECT_EQ(olive::ipc::SharedMemoryRegion::MakeKey(12345, 3),
			  QStringLiteral("olive-rw-12345-3"));

	EXPECT_NE(olive::ipc::SharedMemoryRegion::MakeKey(12345, 3),
			  olive::ipc::SharedMemoryRegion::MakeKey(12345, 4));
	EXPECT_NE(olive::ipc::SharedMemoryRegion::MakeKey(12345, 3),
			  olive::ipc::SharedMemoryRegion::MakeKey(12346, 3));
}

TEST(SharedMemoryRegion, CreateProvidesZeroedWritableMemory)
{
	const QString key = TestShmKey("zeroed");
	olive::ipc::SharedMemoryRegion region;
	ASSERT_TRUE(region.Open(key, 4096, olive::ipc::SharedMemoryRegion::kCreate))
		<< region.error().toStdString();

	EXPECT_TRUE(region.IsValid());
	EXPECT_EQ(region.size(), size_t(4096));
	EXPECT_EQ(region.key(), key);
	ASSERT_NE(region.data(), nullptr);

	// Freshly created segments are zero-filled.
	const auto *bytes = static_cast<const uint8_t *>(region.data());
	for (size_t i = 0; i < region.size(); i++) {
		ASSERT_EQ(bytes[i], 0) << "byte " << i;
	}

	// The mapping is readable and writable.
	auto *writable = static_cast<uint8_t *>(region.data());
	for (size_t i = 0; i < region.size(); i++) {
		writable[i] = uint8_t(i * 31 + 7);
	}
	EXPECT_EQ(writable[0], 7);
	EXPECT_EQ(writable[4095], uint8_t(4095 * 31 + 7));
}

TEST(SharedMemoryRegion, AttachToMissingKeyFails)
{
	olive::ipc::SharedMemoryRegion region;
	EXPECT_FALSE(region.Open(TestShmKey("missing"), 4096,
							 olive::ipc::SharedMemoryRegion::kAttach));
	EXPECT_FALSE(region.IsValid());
	EXPECT_EQ(region.data(), nullptr);
	EXPECT_FALSE(region.error().isEmpty());
}

TEST(SharedMemoryRegion, CreateAttachRoundTrip)
{
	const QString key = TestShmKey("roundtrip");

	olive::ipc::SharedMemoryRegion owner;
	ASSERT_TRUE(owner.Open(key, 8192, olive::ipc::SharedMemoryRegion::kCreate))
		<< owner.error().toStdString();

	olive::ipc::SharedMemoryRegion peer;
	ASSERT_TRUE(peer.Open(key, 8192, olive::ipc::SharedMemoryRegion::kAttach))
		<< peer.error().toStdString();
	EXPECT_TRUE(peer.IsValid());
	EXPECT_EQ(peer.size(), size_t(8192));
	EXPECT_EQ(peer.key(), key);

	// Writes through the owner mapping are visible through the peer mapping.
	auto *owner_bytes = static_cast<uint8_t *>(owner.data());
	const auto *peer_bytes = static_cast<const uint8_t *>(peer.data());
	for (size_t i = 0; i < 8192; i += 257) {
		owner_bytes[i] = uint8_t(i ^ 0x5A);
	}
	for (size_t i = 0; i < 8192; i += 257) {
		EXPECT_EQ(peer_bytes[i], uint8_t(i ^ 0x5A)) << "offset " << i;
	}

	// And vice versa: the peer maps the same segment read/write.
	auto *peer_writable = static_cast<uint8_t *>(peer.data());
	peer_writable[123] = 0xA5;
	EXPECT_EQ(owner_bytes[123], 0xA5);
}

TEST(SharedMemoryRegion, ZeroSizeCreateFails)
{
	olive::ipc::SharedMemoryRegion region;
	// A zero-length mapping is rejected (EINVAL from mmap on POSIX, invalid
	// size for CreateFileMapping on Windows).
	EXPECT_FALSE(region.Open(TestShmKey("zerosize"), 0,
							 olive::ipc::SharedMemoryRegion::kCreate));
	EXPECT_FALSE(region.IsValid());
	EXPECT_FALSE(region.error().isEmpty());
}

TEST(SharedMemoryRegion, CloseInvalidatesThenReopenWorks)
{
	olive::ipc::SharedMemoryRegion region;
	ASSERT_TRUE(region.Open(TestShmKey("close1"), 4096,
							olive::ipc::SharedMemoryRegion::kCreate));

	region.Close();
	EXPECT_FALSE(region.IsValid());
	EXPECT_EQ(region.data(), nullptr);
	EXPECT_EQ(region.size(), size_t(0));

	// Close is idempotent.
	region.Close();
	EXPECT_FALSE(region.IsValid());

	// The same object can be reused for a new segment (Open() closes first).
	ASSERT_TRUE(region.Open(TestShmKey("close2"), 2048,
							olive::ipc::SharedMemoryRegion::kCreate));
	EXPECT_TRUE(region.IsValid());
	EXPECT_EQ(region.size(), size_t(2048));
}

TEST(SharedMemoryRegion, OwnerDestructionUnlinksSegment)
{
	const QString key = TestShmKey("unlink");
	{
		olive::ipc::SharedMemoryRegion owner;
		ASSERT_TRUE(owner.Open(key, 4096,
							   olive::ipc::SharedMemoryRegion::kCreate));

		// While the owner lives, attaching works.
		olive::ipc::SharedMemoryRegion peer;
		ASSERT_TRUE(
			peer.Open(key, 4096, olive::ipc::SharedMemoryRegion::kAttach));
	}

	// Once the owner is destroyed the name is unlinked; new attaches fail.
	olive::ipc::SharedMemoryRegion late;
	EXPECT_FALSE(late.Open(key, 4096, olive::ipc::SharedMemoryRegion::kAttach));
	EXPECT_FALSE(late.IsValid());
}

// ============================================================================
// FrameSlotPool
// ============================================================================

TEST(FrameSlotPool, AttachRejectsBadMagic)
{
	// A region that was never initialized by Create() has no valid magic number.
	std::vector<uint8_t> mem(olive::ipc::FrameSlotPool::BytesNeeded(2, 64), 0xAB);
	olive::ipc::FrameSlotPool pool = olive::ipc::FrameSlotPool::Attach(mem.data());

	EXPECT_FALSE(pool.IsValid());
	EXPECT_EQ(pool.slot_count(), 0u);
	EXPECT_EQ(pool.slot_data_bytes(), size_t(0));
}

TEST(FrameSlotPool, DefaultConstructedIsInvalid)
{
	olive::ipc::FrameSlotPool pool;
	EXPECT_FALSE(pool.IsValid());
	EXPECT_EQ(pool.slot_count(), 0u);
	EXPECT_EQ(pool.slot_data_bytes(), size_t(0));
}

TEST(FrameSlotPool, BytesNeededReflectsGeometry)
{
	// The total is a sum of 64-byte-aligned sub-regions, so it stays 64-aligned.
	EXPECT_EQ(olive::ipc::FrameSlotPool::BytesNeeded(1, 64) % 64, 0u);
	EXPECT_EQ(olive::ipc::FrameSlotPool::BytesNeeded(3, 1000) % 64, 0u);

	// More slots and bigger slots both need strictly more memory...
	EXPECT_LT(olive::ipc::FrameSlotPool::BytesNeeded(1, 64),
			  olive::ipc::FrameSlotPool::BytesNeeded(2, 64));
	EXPECT_LT(olive::ipc::FrameSlotPool::BytesNeeded(2, 64),
			  olive::ipc::FrameSlotPool::BytesNeeded(3, 64));
	EXPECT_LT(olive::ipc::FrameSlotPool::BytesNeeded(2, 64),
			  olive::ipc::FrameSlotPool::BytesNeeded(2, 128));

	// ...but sizes inside the same 64-byte alignment bucket collapse together.
	EXPECT_EQ(olive::ipc::FrameSlotPool::BytesNeeded(2, 65),
			  olive::ipc::FrameSlotPool::BytesNeeded(2, 128));
}

TEST(FrameSlotPool, SlotDataBlocksAreAlignedAndDistinct)
{
	constexpr uint32_t kSlots = 2;
	constexpr size_t kSlotBytes = 100; // deliberately not 64-aligned

	std::vector<uint8_t> mem(
		olive::ipc::FrameSlotPool::BytesNeeded(kSlots, kSlotBytes));
	olive::ipc::FrameSlotPool pool =
		olive::ipc::FrameSlotPool::Create(mem.data(), kSlots, kSlotBytes);
	ASSERT_TRUE(pool.IsValid());

	auto *first = static_cast<uint8_t *>(pool.SlotData(0));
	auto *second = static_cast<uint8_t *>(pool.SlotData(1));

	// Slot data blocks are padded out to 64-byte boundaries within the region.
	EXPECT_EQ(second - first, ptrdiff_t(128));

	// A full-size write to one slot never spills into the next.
	std::memset(first, 0x11, kSlotBytes);
	std::memset(second, 0x22, kSlotBytes);
	EXPECT_EQ(first[kSlotBytes - 1], 0x11);
	EXPECT_EQ(second[0], 0x22);

	// The const overload maps the same addresses.
	const olive::ipc::FrameSlotPool &const_pool = pool;
	EXPECT_EQ(static_cast<const uint8_t *>(const_pool.SlotData(0)), first);
	EXPECT_EQ(static_cast<const uint8_t *>(const_pool.SlotData(1)), second);
}

TEST(FrameSlotPool, MetadataFieldsRoundTrip)
{
	constexpr uint32_t kSlots = 2;
	constexpr size_t kSlotBytes = 64;

	std::vector<uint8_t> mem(
		olive::ipc::FrameSlotPool::BytesNeeded(kSlots, kSlotBytes));
	olive::ipc::FrameSlotPool filler =
		olive::ipc::FrameSlotPool::Create(mem.data(), kSlots, kSlotBytes);
	olive::ipc::FrameSlotPool drainer =
		olive::ipc::FrameSlotPool::Attach(mem.data());

	uint32_t idx = 0;
	ASSERT_TRUE(filler.Acquire(&idx));

	// Freshly created pools zero the metadata array.
	const olive::ipc::FrameSlotPool &const_drainer = drainer;
	const olive::ipc::FrameSlotMeta *blank = const_drainer.Meta(idx);
	EXPECT_EQ(blank->id, 0);
	EXPECT_EQ(blank->time_num, 0);
	EXPECT_EQ(blank->time_den, 0);
	EXPECT_EQ(blank->width, 0);
	EXPECT_EQ(blank->colorspace[0], '\0');

	// Every field the producer writes survives the hand-off.
	olive::ipc::FrameSlotMeta *meta = filler.Meta(idx);
	meta->id = -99;
	meta->time_num = 1001;
	meta->time_den = 30000;
	meta->width = 3840;
	meta->height = 2160;
	meta->format = int(olive::core::PixelFormat::F32);
	meta->channel_count = 4;
	meta->linesize = 3840 * 4 * 4;
	meta->data_size = int32_t(kSlotBytes);
	const char kColorspace[] = "acescg";
	std::strncpy(meta->colorspace, kColorspace, sizeof(meta->colorspace) - 1);
	meta->colorspace[sizeof(meta->colorspace) - 1] = '\0';

	ASSERT_TRUE(filler.Publish(idx));

	uint32_t got = 0;
	ASSERT_TRUE(drainer.Consume(&got));
	EXPECT_EQ(got, idx);

	const olive::ipc::FrameSlotMeta *out = const_drainer.Meta(got);
	EXPECT_EQ(out->id, -99);
	EXPECT_EQ(out->time_num, 1001);
	EXPECT_EQ(out->time_den, 30000);
	EXPECT_EQ(out->width, 3840);
	EXPECT_EQ(out->height, 2160);
	EXPECT_EQ(out->format, int(olive::core::PixelFormat::F32));
	EXPECT_EQ(out->channel_count, 4);
	EXPECT_EQ(out->linesize, 3840 * 4 * 4);
	EXPECT_EQ(out->data_size, int32_t(kSlotBytes));
	EXPECT_STREQ(out->colorspace, kColorspace);

	EXPECT_TRUE(drainer.Release(got));
}

TEST(FrameSlotPool, FreeSlotsAreIssuedInOrder)
{
	constexpr uint32_t kSlots = 4;
	std::vector<uint8_t> mem(
		olive::ipc::FrameSlotPool::BytesNeeded(kSlots, 64));
	olive::ipc::FrameSlotPool pool =
		olive::ipc::FrameSlotPool::Create(mem.data(), kSlots, 64);

	// Create() seeds the free ring FIFO with every slot index.
	for (uint32_t expected = 0; expected < kSlots; expected++) {
		uint32_t idx = 0;
		ASSERT_TRUE(pool.Acquire(&idx));
		EXPECT_EQ(idx, expected);
	}

	uint32_t overflow = 0;
	EXPECT_FALSE(pool.Acquire(&overflow));

	// Released slots are re-issued in the order they were released.
	ASSERT_TRUE(pool.Release(2));
	ASSERT_TRUE(pool.Release(0));
	uint32_t idx = 0;
	ASSERT_TRUE(pool.Acquire(&idx));
	EXPECT_EQ(idx, 2u);
	ASSERT_TRUE(pool.Acquire(&idx));
	EXPECT_EQ(idx, 0u);
}

TEST(FrameSlotPool, ReadyRingDeliversInPublishOrder)
{
	constexpr uint32_t kSlots = 3;
	std::vector<uint8_t> mem(
		olive::ipc::FrameSlotPool::BytesNeeded(kSlots, 64));
	olive::ipc::FrameSlotPool pool =
		olive::ipc::FrameSlotPool::Create(mem.data(), kSlots, 64);

	uint32_t a = 0, b = 0, c = 0;
	ASSERT_TRUE(pool.Acquire(&a));
	ASSERT_TRUE(pool.Acquire(&b));
	ASSERT_TRUE(pool.Acquire(&c));

	// Publish order, not slot order, determines consume order.
	ASSERT_TRUE(pool.Publish(c));
	ASSERT_TRUE(pool.Publish(a));
	ASSERT_TRUE(pool.Publish(b));

	const uint32_t expected[] = { c, a, b };
	for (uint32_t want : expected) {
		uint32_t got = 0;
		ASSERT_TRUE(pool.Consume(&got));
		EXPECT_EQ(got, want);
		ASSERT_TRUE(pool.Release(got));
	}

	uint32_t empty = 0;
	EXPECT_FALSE(pool.Consume(&empty));
}

TEST(FrameSlotPool, CrossMappingHandoff)
{
	constexpr uint32_t kSlots = 2;
	constexpr size_t kSlotBytes = 128;
	const QString key = TestShmKey("pool-handoff");

	const size_t bytes =
		olive::ipc::FrameSlotPool::BytesNeeded(kSlots, kSlotBytes);

	olive::ipc::SharedMemoryRegion owner_region;
	ASSERT_TRUE(owner_region.Open(key, bytes,
								  olive::ipc::SharedMemoryRegion::kCreate))
		<< owner_region.error().toStdString();
	olive::ipc::FrameSlotPool filler = olive::ipc::FrameSlotPool::Create(
		owner_region.data(), kSlots, kSlotBytes);
	ASSERT_TRUE(filler.IsValid());

	// The peer maps the same segment separately and attaches to the pool header.
	olive::ipc::SharedMemoryRegion peer_region;
	ASSERT_TRUE(peer_region.Open(key, bytes,
								 olive::ipc::SharedMemoryRegion::kAttach))
		<< peer_region.error().toStdString();
	olive::ipc::FrameSlotPool drainer =
		olive::ipc::FrameSlotPool::Attach(peer_region.data());
	ASSERT_TRUE(drainer.IsValid());
	EXPECT_EQ(drainer.slot_count(), kSlots);
	EXPECT_EQ(drainer.slot_data_bytes(), kSlotBytes);

	// Filler side: acquire a slot, stamp it, publish it.
	uint32_t idx = 0;
	ASSERT_TRUE(filler.Acquire(&idx));
	auto *data = static_cast<uint8_t *>(filler.SlotData(idx));
	for (size_t i = 0; i < kSlotBytes; i++) {
		data[i] = uint8_t(0xC3 ^ i);
	}
	filler.Meta(idx)->id = 777;
	ASSERT_TRUE(filler.Publish(idx));

	// Drainer side (through the second mapping): same slot, meta and pixels.
	uint32_t got = 0;
	ASSERT_TRUE(drainer.Consume(&got));
	EXPECT_EQ(got, idx);
	EXPECT_EQ(drainer.Meta(got)->id, 777);
	const auto *peer_data =
		static_cast<const uint8_t *>(drainer.SlotData(got));
	for (size_t i = 0; i < kSlotBytes; i++) {
		ASSERT_EQ(peer_data[i], uint8_t(0xC3 ^ i)) << "byte " << i;
	}
	ASSERT_TRUE(drainer.Release(got));

	// The release crosses back to the owner's mapping. The free ring is FIFO:
	// the next fresh slot comes first, then the released slot cycles back.
	uint32_t reacquired = 0;
	ASSERT_TRUE(filler.Acquire(&reacquired));
	EXPECT_EQ(reacquired, 1u);
	ASSERT_TRUE(filler.Acquire(&reacquired));
	EXPECT_EQ(reacquired, got);
}

// ============================================================================
// NDJSON control messages (beyond render_ipc_test.cpp)
// ============================================================================

TEST(IpcMessage, LoadGraphRoundTrip)
{
	olive::ipc::LoadGraphMsg msg;
	msg.path = QStringLiteral("/tmp/oak-render-graph-abc123.ove");

	const QJsonObject obj = msg.ToJson();
	EXPECT_EQ(obj.value(QStringLiteral("type")).toString(),
			  QLatin1String(olive::ipc::msgtype::kLoadGraph));

	olive::ipc::LoadGraphMsg back;
	ASSERT_TRUE(olive::ipc::LoadGraphMsg::FromJson(obj, &back));
	EXPECT_EQ(back.path, msg.path);
}

TEST(IpcMessage, RenderFrameColorTransformRoundTrip)
{
	olive::ipc::RenderFrameMsg msg;
	msg.ticket_id = 123;
	msg.node_uuid = QStringLiteral("{11111111-2222-3333-4444-555555555555}");
	msg.time_num = 1001;
	msg.time_den = 24000;
	msg.width = 1920;
	msg.height = 1080;
	msg.format = int(olive::core::PixelFormat::F32);
	msg.channel_count = 4;
	msg.mode = 1;
	msg.input_slots = { 0, 2, 5 };
	msg.has_color_transform = true;
	msg.color_is_display = true;
	msg.color_output = QStringLiteral("sRGB - Display");
	msg.color_view = QStringLiteral("ACES 1.0 SDR-video");
	msg.color_look = QStringLiteral("None");

	const QJsonObject obj = msg.ToJson();
	EXPECT_EQ(obj.value(QStringLiteral("type")).toString(),
			  QLatin1String(olive::ipc::msgtype::kRenderFrame));
	EXPECT_TRUE(obj.value(QStringLiteral("has_color_transform")).toBool());

	olive::ipc::RenderFrameMsg back;
	ASSERT_TRUE(olive::ipc::RenderFrameMsg::FromJson(obj, &back));
	EXPECT_EQ(back.ticket_id, msg.ticket_id);
	EXPECT_EQ(back.node_uuid, msg.node_uuid);
	EXPECT_EQ(back.time_num, msg.time_num);
	EXPECT_EQ(back.time_den, msg.time_den);
	EXPECT_EQ(back.width, msg.width);
	EXPECT_EQ(back.height, msg.height);
	EXPECT_EQ(back.format, msg.format);
	EXPECT_EQ(back.channel_count, msg.channel_count);
	EXPECT_EQ(back.mode, msg.mode);
	EXPECT_EQ(back.input_slots, msg.input_slots);
	EXPECT_TRUE(back.has_color_transform);
	EXPECT_TRUE(back.color_is_display);
	EXPECT_EQ(back.color_output, msg.color_output);
	EXPECT_EQ(back.color_view, msg.color_view);
	EXPECT_EQ(back.color_look, msg.color_look);
}

TEST(IpcMessage, RenderFrameOmitsColorTransformWhenUnset)
{
	olive::ipc::RenderFrameMsg msg; // has_color_transform defaults to false
	const QJsonObject obj = msg.ToJson();

	EXPECT_FALSE(obj.contains(QStringLiteral("has_color_transform")));
	EXPECT_FALSE(obj.contains(QStringLiteral("color_output")));
	EXPECT_FALSE(obj.contains(QStringLiteral("color_view")));
	EXPECT_FALSE(obj.contains(QStringLiteral("color_look")));

	olive::ipc::RenderFrameMsg back;
	ASSERT_TRUE(olive::ipc::RenderFrameMsg::FromJson(obj, &back));
	EXPECT_FALSE(back.has_color_transform);
	EXPECT_FALSE(back.color_is_display);
	EXPECT_TRUE(back.color_output.isEmpty());
}

TEST(IpcMessage, RenderFrameLegacyInputSlotFallback)
{
	// Older peers only send the scalar "input_slot"; FromJson folds it into the
	// input_slots array when the array is absent.
	QJsonObject obj;
	obj[QStringLiteral("type")] =
		QLatin1String(olive::ipc::msgtype::kRenderFrame);
	obj[QStringLiteral("ticket")] = 5.0;
	obj[QStringLiteral("input_slot")] = 3;

	olive::ipc::RenderFrameMsg back;
	ASSERT_TRUE(olive::ipc::RenderFrameMsg::FromJson(obj, &back));
	EXPECT_EQ(back.input_slot, 3);
	ASSERT_EQ(back.input_slots.size(), 1);
	EXPECT_EQ(back.input_slots.first(), 3);

	// When the array is present it wins and the scalar is not duplicated.
	obj[QStringLiteral("input_slots")] = QJsonArray{ 7, 8 };
	olive::ipc::RenderFrameMsg back2;
	ASSERT_TRUE(olive::ipc::RenderFrameMsg::FromJson(obj, &back2));
	ASSERT_EQ(back2.input_slots.size(), 2);
	EXPECT_EQ(back2.input_slots.at(0), 7);
	EXPECT_EQ(back2.input_slots.at(1), 8);
}

TEST(IpcMessage, RenderFrameDefaultsFromSparseJson)
{
	// A message carrying only the type tag must still parse, with every field
	// falling back to its documented default.
	QJsonObject obj;
	obj[QStringLiteral("type")] =
		QLatin1String(olive::ipc::msgtype::kRenderFrame);

	olive::ipc::RenderFrameMsg back;
	ASSERT_TRUE(olive::ipc::RenderFrameMsg::FromJson(obj, &back));
	EXPECT_EQ(back.ticket_id, 0);
	EXPECT_TRUE(back.node_uuid.isEmpty());
	EXPECT_EQ(back.time_num, 0);
	EXPECT_EQ(back.time_den, 1);
	EXPECT_EQ(back.width, 0);
	EXPECT_EQ(back.height, 0);
	EXPECT_EQ(back.format, -1);
	EXPECT_EQ(back.channel_count, 0);
	EXPECT_EQ(back.mode, 0);
	EXPECT_EQ(back.input_slot, -1);
	EXPECT_TRUE(back.input_slots.isEmpty());
	EXPECT_FALSE(back.has_color_transform);
}

TEST(IpcMessage, LargeIdentifiersSurviveRoundTrip)
{
	// 64-bit ids travel as JSON doubles, exact up to 2^53; ticket ids are
	// pointer-derived and slot sizes are byte counts, both well inside that.
	const qint64 ticket = (qint64(1) << 52) + 12345;
	const qint64 slot_bytes = qint64(7680) * 4320 * 4 * 4; // 8K RGBA float

	olive::ipc::RenderFrameMsg rf;
	rf.ticket_id = ticket;
	rf.time_num = qint64(48000) * 123456789;
	rf.time_den = qint64(1) << 40;
	olive::ipc::RenderFrameMsg rf_back;
	ASSERT_TRUE(olive::ipc::RenderFrameMsg::FromJson(rf.ToJson(), &rf_back));
	EXPECT_EQ(rf_back.ticket_id, ticket);
	EXPECT_EQ(rf_back.time_num, rf.time_num);
	EXPECT_EQ(rf_back.time_den, rf.time_den);

	olive::ipc::FrameReadyMsg fr;
	fr.ticket_id = ticket;
	olive::ipc::FrameReadyMsg fr_back;
	ASSERT_TRUE(olive::ipc::FrameReadyMsg::FromJson(fr.ToJson(), &fr_back));
	EXPECT_EQ(fr_back.ticket_id, ticket);

	olive::ipc::CancelMsg cancel;
	cancel.ticket_id = ticket;
	olive::ipc::CancelMsg cancel_back;
	ASSERT_TRUE(olive::ipc::CancelMsg::FromJson(cancel.ToJson(), &cancel_back));
	EXPECT_EQ(cancel_back.ticket_id, ticket);

	olive::ipc::HandshakeMsg hs;
	hs.slot_data_bytes = slot_bytes;
	hs.input_slot_data_bytes = slot_bytes / 2;
	olive::ipc::HandshakeMsg hs_back;
	ASSERT_TRUE(olive::ipc::HandshakeMsg::FromJson(hs.ToJson(), &hs_back));
	EXPECT_EQ(hs_back.slot_data_bytes, slot_bytes);
	EXPECT_EQ(hs_back.input_slot_data_bytes, slot_bytes / 2);
}

TEST(IpcMessage, TypedBuildersRejectMismatchedType)
{
	const QJsonObject hs_obj = olive::ipc::HandshakeMsg().ToJson();
	const QJsonObject rf_obj = olive::ipc::RenderFrameMsg().ToJson();
	const QJsonObject fr_obj = olive::ipc::FrameReadyMsg().ToJson();
	const QJsonObject cancel_obj = olive::ipc::CancelMsg().ToJson();
	const QJsonObject load_obj = olive::ipc::LoadGraphMsg().ToJson();

	olive::ipc::HandshakeMsg hs_out;
	EXPECT_FALSE(olive::ipc::HandshakeMsg::FromJson(rf_obj, &hs_out));
	olive::ipc::RenderFrameMsg rf_out;
	EXPECT_FALSE(olive::ipc::RenderFrameMsg::FromJson(cancel_obj, &rf_out));
	olive::ipc::FrameReadyMsg fr_out;
	EXPECT_FALSE(olive::ipc::FrameReadyMsg::FromJson(load_obj, &fr_out));
	olive::ipc::CancelMsg cancel_out;
	EXPECT_FALSE(olive::ipc::CancelMsg::FromJson(fr_obj, &cancel_out));
	olive::ipc::LoadGraphMsg load_out;
	EXPECT_FALSE(olive::ipc::LoadGraphMsg::FromJson(hs_obj, &load_out));

	// An object with no "type" at all is rejected by every parser.
	const QJsonObject empty;
	EXPECT_FALSE(olive::ipc::HandshakeMsg::FromJson(empty, &hs_out));
	EXPECT_FALSE(olive::ipc::RenderFrameMsg::FromJson(empty, &rf_out));
	EXPECT_FALSE(olive::ipc::FrameReadyMsg::FromJson(empty, &fr_out));
	EXPECT_FALSE(olive::ipc::CancelMsg::FromJson(empty, &cancel_out));
	EXPECT_FALSE(olive::ipc::LoadGraphMsg::FromJson(empty, &load_out));
}

TEST(IpcMessage, ReadMessageSkipsBlankLines)
{
	olive::ipc::CancelMsg cancel;
	cancel.ticket_id = 9;
	const QByteArray line =
		QJsonDocument(cancel.ToJson()).toJson(QJsonDocument::Compact);

	// A reader loop sees: blank line, whitespace-only line, then a real
	// message. Blank lines are skipped silently.
	QByteArray reader = QByteArray("\n   \n") + line + '\n';

	QJsonObject obj;
	bool ok = true;
	ASSERT_TRUE(olive::ipc::ReadMessage(&reader, &obj, &ok));
	EXPECT_TRUE(ok);
	olive::ipc::CancelMsg back;
	ASSERT_TRUE(olive::ipc::CancelMsg::FromJson(obj, &back));
	EXPECT_EQ(back.ticket_id, 9);
	EXPECT_TRUE(reader.isEmpty());
}

TEST(IpcMessage, ReadMessageRejectsNonObjectJson)
{
	// Valid JSON, but an array rather than an object: consumed, flagged not-ok.
	QByteArray reader = QByteArray("[1,2,3]\n");
	QJsonObject obj;
	bool ok = true;
	EXPECT_FALSE(olive::ipc::ReadMessage(&reader, &obj, &ok));
	EXPECT_FALSE(ok);
	EXPECT_TRUE(reader.isEmpty());
}

TEST(IpcMessage, ReadMessageWorksWithoutOkPointer)
{
	olive::ipc::CancelMsg cancel;
	cancel.ticket_id = 4;
	QByteArray reader =
		QJsonDocument(cancel.ToJson()).toJson(QJsonDocument::Compact);
	reader.append('\n');

	QJsonObject obj;
	EXPECT_TRUE(olive::ipc::ReadMessage(&reader, &obj)); // ok defaults to nullptr

	QByteArray bad = QByteArray("garbage\n");
	EXPECT_FALSE(olive::ipc::ReadMessage(&bad, &obj));
}

TEST(IpcMessage, WriteMessageProducesSingleTerminatedLine)
{
	QByteArray storage;
	QBuffer device(&storage);
	ASSERT_TRUE(device.open(QIODevice::WriteOnly));

	olive::ipc::HandshakeMsg hs;
	hs.protocol_version = 1;
	hs.shm_key = QStringLiteral("olive-rw-1-0");
	ASSERT_TRUE(olive::ipc::WriteMessage(&device, hs.ToJson()));
	device.close();

	// NDJSON: exactly one compact line, newline-terminated.
	EXPECT_TRUE(storage.startsWith('{'));
	EXPECT_TRUE(storage.endsWith('\n'));
	EXPECT_EQ(storage.count('\n'), 1);

	// And it parses back to an identical object.
	QJsonObject obj;
	bool ok = false;
	ASSERT_TRUE(olive::ipc::ReadMessage(&storage, &obj, &ok));
	EXPECT_TRUE(ok);
	EXPECT_EQ(obj, hs.ToJson());
}

TEST(IpcMessage, WriteMessageFailsOnClosedDevice)
{
	QByteArray storage;
	QBuffer device(&storage); // never opened: writes fail

	olive::ipc::CancelMsg cancel;
	EXPECT_FALSE(olive::ipc::WriteMessage(&device, cancel.ToJson()));
	EXPECT_TRUE(storage.isEmpty());
}

TEST(IpcMessage, MessageTypeConstantsAreDistinct)
{
	const QSet<QString> types = {
		QString::fromUtf8(olive::ipc::msgtype::kHandshake),
		QString::fromUtf8(olive::ipc::msgtype::kLoadGraph),
		QString::fromUtf8(olive::ipc::msgtype::kRenderFrame),
		QString::fromUtf8(olive::ipc::msgtype::kFrameReady),
		QString::fromUtf8(olive::ipc::msgtype::kCancel),
		QString::fromUtf8(olive::ipc::msgtype::kGraphUpdate),
		QString::fromUtf8(olive::ipc::msgtype::kShutdown),
		QString::fromUtf8(olive::ipc::msgtype::kError),
	};
	EXPECT_EQ(types.size(), 8);

	// Each builder stamps its own constant into the "type" field.
	EXPECT_EQ(olive::ipc::HandshakeMsg()
				  .ToJson()
				  .value(QStringLiteral("type"))
				  .toString(),
			  QLatin1String(olive::ipc::msgtype::kHandshake));
	EXPECT_EQ(olive::ipc::RenderFrameMsg()
				  .ToJson()
				  .value(QStringLiteral("type"))
				  .toString(),
			  QLatin1String(olive::ipc::msgtype::kRenderFrame));
	EXPECT_EQ(olive::ipc::FrameReadyMsg()
				  .ToJson()
				  .value(QStringLiteral("type"))
				  .toString(),
			  QLatin1String(olive::ipc::msgtype::kFrameReady));
	EXPECT_EQ(olive::ipc::CancelMsg()
				  .ToJson()
				  .value(QStringLiteral("type"))
				  .toString(),
			  QLatin1String(olive::ipc::msgtype::kCancel));
	EXPECT_EQ(olive::ipc::LoadGraphMsg()
				  .ToJson()
				  .value(QStringLiteral("type"))
				  .toString(),
			  QLatin1String(olive::ipc::msgtype::kLoadGraph));
}

// ============================================================================
// RenderWorkerPool (validation paths that never reach a worker process)
// ============================================================================

TEST(RenderWorkerPool, RemoveTicketRejectsNull)
{
	olive::DecoderCache cache;
	olive::RenderWorkerPool pool(&cache, QStringLiteral("cpu"));

	EXPECT_FALSE(pool.RemoveTicket(nullptr));
}

TEST(RenderWorkerPool, RemoveTicketUnknownTicketReturnsFalse)
{
	olive::DecoderCache cache;
	olive::RenderWorkerPool pool(&cache, QStringLiteral("cpu"));

	// The pool thread was never started, so the ticket can be neither queued
	// nor active.
	const olive::RenderTicketPtr ticket = std::make_shared<olive::RenderTicket>();
	EXPECT_FALSE(pool.RemoveTicket(ticket));
}

TEST(RenderWorkerPool, ShutdownWithoutStartIsSafeAndIdempotent)
{
	olive::DecoderCache cache;
	olive::RenderWorkerPool pool(&cache, QStringLiteral("cpu"));

	// Shutdown on a pool whose thread never ran must not block or crash; the
	// destructor runs it once more when the pool goes out of scope.
	pool.Shutdown();
	pool.Shutdown();
	EXPECT_FALSE(pool.isRunning());
}

TEST(RenderWorkerPool, SubmitFrameRejectsNullNode)
{
	olive::DecoderCache cache;
	olive::RenderWorkerPool pool(&cache, QStringLiteral("cpu"));

	const olive::RenderTicketPtr ticket = std::make_shared<olive::RenderTicket>();
	EXPECT_FALSE(pool.SubmitFrame(
		ticket,
		MakeVideoParams(nullptr, olive::VideoParams(
									 64, 64, olive::core::PixelFormat::U8, 4))));

	// A rejected submission must leave the ticket untouched.
	EXPECT_FALSE(ticket->IsRunning());
	EXPECT_EQ(ticket->GetFinishCount(), 0);
}

TEST(RenderWorkerPool, SubmitFrameRejectsInvalidVideoParams)
{
	olive::DecoderCache cache;
	olive::RenderWorkerPool pool(&cache, QStringLiteral("cpu"));

	// A real node, but a default (zero-sized) VideoParams fails validation
	// before any project resolution or decode work happens.
	olive::Track track;
	ASSERT_FALSE(olive::VideoParams().is_valid());

	const olive::RenderTicketPtr ticket = std::make_shared<olive::RenderTicket>();
	EXPECT_FALSE(
		pool.SubmitFrame(ticket, MakeVideoParams(&track, olive::VideoParams())));
	EXPECT_FALSE(ticket->IsRunning());
}

TEST(RenderWorkerPool, SubmitFrameRejectsNonFrameReturnType)
{
	olive::DecoderCache cache;
	olive::RenderWorkerPool pool(&cache, QStringLiteral("cpu"));

	olive::Track track;
	olive::RenderManager::RenderVideoParams params = MakeVideoParams(
		&track, olive::VideoParams(64, 64, olive::core::PixelFormat::U8, 4));
	params.return_type = olive::RenderManager::kTexture;

	const olive::RenderTicketPtr ticket = std::make_shared<olive::RenderTicket>();
	EXPECT_FALSE(pool.SubmitFrame(ticket, params));
	EXPECT_FALSE(ticket->IsRunning());
}

// ============================================================================
// DecoderCache
// ============================================================================

TEST(DecoderCache, DefaultPairAndInsertRoundTrip)
{
	olive::DecoderCache cache;
	const olive::Decoder::CodecStream stream(
		QStringLiteral("/nonexistent/source.mov"), 2, nullptr);

	// Missing entries yield a default DecoderPair.
	const olive::DecoderPair missing = cache.value(stream);
	EXPECT_EQ(missing.decoder, nullptr);
	EXPECT_EQ(missing.last_modified, 0);

	olive::DecoderPair pair;
	pair.last_modified = qint64(1700000000123);
	cache.insert(stream, pair);

	const olive::DecoderPair fetched = cache.value(stream);
	EXPECT_EQ(fetched.decoder, nullptr);
	EXPECT_EQ(fetched.last_modified, qint64(1700000000123));

	// The cache exposes its mutex for locked access (used by the worker pool).
	EXPECT_NE(cache.mutex(), nullptr);

	// A different stream is an independent entry.
	const olive::Decoder::CodecStream other(
		QStringLiteral("/nonexistent/other.mov"), 2, nullptr);
	EXPECT_EQ(cache.value(other).last_modified, 0);
}
