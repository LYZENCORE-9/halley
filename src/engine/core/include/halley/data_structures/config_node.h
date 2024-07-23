#pragma once

#include <gsl/span>

#include "halley/text/halleystring.h"
#include "halley/maths/range.h"
#include "halley/maths/vector4.h"
#include "halley/data_structures/maybe.h"
#include <map>
#include "halley/data_structures/vector.h"

#include "hash_map.h"
#include "halley/entity/entity_id.h"
#include "halley/maths/ops.h"
#include "halley/maths/rect.h"
#include "halley/utils/type_traits.h"

#if defined(DEV_BUILD)
#define STORE_CONFIG_NODE_PARENTING
#endif

namespace Halley {
	class Serializer;
	class Deserializer;
	class ConfigNode;

	namespace Hash {
		class Hasher;
	}

	template<typename T>
	struct HasToConfigNode
	{
	private:
		typedef std::true_type yes;
		typedef std::false_type no;
		template<typename U> static auto test(int) -> decltype(std::declval<U>().toConfigNode(), yes());
		template<typename> static no test(...);
	 
	public:
		static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
	};

	template<typename T>
	struct HasConfigNodeConstructor
	{
	private:
		typedef std::true_type yes;
		typedef std::false_type no;
		template<typename U> static auto test(int) -> decltype(U(std::declval<ConfigNode>()), yes());
		template<typename> static no test(...);
	 
	public:
		static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
	};

	enum class ConfigNodeType
	{
		Undefined,
		String,
		Sequence,
		Map,
		Int,
		Float,
		Int2,
		Float2,
		Bytes,
		DeltaSequence, // For delta coding
		DeltaMap, // For delta coding
		Noop, // For delta coding
		Idx, // For delta coding
		Del, // For delta coding
		Int64,
		EntityId,
		Bool
	};

	template <>
	struct EnumNames<ConfigNodeType> {
		constexpr std::array<const char*, 17> operator()() const {
			return{{
				"undefined",
				"string",
				"sequence",
				"map",
				"int",
				"float",
				"int2",
				"float2",
				"bytes",
				"deltaSequence",
				"deltaMap",
				"noop",
				"idx",
				"del",
				"int64",
				"entityId",
				"bool"
			}};
		}
	};

	class ConfigFile;
	
	class ConfigNode
	{
		friend class ConfigFile;

	public:
		template <typename T>
		class Tag {};

		using MapType = HashMap<String, ConfigNode>;
		using SequenceType = Vector<ConfigNode>;

		struct NoopType {};
		struct DelType {};
		struct IdxType {
			int start;
			int len;
			IdxType() = default;
			IdxType(int start, int len) : start(start), len(len) {}
		};
		
		ConfigNode();
		explicit ConfigNode(const ConfigNode& other);
		ConfigNode(ConfigNode&& other) noexcept;
		ConfigNode(MapType entryMap);
		ConfigNode(SequenceType entryList);
		explicit ConfigNode(String value);
		explicit ConfigNode(const std::string_view& value);
		explicit ConfigNode(const char* value);
		explicit ConfigNode(bool value);
		explicit ConfigNode(int value);
		explicit ConfigNode(uint32_t value);
		explicit ConfigNode(int64_t value);
		explicit ConfigNode(EntityId value);
		explicit ConfigNode(float value);
		explicit ConfigNode(Vector2i value);
		explicit ConfigNode(Vector2f value);
		explicit ConfigNode(Vector3i value);
		explicit ConfigNode(Vector3f value);
		explicit ConfigNode(Vector4i value);
		explicit ConfigNode(Vector4f value);
		explicit ConfigNode(Rect4i value);
		explicit ConfigNode(Rect4f value);
		explicit ConfigNode(Bytes value);
		explicit ConfigNode(NoopType value);
		explicit ConfigNode(DelType value);
		explicit ConfigNode(IdxType value);
		explicit ConfigNode(Range<float> value);
		explicit ConfigNode(Range<int> value);

		template <typename T>
		explicit ConfigNode(const Vector<T>& sequence)
		{
			*this = sequence;
		}
		
		template <typename T>
		explicit ConfigNode(const std::optional<T>& opt)
		{
			*this = opt;
		}

