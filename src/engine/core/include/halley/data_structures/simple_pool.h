#pragma once

#include <cstdint>
#include <list>
#include <mutex>
#include "halley/data_structures/vector.h"

namespace Halley {
	template <size_t size, size_t align, size_t blockLen = 16384, bool threadSafe = true>
	class FixedBytePool {
		template <typename T>
		[[nodiscard]] constexpr static T alignUp(T val, T alignment)
		{
			return val + (alignment - (val % alignment)) % alignment;
		}

		constexpr static size_t nEntries = blockLen / alignUp(size, align);
		static_assert(nEntries >= 1);

		struct Entry {
			union {
				alignas(align) std::array<char, size> data;
				Entry* nextFreeEntry;
			};
		};

		struct Block {
			Vector<Entry> data;

			Block()
				: data(nEntries)
			{
				for (size_t i = 1; i < nEntries; i++) {
					// Each entry points to the next
					data[i - 1].nextFreeEntry = &data[i];
				}
				data.back().nextFreeEntry = nullptr;
			}

			Block(const Block& other) = delete;
			Block& operator=(const Block& other) = delete;
			Block(Block&& other) noexcept = delete;
			Block& operator=(Block&& other) noexcept = delete;

			bool ownsPointer(const Entry* ptr) const
			{
				return ptr >= &data.front() && ptr <= &data.back();
			}
		};

	public:
		void* alloc() {
			auto lock = lockMutex();

			// Create a new block if there's no next entry
			if (!next) {
				auto& block = blocks.emplace_back();
				next = &block.data.front();
			}

			// Get next block
			Entry* result = next;
			next = result->nextFreeEntry;

			// Return block as data
			return result->data.data();
		}

		void free(void* p) {
			assert(ownsPointer(p));

			// Convert back to entry
			Entry* entry = static_cast<Entry*>(p);

			// Store previous next on this and set it as the new next
			auto lock = lockMutex();
			entry->nextFreeEntry = next;
			next = entry;
		}

		bool ownsPointer(void* p) const
		{
			auto lock = lockMutex();

			for (auto& block: blocks) {
				if (block.ownsPointer(static_cast<Entry*>(p))) {
					return true;
				}
			}
			return false;
		}

	private:
		std::list<Block> blocks;
		Entry* next = nullptr;

		mutable std::mutex mutex;

		std::unique_lock<std::mutex> lockMutex() const
		{
			if constexpr (threadSafe) {
				return std::unique_lock<std::mutex>(mutex);
			} else {
				return std::unique_lock<std::mutex>();
			}
		}
	};

	template <typename T, size_t blockLen = 16384, bool threadSafe = true>
	class TypedPool : private FixedBytePool<sizeof(T), alignof(T), blockLen, threadSafe> {
	public:
		T* alloc() {
			return static_cast<T*>(FixedBytePool<sizeof(T), alignof(T), blockLen, threadSafe>::alloc());
		}

		void free(T* p) {
			FixedBytePool<sizeof(T), alignof(T), blockLen, threadSafe>::free(p);
		}
	};
}
