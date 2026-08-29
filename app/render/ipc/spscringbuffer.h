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

#ifndef IPC_SPSCRINGBUFFER_H
#define IPC_SPSCRINGBUFFER_H

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace olive
{
namespace ipc
{

/**
 * @brief A lock-free single-producer / single-consumer ring buffer of uint32 indices.
 *
 * This is the core synchronization primitive for cross-process communication. It is designed to
 * live in a shared memory segment: the control block (head/tail cursors) and the slot array are a
 * single trivially-copyable, self-contained POD. Both the producer process and the consumer
 * process map the same memory and operate on it concurrently.
 *
 * Correctness relies on the classic SPSC invariant:
 *   - Exactly ONE thread/process calls Push() (the producer).
 *   - Exactly ONE thread/process calls Pop() (the consumer).
 * Under that invariant, no mutex is required. The producer only ever writes `head_`, the consumer
 * only ever writes `tail_`, and the acquire/release ordering on those two atomics publishes the
 * slot writes safely across the process boundary.
 *
 * Capacity note: one slot is always left empty to disambiguate the full and empty states, so a
 * buffer constructed with kCapacity slots can hold at most (kCapacity - 1) live entries.
 *
 * The payload stored is a single uint32_t per entry, intended to be an index into a separately
 * managed slot pool (see FrameSlotPool). We never put pointers in shared memory.
 */
class SpscRingBuffer {
public:
	/**
   * @brief In-place construct a ring buffer header at `mem` with `capacity` index slots.
   *
   * `mem` must point to at least BytesNeeded(capacity) bytes of zero-initializable memory. This is
   * intended to be placement-style initialization performed exactly once by whichever process owns
   * the segment's creation; the peer process uses Attach() instead.
   */
	static SpscRingBuffer *Create(void *mem, uint32_t capacity)
	{
		auto *self = reinterpret_cast<SpscRingBuffer *>(mem);
		self->capacity_ = capacity;
		self->head_.store(0, std::memory_order_relaxed);
		self->tail_.store(0, std::memory_order_relaxed);
		for (uint32_t i = 0; i < capacity; i++) {
			self->slot_array()[i] = 0;
		}
		return self;
	}

	/**
   * @brief Re-interpret already-initialized memory as a ring buffer (peer process side).
   *
   * No writes are performed; the cursors and capacity are assumed already set by Create().
   */
	static SpscRingBuffer *Attach(void *mem)
	{
		return reinterpret_cast<SpscRingBuffer *>(mem);
	}

	/**
   * @brief Total bytes required to hold the header plus `capacity` index slots.
   */
	static size_t BytesNeeded(uint32_t capacity)
	{
		return sizeof(SpscRingBuffer) + size_t(capacity) * sizeof(uint32_t);
	}

	/**
   * @brief Producer side: enqueue an index. Returns false if the buffer is full.
   */
	bool Push(uint32_t value)
	{
		const uint32_t head = head_.load(std::memory_order_relaxed);
		const uint32_t next = Increment(head);

		// Buffer is full if advancing head would collide with the consumer's tail.
		if (next == tail_.load(std::memory_order_acquire)) {
			return false;
		}

		slot_array()[head] = value;
		head_.store(next, std::memory_order_release);
		return true;
	}

	/**
   * @brief Consumer side: dequeue an index into `out`. Returns false if the buffer is empty.
   */
	bool Pop(uint32_t *out)
	{
		const uint32_t tail = tail_.load(std::memory_order_relaxed);

		// Buffer is empty if the consumer has caught up to the producer.
		if (tail == head_.load(std::memory_order_acquire)) {
			return false;
		}

		*out = slot_array()[tail];
		tail_.store(Increment(tail), std::memory_order_release);
		return true;
	}

	/**
   * @brief Approximate number of entries currently queued.
   *
   * Safe to call from either side, but the value may be stale the instant it returns. Intended for
   * metrics/backpressure heuristics, not for correctness decisions.
   */
	uint32_t SizeApprox() const
	{
		const uint32_t head = head_.load(std::memory_order_acquire);
		const uint32_t tail = tail_.load(std::memory_order_acquire);
		return (head + capacity_ - tail) % capacity_;
	}

	bool IsEmptyApprox() const
	{
		return head_.load(std::memory_order_acquire) ==
			   tail_.load(std::memory_order_acquire);
	}

	uint32_t capacity() const
	{
		return capacity_;
	}

private:
	uint32_t Increment(uint32_t index) const
	{
		// capacity_ is small and this avoids requiring a power-of-two capacity.
		return (index + 1) % capacity_;
	}

	// The index slot array is allocated immediately after this struct in the same contiguous block.
	// (Named slot_array() rather than slots() to avoid Qt's `slots` keyword macro.)
	uint32_t *slot_array()
	{
		return reinterpret_cast<uint32_t *>(this + 1);
	}
	const uint32_t *slot_array() const
	{
		return reinterpret_cast<const uint32_t *>(this + 1);
	}

	// Producer writes head_, consumer writes tail_. Kept on separate cache lines would be ideal, but
	// since these live in shared memory with a trailing flexible array we keep the header compact and
	// rely on acquire/release ordering for correctness.
	std::atomic<uint32_t> head_;
	std::atomic<uint32_t> tail_;
	uint32_t capacity_;

	static_assert(
		sizeof(std::atomic<uint32_t>) == sizeof(uint32_t),
		"atomic<uint32_t> must be lock-free POD-sized for shared memory use");
};

} // namespace ipc
} // namespace olive

#endif // IPC_SPSCRINGBUFFER_H