		template <typename K, typename V>
		explicit ConfigNode(const std::map<K, V>& values)
		{
			*this = values;
		}

		template <typename T, typename U>
		explicit ConfigNode(const std::pair<T, U>& values)
		{
			*this = values;
		}

		template <typename T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
		explicit ConfigNode(T enumValue)
		{
			*this = toString(enumValue);
		}

		~ConfigNode();
		
		ConfigNode& operator=(const ConfigNode& other);
		ConfigNode& operator=(ConfigNode&& other) noexcept;
		ConfigNode& operator=(bool value);
		ConfigNode& operator=(int value);
		ConfigNode& operator=(uint32_t value);
		ConfigNode& operator=(int64_t value);
		ConfigNode& operator=(EntityId value);
		ConfigNode& operator=(float value);
		ConfigNode& operator=(Vector2i value);
		ConfigNode& operator=(Vector2f value);
		ConfigNode& operator=(Vector3i value);
		ConfigNode& operator=(Vector3f value);
		ConfigNode& operator=(Vector4i value);
		ConfigNode& operator=(Vector4f value);
		ConfigNode& operator=(Rect4i value);
		ConfigNode& operator=(Rect4f value);
		ConfigNode& operator=(Range<float> value);
		ConfigNode& operator=(Range<int> value);

		ConfigNode& operator=(MapType entryMap);
		ConfigNode& operator=(SequenceType entryList);
		ConfigNode& operator=(String value);
		ConfigNode& operator=(Bytes value);
		ConfigNode& operator=(gsl::span<const gsl::byte> bytes);

		ConfigNode& operator=(const char* value);
		ConfigNode& operator=(const std::string_view& value);
		
		ConfigNode& operator=(NoopType value);
		ConfigNode& operator=(DelType value);
		ConfigNode& operator=(IdxType value);

		template <typename T>
		ConfigNode& operator=(const Vector<T>& sequence)
		{
			SequenceType seq;
			seq.reserve(sequence.size());
			for (const auto& e: sequence) {
				if constexpr (HasToConfigNode<T>::value) {
					seq.push_back(e.toConfigNode());
				} else {
					seq.push_back(ConfigNode(e));
				}
			}
			return *this = std::move(seq);
		}

		template <typename K, typename V>
		ConfigNode& operator=(const std::map<K, V>& values)
		{
			MapType map;
			for (const auto& [k, v]: values) {
				String key = toString(k);

				if constexpr (HasToConfigNode<V>::value) {
					map[std::move(key)] = v.toConfigNode();
				} else {
					map[std::move(key)] = ConfigNode(v);
				}
			}
			return *this = std::move(map);
		}

		template <typename K, typename V>
		ConfigNode& operator=(const HashMap<K, V>& values)
		{
			MapType map;
			for (const auto& [k, v]: values) {
				String key = toString(k);

				if constexpr (HasToConfigNode<V>::value) {
					map[std::move(key)] = v.toConfigNode();
				} else {
					map[std::move(key)] = ConfigNode(v);
				}
			}
			return *this = std::move(map);
		}

		template <typename T>
		ConfigNode& operator=(const HashSet<T>& values)
		{
			SequenceType seq;
			for (const auto& v: values) {
				if constexpr (HasToConfigNode<T>::value) {
					seq.push_back(v.toConfigNode());
				} else {
					seq.push_back(ConfigNode(v));
				}
			}
			return *this = std::move(seq);
		}

		template <typename T, typename U>
		ConfigNode& operator=(const std::pair<T, U>& value)
		{
			MapType map;
			map["first"] = value.first;
			map["second"] = value.second;
			return *this = std::move(map);
		}

		template <typename T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
		ConfigNode& operator=(T enumValue)
		{
			return *this = toString(enumValue);
		}

		template <typename T, std::enable_if_t<HasToConfigNode<T>::value, bool> = true>
		ConfigNode& operator=(const T& value)
		{
			return *this = value.toConfigNode();
		}

		template <typename T>
		ConfigNode& operator=(const std::optional<T>& value)
		{
			if (value) {
				*this = *value;
			} else {
				reset();
			}
			return *this;
		}

