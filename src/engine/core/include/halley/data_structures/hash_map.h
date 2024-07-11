#pragma once

#include "../../../../../contrib/skarupke/flat_hash_map.hpp"
#include <string_view>

namespace Halley {

	class String;

	template <typename T>
	struct EqualToPicker {
		using type = std::equal_to<T>;
	};
	
	template <>
	struct EqualToPicker<String> {
		using type = std::equal_to<std::string_view>;
	};

	template<typename Key, typename Value, typename Hash = std::hash<Key>>
	using HashMap = ska::flat_hash_map<Key, Value, Hash, typename EqualToPicker<Key>::type>;

	template<typename Key, typename Hash = std::hash<Key>>
	using HashSet = ska::flat_hash_set<Key, Hash, typename EqualToPicker<Key>::type>;

	static inline uint32_t combineHash32(uint32_t a, uint32_t b)
	{
		// From https://stackoverflow.com/a/27952689
		return a ^ (b + 0x9e3779b9u + (a << 6) + (a >> 2));
	}

	static inline uint64_t combineHash64(uint64_t a, uint64_t b)
	{
		// From https://stackoverflow.com/a/27952689
		return a ^ (b + 0x517cc1b727220a95ull + (a << 6) + (a >> 2));
	}

	static inline size_t combineHash(size_t a, size_t b)
	{
		if constexpr (sizeof(size_t) == 8) {
			return static_cast<size_t>(combineHash64(static_cast<uint64_t>(a), static_cast<uint64_t>(b)));
		} else {
			return static_cast<size_t>(combineHash32(static_cast<uint32_t>(a), static_cast<uint32_t>(b)));
		}
	}
}

namespace std {
	template<typename A, typename B>
	struct hash<std::pair<A, B>> {
        std::size_t operator()(const std::pair<A, B>& v) const noexcept
        {
			return Halley::combineHash(std::hash<A>()(v.first), std::hash<B>()(v.second));
        }
    };

	template<typename A, typename B, typename C>
	struct hash<std::tuple<A, B, C>> {
        std::size_t operator()(const std::tuple<A, B, C>& v) const noexcept
        {
			return Halley::combineHash(Halley::combineHash(std::hash<A>()(std::get<0>(v)), std::hash<B>()(std::get<1>(v))), std::hash<C>()(std::get<2>(v)));
        }
    };
}
