#pragma once

#include <memory>
#include <halley/maths/colour.h>
#include <halley/maths/vector2.h>
#include "halley/maths/rect.h"
#include "halley/data_structures/maybe.h"
#include <gsl/span>
#include <map>
#include "halley/graphics/sprite/sprite.h"
#include "halley/graphics/text/font.h"

namespace Halley
{
	class LocalisedString;
	class Painter;
	class Material;

	using ColourOverride = std::pair<size_t, std::optional<Colour4f>>;
	using FontOverride = std::pair<size_t, std::shared_ptr<const Font>>;
	using FontSizeOverride = std::pair<size_t, std::optional<float>>;

	template <typename T, typename C = std::optional<T>>
	class TextOverrideCursor {
	public:
		TextOverrideCursor(T defaultValue, const Vector<std::pair<size_t, C>>& data)
			: curValue(defaultValue)
			, defaultValue(std::move(defaultValue))
			, dataPtr(&data)
		{}

		const T& operator*() const
		{
			return curValue;
		}

		const T& operator->() const
		{
			return curValue;
		}

		const T& getCurValue() const
		{
			return curValue;
		}

		void setPos(size_t i)
		{
			const auto& data = *dataPtr;
			while (curIdx < data.size() && data[curIdx].first <= i) {
				if constexpr (std::is_same_v<T, C>) {
					curValue = data[curIdx].second ? data[curIdx].second : defaultValue;
				} else {
					curValue = data[curIdx].second ? *(data[curIdx].second) : defaultValue;
				}
				++curIdx;
			}
		}

	private:
		size_t curIdx = 0;
		T curValue;
		T defaultValue;
		const Vector<std::pair<size_t, C>>* dataPtr = nullptr;
	};

	class TextRenderer
	{
	public:
		using SpriteFilter = std::function<void(gsl::span<Sprite>)>;

		TextRenderer();
		explicit TextRenderer(std::shared_ptr<const Font> font, const String& text = "", float size = 20, Colour colour = Colour(1, 1, 1, 1), float outline = 0, Colour outlineColour = Colour(0, 0, 0, 1));

		TextRenderer& setPosition(Vector2f pos);
		TextRenderer& setFont(std::shared_ptr<const Font> font);
		TextRenderer& setText(const String& text);
		TextRenderer& setText(const StringUTF32& text);
		TextRenderer& setText(const LocalisedString& text);
		TextRenderer& setSize(float size);
		TextRenderer& setColour(Colour colour);
		TextRenderer& setOutlineColour(Colour colour);
		TextRenderer& setOutline(float width);
		TextRenderer& setOutline(float width, Colour colour);
		TextRenderer& setShadow(float distance, float smoothness, Colour colour);
		TextRenderer& setShadow(Vector2f distance, float smoothness, Colour colour);
		TextRenderer& setShadowColour(Colour colour);
		TextRenderer& setAlignment(float align);
		TextRenderer& setOffset(Vector2f align);
		TextRenderer& setClip(Rect4f clip);
		TextRenderer& setClip();
		TextRenderer& setSmoothness(float smoothness);
		TextRenderer& setPixelOffset(Vector2f offset);
		TextRenderer& setColourOverride(Vector<ColourOverride> colOverride);
		TextRenderer& setFontOverride(Vector<FontOverride> fontOverride);
		TextRenderer& setFontSizeOverride(Vector<FontSizeOverride> fontSizeOverride);
		TextRenderer& setLineSpacing(float spacing);
		TextRenderer& setAlpha(float alpha);
		TextRenderer& setScale(float scale);
		TextRenderer& setAngle(Angle1f angle);

		void refresh();

		TextRenderer clone() const;

		void generateSprites() const;
		void draw(Painter& painter, const std::optional<Rect4f>& extClip = {}) const;

		void setSpriteFilter(SpriteFilter f);