		bool operator==(const ConfigNode& other) const;
		bool operator!=(const ConfigNode& other) const;
		bool operator>(const ConfigNode& other) const;
		bool operator<(const ConfigNode& other) const;
		bool operator>=(const ConfigNode& other) const;
		bool operator<=(const ConfigNode& other) const;
		bool compareTo(MathRelOp op, const ConfigNode& other) const;

		ConfigNodeType getType() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		int asInt() const;
		int64_t asInt64() const;
		EntityId asEntityId() const;
		float asFloat() const;
		bool asBool() const;
		Vector2i asVector2i() const;
		Vector2f asVector2f() const;
		Vector3i asVector3i() const;
		Vector3f asVector3f() const;
		Vector4i asVector4i() const;
		Vector4f asVector4f() const;
		Rect4i asRect4i() const;
		Rect4f asRect4f() const;
		Range<int> asIntRange() const;
		Range<float> asFloatRange() const;
		String asString() const;
		std::string_view asStringView() const;
		const Bytes& asBytes() const;

		int asInt(int defaultValue) const;
		int64_t asInt64(int64_t defaultValue) const;
		EntityId asEntityId(EntityId defaultValue) const;
		float asFloat(float defaultValue) const;
		bool asBool(bool defaultValue) const;
		String asString(const std::string_view& defaultValue) const;
		std::string_view asStringView(const std::string_view& defaultValue) const;
		Vector2i asVector2i(Vector2i defaultValue) const;
		Vector2f asVector2f(Vector2f defaultValue) const;
		Vector3i asVector3i(Vector3i defaultValue) const;
		Vector3f asVector3f(Vector3f defaultValue) const;
		Vector4i asVector4i(Vector4i defaultValue) const;
		Vector4f asVector4f(Vector4f defaultValue) const;
		Rect4i asRect4i(Rect4i defaultValue) const;
		Rect4f asRect4f(Rect4f defaultValue) const;
		Range<float> asFloatRange(Range<float> defaultValue) const;
		Range<int> asIntRange(Range<int> defaultValue) const;

		template <typename T>
		Vector<T> asVector() const
		{
			if (type == ConfigNodeType::Sequence) {
				Vector<T> result;
				result.reserve(asSequence().size());
				for (const auto& e : asSequence()) {
					if constexpr (HasConfigNodeConstructor<T>::value) {
						result.emplace_back(T(e));
					} else if constexpr (std::is_enum_v<T>) {
						result.emplace_back(fromString<T>(e.asString()));
					} else {
						result.emplace_back(e.convertTo(Tag<T>()));
					}
				}
				return result;
			} else if (type == ConfigNodeType::Undefined) {
				return {};
			} else {
				throw Exception("Can't convert " + getNodeDebugId() + " from " + toString(getType()) + " to Vector<T>.", HalleyExceptions::Resources);
			}
		}

		template <typename T>
		Vector<T> asVector(const Vector<T>& defaultValue) const
		{
			if (type == ConfigNodeType::Sequence) {
				return asVector<T>();
			} else {
				return defaultValue;
			}
		}

		template <typename K, typename V>
		std::map<K, V> asMap() const
		{
			if (type == ConfigNodeType::Map) {
				std::map<K, V> result;
				for (const auto& [k, v] : asMap()) {
					const K key = fromString<K>(k);
					
					if constexpr (HasConfigNodeConstructor<V>::value) {
						result[std::move(key)] = V(v);
					} else {
						result[std::move(key)] = v.convertTo(Tag<V>());
					}
				}
				return result;
			} else if (type == ConfigNodeType::Undefined) {
				return {};
			} else {
				throw Exception("Can't convert " + getNodeDebugId() + " from " + toString(getType()) + " to std::map<K, V>.", HalleyExceptions::Resources);
			}
		}

		template <typename K, typename V>
		HashMap<K, V> asHashMap() const
		{
			if (type == ConfigNodeType::Map) {
				HashMap<K, V> result;
				for (const auto& [k, v] : asMap()) {
					const K key = fromString<K>(k);
					
					if constexpr (HasConfigNodeConstructor<V>::value) {
						result[std::move(key)] = V(v);
					} else {
						result[std::move(key)] = v.convertTo(Tag<V>());
					}
				}
				return result;
			} else if (type == ConfigNodeType::Undefined) {
				return {};
			} else {
				throw Exception("Can't convert " + getNodeDebugId() + " from " + toString(getType()) + " to HashMap<K, V>.", HalleyExceptions::Resources);
			}
		}

