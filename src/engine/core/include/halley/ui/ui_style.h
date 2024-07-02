#pragma once

#include "ui_stylesheet.h"

namespace Halley {
	class UIStyle {
	public:
		UIStyle();
		UIStyle(std::shared_ptr<const UIStyleDefinition>);
		UIStyle(const String& name, std::shared_ptr<UIStyleSheet> styleSheet);

		bool hasSubStyle(const String& name) const;
		UIStyle getSubStyle(const String& name) const;

		const String& getName() const;
		const Sprite& getSprite(const String& name) const;
		bool hasSprite(const String& name) const;
		const TextRenderer& getTextRenderer(const String& name) const;
		bool hasTextRenderer(const String& name) const;
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
		bool hasColour(const String& name) const;
		bool hasFloat(const String& name) const;
		bool hasString(const String& name) const;
		bool hasVector2f(const String& name) const;

	private:
		std::shared_ptr<const UIStyleDefinition> style;
	};
}