		Vector2f getExtents() const;
		Vector2f getExtents(const StringUTF32& str) const;
		Vector2f getCharacterPosition(size_t character) const;
		size_t getCharacterAt(const Vector2f& position) const;

		StringUTF32 split(const String& str, float width) const;
		StringUTF32 split(const StringUTF32& str, float width, std::function<bool(int32_t)> filter = {}, const Vector<FontOverride>& fontOverrides = {}, const Vector<FontSizeOverride>& fontSizeOverrides = {}) const;
		StringUTF32 split(float width) const;

		Vector2f getPosition() const;
		String getText() const;
		const StringUTF32& getTextUTF32() const;
		Colour getColour() const;
		float getOutline() const;
		Colour getOutlineColour() const;
		Colour getShadowColour() const;
		float getSmoothness() const;
		std::optional<Rect4f> getClip() const;
		float getLineHeight() const;
		float getLineHeight(const Font& font, float size) const;
		float getAlignment() const;

		bool empty() const;
		Rect4f getAABB() const;

		bool isCompatibleWith(const TextRenderer& other) const; // Can be drawn as part of the same draw call

	private:
		struct GlyphLayout {
			Vector2f pos;
			Vector2f penPos;
			float lineStartY;
			float lineEndY;
			float advanceX;
			float ascender;
		};

		std::shared_ptr<const Font> font;
		mutable HashMap<const Font*, std::shared_ptr<Material>> materials;
		StringUTF32 text;
		SpriteFilter spriteFilter;
		
		float size = 20;
		float outline = 0;
		float align = 0;
		float smoothness = 1.0f;
		float lineSpacing = 0.0f;
		Vector2f shadowDistance;
		float shadowSmoothness = 1.0f;
		float scale = 1.0f;
		Angle1f angle;

		Vector2f position;
		Vector2f offset;
		Vector2f pixelOffset;
		Colour colour;
		Colour outlineColour;
		Colour shadowColour = Colour(0, 0, 0, 0);
		std::optional<Rect4f> clip;

		Vector<ColourOverride> colourOverrides;
		Vector<FontOverride> fontOverrides;
		Vector<FontSizeOverride> fontSizeOverrides;

		mutable Vector<GlyphLayout> layoutCache;
		mutable Vector<Sprite> spritesCache;
		mutable bool materialDirty = true;
		mutable bool glyphsDirty = true;
		mutable bool layoutDirty = true;
		mutable bool positionDirty = true;
		mutable bool hasExtents = false;

		mutable Vector2f extents;

		const std::shared_ptr<Material>& getMaterial(const Font& font) const;
		void updateMaterial(Material& material, const Font& font) const;
		void updateMaterialForFont(const Font& font) const;
		void updateMaterials() const;
		float getScale(const Font& font) const;
		float getScale(const Font& font, float size) const;

		void markLayoutDirty() const;
		void markSpritesDirty() const;

		void generateLayoutIfNeeded() const;
		void generateGlyphsIfNeeded() const;
		void generateLayout(const StringUTF32& text, Vector<GlyphLayout>* layouts, Vector2f& extents) const;
		void generateSprites(Vector<Sprite>& sprites, const Vector<GlyphLayout>& layouts) const;
		static size_t getGlyphCount(const StringUTF32& text);
	};

	class ColourStringBuilder {
	public:
		explicit ColourStringBuilder(bool replaceEmptyWithQuotes = false);
		void append(std::string_view text, std::optional<Colour4f> col = {});

		std::pair<String, Vector<ColourOverride>> moveResults();

	private:
		bool replaceEmptyWithQuotes;
		Vector<String> strings;
		Vector<ColourOverride> colours;
		size_t len = 0;
	};

	class Resources;
	template<>
	class ConfigNodeSerializer<TextRenderer> {
	public:
		ConfigNode serialize(const TextRenderer& text, const EntitySerializationContext& context);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, TextRenderer& target);
	};
}