		template <typename T>
		HashSet<T> asHashSet() const
		{
			if (type == ConfigNodeType::Sequence) {
				HashSet<T> result;
				for (const auto& v : asSequence()) {
					if constexpr (HasConfigNodeConstructor<T>::value) {
						result.insert(T(v));
					} else {
						result.insert(v.convertTo(Tag<T>()));
					}
				}
				return result;
			} else if (type == ConfigNodeType::Undefined) {
				return {};
			} else {
				throw Exception("Can't convert " + getNodeDebugId() + " from " + toString(getType()) + " to HashSet<T>.", HalleyExceptions::Resources);
			}
		}

		template <typename T>
		T asType() const
		{
			if constexpr (std::is_enum_v<T>) {
				return asEnum<T>();
			} else if constexpr (HasConfigNodeConstructor<T>::value) {
				return T(*this);
			} else {
				return convertTo(Tag<T>());
			}
		}

		template <typename T>
		T asType(T defaultValue) const
		{
			if (getType() == ConfigNodeType::Undefined) {
				return defaultValue;
			}
			return asType<T>();
		}

		template <typename T>
		T asEnum() const
		{
			if (auto v = tryFromString<T>(asString())) {
				return *v;
			} else {
				Logger::logError("Unknown enum value \"" + asString() + "\" in type " + typeid(T).name());
				return T();
			}
		}

		template <typename T>
		T asEnum(T defaultValue) const
		{
			if (getType() == ConfigNodeType::Undefined) {
				return defaultValue;
			}
			if (auto v = tryFromString<T>(asString())) {
				return *v;
			} else {
				Logger::logError("Unknown enum value \"" + asString() + "\" in type " + typeid(T).name());
				return defaultValue;
			}
		}

		template <typename T>
		std::optional<T> asOptional() const
		{
			if (getType() == ConfigNodeType::Undefined) {
				return {};
			} else {
				return asType<T>();
			}
		}
		
		const SequenceType& asSequence() const;
		const MapType& asMap() const;
		SequenceType& asSequence();
		MapType& asMap();
		size_t getSequenceSize(size_t defaultValue = 0) const;

		void ensureType(ConfigNodeType type);

		bool hasKey(std::string_view key) const;
		bool removeKey(std::string_view key, bool unsetMapIfEmpty = false);

		ConfigNode& operator[](std::string_view key);
		ConfigNode& operator[](size_t idx);

		const ConfigNode& operator[](std::string_view key) const;
		const ConfigNode& operator[](size_t idx) const;

		ConfigNode& at(std::string_view key);
		const ConfigNode& at(std::string_view key) const;

		SequenceType::iterator begin();
		SequenceType::iterator end();

		SequenceType::const_iterator begin() const;
		SequenceType::const_iterator end() const;

		void reset();
		std::pair<int, int> getOriginalPosition() const;
		void setOriginalPosition(int line, int column);
		void setParent(const ConfigNode* parent, int idx);
		void propagateParentingInformation(const ConfigFile* parentFile);

		inline void assertValid() const
		{
			Expects(intData != 0xCDCDCDCD);
			Expects(intData != 0xDDDDDDDD);
		}

		size_t getSizeBytes() const;

		struct BreadCrumb {
			const BreadCrumb* prev = nullptr;
			String key;
			OptionalLite<int> idx;
			int depth = 0;

			BreadCrumb() = default;
			BreadCrumb(const BreadCrumb& prev, String key) : prev(&prev), key(std::move(key)), depth(prev.depth + 1) {}
			BreadCrumb(const BreadCrumb& prev, int index) : prev(&prev), idx(index), depth(prev.depth + 1) {}

			bool hasKeyAt(const String& key, int depth) const;
			bool hasIndexAt(int idx, int depth) const;
		};

		class IDeltaCodeHints {
		public:
			virtual ~IDeltaCodeHints() = default;

