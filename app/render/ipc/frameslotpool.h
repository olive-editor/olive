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

#ifndef IPC_FRAMESLOTPOOL_H
#define IPC_FRAMESLOTPOOL_H

#include <cstddef>
#include <cstdint>

#include "spscringbuffer.h"

namespace olive
{
namespace ipc
{

/**
 * @brief Per-slot metadata describing the frame currently occupying a slot.
 *
 * Trivially-copyable POD that lives in shared memory alongside the pixel data. Carries everything
 * the consumer needs to reconstruct an olive::Frame without any out-of-band information. We store
 * the rational timestamp as an explicit numerator/denominator pair to stay POD (olive::rational is
 * not guaranteed shared-memory-safe).
 */
struct FrameSlotMeta {
	int64_t id; ///< Caller-defined tag (e.g. ticket id, or footage stream hash).
	int64_t time_num; ///< Frame timestamp numerator.
	int64_t time_den; ///< Frame timestamp denominator.
	int32_t width;
	int32_t height;
	int32_t format; ///< olive::PixelFormat::Format value.
	int32_t channel_count;
	int32_t linesize; ///< Bytes per scanline (stride).
	int32_t data_size; ///< Valid bytes written into the slot's data block.
	char colorspace[128]; ///< Input colorspace name for color-managed footage.
};

/**
 * @brief A fixed-size pool of equal-sized frame slots in shared memory, with lock-free hand-off.
 *
 * One pool models a single direction of frame flow (e.g. worker -> main for rendered output, or
 * main -> worker for decoded input). Ownership of a slot is transferred via two SPSC ring buffers
 * of slot indices, so no mutex is ever taken:
 *
 *   - free_ring:  indices of slots available to the FILLER. The drainer returns slots here.
 *   - ready_ring: indices of slots holding a published frame, produced by the FILLER for the
 *                 DRAINER to consume.
 *
 * Lifecycle (filler = producer of frames, drainer = consumer of frames):
 *   filler:  Acquire() -> pop a free index -> write meta + pixels -> Publish() -> push to ready
 *   drainer: Consume() -> pop a ready index -> read meta + pixels -> Release() -> push to free
 *
 * Because each ring has exactly one producer and one consumer (the filler owns free.Pop +
 * ready.Push, the drainer owns ready.Pop + free.Push), the SPSC invariant holds and the whole
 * exchange is lock-free.
 *
 * All slots are sized to `slot_data_bytes`, computed for the maximum supported frame (e.g. 8K RGBA
 * half-float). Frames smaller than that simply use a prefix of the slot.
 *
 * The pool does NOT own the memory; it is constructed over a SharedMemoryRegion mapping. Use
 * BytesNeeded() to size that region.
 */
class FrameSlotPool {
public:
	/**
   * @brief Total bytes a region must provide to back a pool of `slot_count` x `slot_data_bytes`.
   */
	static size_t BytesNeeded(uint32_t slot_count, size_t slot_data_bytes);

	/**
   * @brief Lay out and initialize a brand-new pool over `mem` (owner side, once).
   *
   * Initializes both rings, seeds the free ring with every slot index, and zeroes metadata.
   * `mem` must provide at least BytesNeeded(slot_count, slot_data_bytes) bytes.
   */
	static FrameSlotPool Create(void *mem, uint32_t slot_count,
								size_t slot_data_bytes);

	/**
   * @brief Map an existing, already-initialized pool (peer side).
   *
   * Reads slot_count/slot_data_bytes from the in-memory header written by Create().
   */
	static FrameSlotPool Attach(void *mem);

	bool IsValid() const
	{
		return header_ != nullptr;
	}

	uint32_t slot_count() const;
	size_t slot_data_bytes() const;

	// ---- Filler side ----

	/**
   * @brief Take ownership of a free slot. Returns false (and leaves *index untouched) if none free.
   */
	bool Acquire(uint32_t *index);

	/**
   * @brief Pointer to a slot's pixel data block (slot_data_bytes available).
   */
	void *SlotData(uint32_t index);

	/**
   * @brief Mutable metadata for a slot. Filler writes this before Publish().
   */
	FrameSlotMeta *Meta(uint32_t index);

	/**
   * @brief Publish a filled slot to the drainer. Must follow a successful Acquire() of `index`.
   */
	bool Publish(uint32_t index);

	// ---- Drainer side ----

	/**
   * @brief Take the next published slot. Returns false if nothing is ready.
   */
	bool Consume(uint32_t *index);

	/**
   * @brief Return a consumed slot to the free pool for reuse. Must follow Consume() of `index`.
   */
	bool Release(uint32_t index);

	const FrameSlotMeta *Meta(uint32_t index) const;
	const void *SlotData(uint32_t index) const;

public:
	FrameSlotPool() = default;

private:
	struct Header {
		uint32_t magic;
		uint32_t slot_count;
		uint64_t slot_data_bytes;
		// Byte offsets from the start of the segment to each sub-region.
		uint64_t free_ring_offset;
		uint64_t ready_ring_offset;
		uint64_t meta_offset;
		uint64_t data_offset;
	};

	static constexpr uint32_t kMagic = 0x4F4B5350; // 'OKSP'

	// Ring capacity must exceed slot_count by one because a ring can hold at most capacity-1 entries
	// and we need to be able to enqueue every slot at once.
	static uint32_t RingCapacity(uint32_t slot_count)
	{
		return slot_count + 1;
	}

	uint8_t *base_ = nullptr;
	Header *header_ = nullptr;
	SpscRingBuffer *free_ring_ = nullptr;
	SpscRingBuffer *ready_ring_ = nullptr;
	FrameSlotMeta *meta_ = nullptr;
	uint8_t *data_ = nullptr;
};

} // namespace ipc
} // namespace olive

#endif // IPC_FRAMESLOTPOOL_H