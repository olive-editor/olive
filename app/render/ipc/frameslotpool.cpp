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

#include "frameslotpool.h"

#include <cstring>

namespace olive
{
namespace ipc
{

namespace
{

// Round `value` up to the next multiple of `align` (align must be a power of two).
size_t AlignUp(size_t value, size_t align)
{
	return (value + (align - 1)) & ~(align - 1);
}

constexpr size_t kAlign = 64; // Cache-line alignment for each sub-region.

} // namespace

size_t FrameSlotPool::BytesNeeded(uint32_t slot_count, size_t slot_data_bytes)
{
	const uint32_t ring_cap = RingCapacity(slot_count);
	size_t total = AlignUp(sizeof(Header), kAlign);
	total +=
		AlignUp(SpscRingBuffer::BytesNeeded(ring_cap), kAlign); // free ring
	total +=
		AlignUp(SpscRingBuffer::BytesNeeded(ring_cap), kAlign); // ready ring
	total +=
		AlignUp(sizeof(FrameSlotMeta) * slot_count, kAlign); // metadata array
	total += AlignUp(slot_data_bytes, kAlign) * slot_count; // pixel data blocks
	return total;
}

FrameSlotPool FrameSlotPool::Create(void *mem, uint32_t slot_count,
									size_t slot_data_bytes)
{
	FrameSlotPool pool;
	pool.base_ = reinterpret_cast<uint8_t *>(mem);

	const uint32_t ring_cap = RingCapacity(slot_count);

	size_t offset = 0;
	const size_t header_off = offset;
	offset += AlignUp(sizeof(Header), kAlign);

	const size_t free_off = offset;
	offset += AlignUp(SpscRingBuffer::BytesNeeded(ring_cap), kAlign);

	const size_t ready_off = offset;
	offset += AlignUp(SpscRingBuffer::BytesNeeded(ring_cap), kAlign);

	const size_t meta_off = offset;
	offset += AlignUp(sizeof(FrameSlotMeta) * slot_count, kAlign);

	const size_t data_off = offset;

	pool.header_ = reinterpret_cast<Header *>(pool.base_ + header_off);
	pool.header_->magic = kMagic;
	pool.header_->slot_count = slot_count;
	pool.header_->slot_data_bytes = slot_data_bytes;
	pool.header_->free_ring_offset = free_off;
	pool.header_->ready_ring_offset = ready_off;
	pool.header_->meta_offset = meta_off;
	pool.header_->data_offset = data_off;

	pool.free_ring_ = SpscRingBuffer::Create(pool.base_ + free_off, ring_cap);
	pool.ready_ring_ = SpscRingBuffer::Create(pool.base_ + ready_off, ring_cap);
	pool.meta_ = reinterpret_cast<FrameSlotMeta *>(pool.base_ + meta_off);
	pool.data_ = pool.base_ + data_off;

	memset(pool.meta_, 0, sizeof(FrameSlotMeta) * slot_count);

	// Seed the free ring with every slot index so the filler can Acquire() immediately.
	for (uint32_t i = 0; i < slot_count; i++) {
		pool.free_ring_->Push(i);
	}

	return pool;
}

FrameSlotPool FrameSlotPool::Attach(void *mem)
{
	FrameSlotPool pool;
	pool.base_ = reinterpret_cast<uint8_t *>(mem);
	pool.header_ = reinterpret_cast<Header *>(pool.base_);

	if (pool.header_->magic != kMagic) {
		// Caller will see IsValid() == false via a null header reset.
		pool.header_ = nullptr;
		pool.base_ = nullptr;
		return pool;
	}

	pool.free_ring_ =
		SpscRingBuffer::Attach(pool.base_ + pool.header_->free_ring_offset);
	pool.ready_ring_ =
		SpscRingBuffer::Attach(pool.base_ + pool.header_->ready_ring_offset);
	pool.meta_ = reinterpret_cast<FrameSlotMeta *>(pool.base_ +
												   pool.header_->meta_offset);
	pool.data_ = pool.base_ + pool.header_->data_offset;

	return pool;
}

uint32_t FrameSlotPool::slot_count() const
{
	return header_ ? header_->slot_count : 0;
}

size_t FrameSlotPool::slot_data_bytes() const
{
	return header_ ? size_t(header_->slot_data_bytes) : 0;
}

bool FrameSlotPool::Acquire(uint32_t *index)
{
	return free_ring_->Pop(index);
}

void *FrameSlotPool::SlotData(uint32_t index)
{
	return data_ + size_t(index) * AlignUp(slot_data_bytes(), kAlign);
}

const void *FrameSlotPool::SlotData(uint32_t index) const
{
	return data_ + size_t(index) * AlignUp(slot_data_bytes(), kAlign);
}

FrameSlotMeta *FrameSlotPool::Meta(uint32_t index)
{
	return &meta_[index];
}

const FrameSlotMeta *FrameSlotPool::Meta(uint32_t index) const
{
	return &meta_[index];
}

bool FrameSlotPool::Publish(uint32_t index)
{
	return ready_ring_->Push(index);
}

bool FrameSlotPool::Consume(uint32_t *index)
{
	return ready_ring_->Pop(index);
}

bool FrameSlotPool::Release(uint32_t index)
{
	return free_ring_->Push(index);
}

} // namespace ipc
} // namespace olive