			virtual std::optional<size_t> getSequenceMatch(const SequenceType& seq, const ConfigNode& newValue, size_t curIdx, const BreadCrumb& breadCrumb) const = 0;
			virtual bool doesSequenceOrderMatter(const BreadCrumb& breadCrumb) const { return true; }
			virtual bool canDeleteKey(const String& key, const BreadCrumb& breadCrumb) const { return true; }
			virtual bool canDeleteAnyKey() const { return true; }
			virtual bool shouldBypass(const BreadCrumb& breadCrumb) const { return false; }
			virtual bool areNullAndEmptyEquivalent(const BreadCrumb& breadCrumb) const { return false; }
		};

		static ConfigNode createDelta(const ConfigNode& from, const ConfigNode& to, const IDeltaCodeHints* hints = nullptr);
		static ConfigNode applyDelta(const ConfigNode& from, const ConfigNode& delta);
		void applyDelta(const ConfigNode& delta);
		void decayDeltaArtifacts();
		
		static ConfigNodeType getPromotedType(gsl::span<const ConfigNodeType> types, bool promoteUndefined);
		static bool isScalarType(ConfigNodeType type, bool acceptUndefined);
		static bool isVector2Type(ConfigNodeType type, bool acceptUndefined);

		void feedToHash(Hash::Hasher& hasher) const;

		static const ConfigNode& getUndefined();

	private:
		union {
			String* strData;
			MapType* mapData;
			SequenceType* sequenceData;
			Bytes* bytesData;
			void* rawPtrData;
			int intData;
			float floatData;
			Vector2i vec2iData;
			Vector2f vec2fData;
			int64_t int64Data;
		};
		ConfigNodeType type = ConfigNodeType::Undefined;
		int auxData = 0; // Used by delta coding

#if defined(STORE_CONFIG_NODE_PARENTING)
		struct ParentingInfo {
			int line = 0;
			int column = 0;
			int idx = 0;
			const ConfigNode* node = nullptr;
			const ConfigFile* file = nullptr;
		};
		std::unique_ptr<ParentingInfo> parent;
#endif

		thread_local static ConfigNode undefinedConfigNode;
		thread_local static String undefinedConfigNodeName;

		template <typename T> void deserializeContents(Deserializer& s)
		{
			T v;
			s >> v;
			*this = std::move(v);
		}

		String getNodeDebugId() const;
		String backTrackFullNodeName() const;

		int convertTo(Tag<int> tag) const;
		int64_t convertTo(Tag<int64_t> tag) const;
		EntityId convertTo(Tag<EntityId> tag) const;
		float convertTo(Tag<float> tag) const;
		bool convertTo(Tag<bool> tag) const;
		uint8_t convertTo(Tag<uint8_t> tag) const;
		uint16_t convertTo(Tag<uint16_t> tag) const;
		uint32_t convertTo(Tag<uint32_t> tag) const;
		uint64_t convertTo(Tag<uint64_t> tag) const;
		Vector2i convertTo(Tag<Vector2i> tag) const;
		Vector2f convertTo(Tag<Vector2f> tag) const;
		Vector3i convertTo(Tag<Vector3i> tag) const;
		Vector3f convertTo(Tag<Vector3f> tag) const;
		Vector4i convertTo(Tag<Vector4i> tag) const;
		Vector4f convertTo(Tag<Vector4f> tag) const;
		Range<float> convertTo(Tag<Range<float>> tag) const;
		String convertTo(Tag<String> tag) const;
		const Bytes& convertTo(Tag<Bytes&> tag) const;

		template <typename T>
		Vector<T> convertTo(Tag<Vector<T>> tag) const
		{
			return asVector<T>();
		}

		template <typename T>
		HashMap<String, T> convertTo(Tag<HashMap<String, T>> tag) const
		{
			return asHashMap<String, T>();
		}

		bool isNullOrEmpty() const;

		static ConfigNode doCreateDelta(const ConfigNode& from, const ConfigNode& to, const BreadCrumb& breadCrumb, const IDeltaCodeHints* hints);
		static ConfigNode createMapDelta(const ConfigNode& from, const ConfigNode& to, const BreadCrumb& breadCrumb, const IDeltaCodeHints* hints);
		static ConfigNode createSequenceDelta(const ConfigNode& from, const ConfigNode& to, const BreadCrumb& breadCrumb, const IDeltaCodeHints* hints);
		void applyMapDelta(const ConfigNode& delta);
		void applySequenceDelta(const ConfigNode& delta);
	};
}
