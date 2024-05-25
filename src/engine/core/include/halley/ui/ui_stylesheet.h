#pragma once

#include "halley/graphics/sprite/sprite.h"
#include "halley/graphics/text/text_renderer.h"
#include <map>

namespace Halley {
	class UIStyleSheet;
	class UIColourScheme;
	class ConfigFile;
	class ConfigNode;
	class ConfigObserver;
	class AudioClip;
	class UISTyle;

	class UIStyleDefinition
	{
	public:
		UIStyleDefinition(String styleName, const ConfigNode& node, UIStyleSheet& styleSheet);
		~UIStyleDefinition();

		const String& getName() const;
		const Sprite& getSprite(const String& name) const;
		const TextRenderer& getTextRenderer(const String& name) const;
		Vector4f getBorder(const String& name) const;
		Vector4f getBorder(const String& name, Vector4f defaultValue) const;
		const String& getString(const String& name) const;
		float getFloat(const String& name) const;
		float getFloat(const String& name, float defaultValue) const;
		Time getTime(const String& name) const;
		Time getTime(const String& name, Time defaultValue) const;
		Vector2f getVector2f(const String& name) const;
		Vector2f getVector2f(const String& name, Vector2f defaultValue) const;
		Colour4f getColour(const String& name) const;
		std::shared_ptr<const UIStyleDefinition> getSubStyle(const String& name) const;

		bool hasTextRenderer(const String& name) const;
		bool hasColour(const String& name) const;
		bool hasSubStyle(const String& name) const;
		bool hasSprite(const String& name) const;
		bool hasVector2f(const String& name) const;
		bool hasFloat(const String& name) const;

		void reload(const ConfigNode& node);

		gsl::span<const String> getClasses() const;
		bool hasClass(const String& className) const;

	private:
		class Pimpl;
		
		const String styleName;
		const ConfigNode* node = nullptr;
		UIStyleSheet& styleSheet;

		std::unique_ptr<Pimpl> pimpl;

		Vector<String> classes;

		void loadDefaults();
	};

	class UIStyleSheet {
		friend class UIStyle;

	public:
		UIStyleSheet(Resources& resources);
		UIStyleSheet(Resources& resources, std::shared_ptr<const UIColourScheme> colourScheme);
		UIStyleSheet(Resources& resources, const ConfigFile& file, std::shared_ptr<const UIColourScheme> colourScheme = {});

		void load(const ConfigFile& file, std::shared_ptr<const UIColourScheme> colourScheme = {});

		bool updateIfNeeded();
		void reload(std::shared_ptr<const UIColourScheme> colourScheme);

		Resources& getResources() const { return resources; }
		const std::shared_ptr<const UIColourScheme>& getColourScheme() const { return lastColourScheme; }
		std::shared_ptr<const UIStyleDefinition> getStyle(const String& styleName) const;
		std::shared_ptr<UIStyleDefinition> getStyle(const String& styleName);

		Vector<String> getStylesForClass(const String& className) const;
		void getStylesForClass(Vector<String>& dst, const String& className) const;

		bool hasStyleObserver(const String& styleName) const;
		const ConfigObserver& getStyleObserver(const String& styleName) const;

		void applyStyles(const UIStyleSheet& other);

		float getUIScale() const;
		void setUIScale(float scale);

		const TextRenderer& getDefaultTextRenderer() const;

	private:
		Resources& resources;
		HashMap<String, std::shared_ptr<UIStyleDefinition>> styles;
		std::map<String, ConfigObserver> observers;
		HashMap<String, String> styleToObserver;
		std::shared_ptr<const UIColourScheme> lastColourScheme = nullptr;
		float uiScale = 1;

		ConfigNode defaultData;
		std::shared_ptr<UIStyleDefinition> defaultStyle;
		TextRenderer defaultTextRenderer;

		void load(const ConfigNode& node, const String& assetId, std::shared_ptr<const UIColourScheme> colourScheme);

		bool needsUpdate() const;
		void update();
		void setupDefaultStyle();
	};
}
