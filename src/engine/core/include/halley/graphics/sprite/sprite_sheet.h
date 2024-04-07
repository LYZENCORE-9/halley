#pragma once

#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <memory>
#include <halley/resources/resource.h>
#include <halley/text/halleystring.h>
#include <halley/data_structures/hash_map.h>
#include <gsl/span>

#include "halley/file_formats/image.h"
#include "halley/maths/vector4.h"

namespace Halley
{
	class BinPackResult;
	class Sprite;
	class Resources;
	class Serializer;
	class Deserializer;
	class ResourceDataStatic;
	class Texture;
	class ResourceLoader;
	class Material;
	class SpriteSheet;

	class SpriteSheetEntry
	{
	public:
		Vector2f pivot;
		Vector2i origPivot;
		Vector2f size;
		Rect4f coords;
		Vector4s trimBorder;
		Vector4s slices;
		bool rotated = false;
		bool sliced = false;

		SpriteSheetEntry() = default;
		SpriteSheetEntry(const ConfigNode& node);
		ConfigNode toConfigNode() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

#ifdef ENABLE_HOT_RELOAD
		SpriteSheet* parent = nullptr;
		uint32_t idx = 0;
		String name;
#endif
	};

	class SpriteSheetFrameTag
	{
	public:
		String name;
		int from = 0;
		int to = 0;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class SpriteHotReloader {
	public:
		virtual ~SpriteHotReloader() = default;

		virtual const String& getHotReloaderId() const = 0;

#ifdef ENABLE_HOT_RELOAD
		void addSprite(Sprite* sprite, uint32_t idx) const;
		void removeSprite(Sprite* sprite) const;
		void updateSpriteIndex(Sprite* sprite, uint32_t idx) const;
		void clearSpriteRefs();

	protected:
		class SpritePointerHasher {
		public:
			std::size_t operator()(Sprite* ptr) const noexcept;
		};
		
		mutable std::mutex spriteMutex;
		mutable HashMap<Sprite*, uint32_t, SpritePointerHasher> spriteRefs;
#endif
	};
	
	class SpriteSheet final : public Resource, public SpriteHotReloader
	{
	public:
		struct ImageData
		{
			int frameNumber = 0;
			int origFrameNumber = 0;
			int duration = 0;
			String sequenceName;
			String direction;
			Rect4i clip;
			Vector2i pivot;
			Vector4s slices;

			std::unique_ptr<Image> img;
			Vector<String> filenames;
			String origFilename;
			String group;

			bool operator==(const ImageData& other) const;
			bool operator!=(const ImageData& other) const;

		private:
			friend class SpriteSheet;
			
			bool isDuplicate = false;
			Vector<ImageData*> duplicatesOfThis;
		};

		SpriteSheet();
		~SpriteSheet() override;

		void load(const ConfigNode& node);
		ConfigNode toConfigNode() const;
		
		const std::shared_ptr<const Texture>& getTexture() const;
		const std::shared_ptr<const Texture>& getPaletteTexture() const;
		const SpriteSheetEntry& getSprite(std::string_view name) const;
		const SpriteSheetEntry& getSprite(size_t idx) const;
		const SpriteSheetEntry* tryGetSprite(std::string_view name) const;
		const SpriteSheetEntry& getDummySprite() const;

		const Vector<SpriteSheetFrameTag>& getFrameTags() const;
		Vector<String> getSpriteNames() const;
		const HashMap<String, uint32_t>& getSpriteNameMap() const;

		size_t getSpriteCount() const;
		std::optional<size_t> getIndex(std::string_view name) const;
		bool hasSprite(std::string_view name) const;

		const SpriteSheetEntry* getSpriteAtTexel(Vector2i pos) const;

		void addSprite(String name, const SpriteSheetEntry& sprite);
		void setTextureName(String name);

		std::shared_ptr<Material> getMaterial(std::string_view name) const;
		void clearMaterialCache() const;

		void setDefaultMaterialName(String materialName);
		const String& getDefaultMaterialName() const;
		void setPaletteName(String paletteName);
		const String& getPaletteName() const;

		std::unique_ptr<Image> generateAtlas(Vector<ImageData>& images, ConfigNode& spriteInfo, bool powerOfTwo);

		static std::unique_ptr<SpriteSheet> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::SpriteSheet; }
		void reload(Resource&& resource) override;

		const String& getHotReloaderId() const override;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void onOtherResourcesUnloaded() override;

	protected:
		ResourceMemoryUsage getMemoryUsage() const override;

	private:
		constexpr static int version = 2;
		
		Resources* resources = nullptr;

		Vector<SpriteSheetEntry> sprites;
		SpriteSheetEntry dummySprite;
		HashMap<String, uint32_t> spriteIdx;
		Vector<SpriteSheetFrameTag> frameTags;

		String textureName;
		String paletteName;
		mutable std::shared_ptr<const Texture> texture;
		mutable std::shared_ptr<const Texture> paletteTexture;

		String defaultMaterialName;
		mutable HashMap<String, std::weak_ptr<Material>> materials;

		void loadTexture(Resources& resources) const;
		void loadPaletteTexture(Resources& resources) const;
		void assignIds();

		std::unique_ptr<Image> makeAtlas(const Vector<BinPackResult>& result, ConfigNode& spriteInfo, bool powerOfTwo);
		Vector2i computeAtlasSize(const Vector<BinPackResult>& results, bool powerOfTwo) const;
		void markDuplicates(Vector<ImageData>& images) const;
	};

	class SpriteResource final : public Resource, public SpriteHotReloader
	{
	public:
		SpriteResource();
		SpriteResource(const std::shared_ptr<const SpriteSheet>& spriteSheet, size_t idx);
		~SpriteResource();

		const SpriteSheetEntry& getSprite() const;
		size_t getIdx() const;
		std::shared_ptr<const SpriteSheet> getSpriteSheet() const;

		std::shared_ptr<Material> getMaterial(std::string_view name) const;
		const String& getDefaultMaterialName() const;

		constexpr static AssetType getAssetType() { return AssetType::Sprite; }
		static std::unique_ptr<SpriteResource> loadResource(ResourceLoader& loader);
		void reload(Resource&& resource) override;

		const String& getHotReloaderId() const override;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	protected:
		ResourceMemoryUsage getMemoryUsage() const override;

	private:
		std::weak_ptr<const SpriteSheet> spriteSheet;
		uint64_t idx = -1;
		Resources* resources = nullptr;
	};
}
