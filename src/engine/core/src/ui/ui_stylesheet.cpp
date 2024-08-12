#include "halley/ui/ui_stylesheet.h"

#include "halley/ui/ui_colour_scheme.h"
#include "halley/file_formats/config_file.h"
#include "halley/audio/audio_clip.h"
#include "halley/graphics/text/font.h"
#include "halley/resources/resources.h"
#include "halley/support/logger.h"
#include "halley/maths/vector4.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

Colour4f getColour(const ConfigNode& node, const std::shared_ptr<const UIColourScheme>& colourScheme)
{
	if (node.getType() != ConfigNodeType::String) {
		return Colour4f(1, 1, 1, 1);
	}
	
	const auto& str = node.asString();
	if (Colour4f::isColour(str)) {
		return Colour4f::fromString(str);
	} else if (str.startsWith("$") && colourScheme) {
		return colourScheme->getColour(str.mid(1));
	} else {
		Logger::logWarning("Invalid colour: \"" + str + "\"");
		return Colour4f();
	}
}

template <typename T>
void loadStyleData(UIStyleSheet& styleSheet, const String& name, const ConfigNode& node, T& data)
{
	throw Exception("Unreachable code", HalleyExceptions::UI);
}

template <>
void loadStyleData(UIStyleSheet& styleSheet, const String& name, const ConfigNode& node, Sprite& data)
{
	assert(styleSheet.getColourScheme() != nullptr);
	if (node.getType() == ConfigNodeType::String) {
		if (!node.asString().isEmpty()) {
			data = styleSheet.getColourScheme()->getSprite(styleSheet.getResources(), node.asString(), "");
		}
	} else {
		data = styleSheet.getColourScheme()->getSprite(styleSheet.getResources(), node["img"].asString(), node["material"].asString(""))
			.setColour(getColour(node["colour"], styleSheet.getColourScheme()));
	}
}

template <>
void loadStyleData(UIStyleSheet& styleSheet, const String& name, const ConfigNode& node, TextRenderer& data)
{
	const auto scale = styleSheet.getUIScale();
	data = TextRenderer()
		.setFont(styleSheet.getResources().get<Font>(node["font"].asString()))
		.setSize(node["size"].asFloat() * scale)
		.setColour(getColour(node["colour"], styleSheet.getColourScheme()))
		.setOutline(node["outline"].asFloat(0.0f) * scale)
		.setOutlineColour(Colour4f::fromString(node["outlineColour"].asString("#000000")))
		.setShadow(node["shadowDistance"].asVector2f({}) * scale, node["shadowSmoothness"].asFloat(1) * scale, Colour4f::fromString(node["shadowColour"].asString("#00000000")))
		.setAlignment(node["alignment"].asFloat(0.0f))
		.setSmoothness(node["smoothness"].asFloat(1.0))
		.setLineSpacing(node["lineSpacing"].asFloat(0) * scale);
}

template <>
void loadStyleData(UIStyleSheet& styleSheet, const String& name, const ConfigNode& node, String& data)
{
	if (node.asString().isEmpty()) {
		data = "";
	} else {
		data = node.asString();
	}
}

template <>
void loadStyleData(UIStyleSheet& styleSheet, const String& name, const ConfigNode& node, Vector4f& data)
{
	auto& vals = node.asSequence();
	data = Vector4f(vals[0].asFloat(), vals[1].asFloat(), vals[2].asFloat(), vals[3].asFloat());
}

template <>
void loadStyleData(UIStyleSheet& styleSheet, const String& name, const ConfigNode& node, float& data)
{
	data = node.asFloat();
}

template <>
void loadStyleData(UIStyleSheet& styleSheet, const String& name, const ConfigNode& node, Time& data)
{
	data = node.asFloat();
}

template <>
void loadStyleData(UIStyleSheet& styleSheet, const String& name, const ConfigNode& node, Vector2f& data)
{
	auto& vals = node.asSequence();
	data = Vector2f(vals[0].asFloat(), vals[1].asFloat());
}

template <>
void loadStyleData(UIStyleSheet& styleSheet, const String& name, const ConfigNode& node, Colour4f& data)
{
	data = getColour(node, styleSheet.getColourScheme());
}

template <>
void loadStyleData(UIStyleSheet& styleSheet, const String& name, const ConfigNode& node, std::shared_ptr<UIStyleDefinition>& data)
{
	if (node.getType() == ConfigNodeType::Map) {
		data = std::make_shared<UIStyleDefinition>(name, node, styleSheet);
	} else if (node.getType() == ConfigNodeType::String) {
		data = styleSheet.getStyle(node.asString());
	} else {
		data = {};
	}
}

template <typename T>
const T& getValue(const ConfigNode* node, UIStyleSheet& styleSheet, const String& name, const String& key, HashMap<String, T>& cache)
{
	Expects(node);
	node->assertValid();
	
	// Is it already in cache?
	const auto iter = cache.find(key);
	if (iter != cache.end()) {
		return iter->second;
	}

	// Not in cache, try to load it
	if (node->hasKey(key)) {
		T data;
		loadStyleData(styleSheet, key, (*node)[key], data);
		cache[key] = data;
		return cache[key];
	} else {
		// Not found. Use a default.
		const auto iter2 = cache.find(":default");
		if (iter2 != cache.end()) {
			Logger::logWarning(name + "." + key + " (" + String(typeid(T).name()) + ") not found in UI style.");
			return iter2->second;
		} else {
			throw Exception(name + "." + key + " (" + String(typeid(T).name()) + ") not found in UI style. Additionally, default was not set.", HalleyExceptions::Tools);
		}
	}
}

template <typename T>
bool hasValue(const ConfigNode* node, const String& key, HashMap<String, T>& cache, std::initializer_list<const ConfigNodeType> typeRequired)
{
	// Is it already in cache?
	const auto iter = cache.find(key);
	if (iter != cache.end()) {
		return true;
	}

	return (node->hasKey(key) && typeRequired.size() == 0) || std_ex::contains(typeRequired, (*node)[key].getType());
}

class UIStyleDefinition::Pimpl {
public:
	mutable HashMap<String, Sprite> sprites;
	mutable HashMap<String, TextRenderer> textRenderers;
	mutable HashMap<String, Vector4f> borders;
	mutable HashMap<String, String> strings;
	mutable HashMap<String, float> floats;
	mutable HashMap<String, Time> times;
	mutable HashMap<String, Vector2f> vector2fs;
	mutable HashMap<String, Colour4f> colours;
	mutable HashMap<String, std::shared_ptr<UIStyleDefinition>> subStyles;
};

UIStyleDefinition::UIStyleDefinition(String styleName, const ConfigNode& node, UIStyleSheet& styleSheet)
	: styleName(std::move(styleName))
	, node(&node)
	, styleSheet(styleSheet)
	, pimpl(std::make_unique<Pimpl>())
{
	if (node.getType() == ConfigNodeType::Map) {
		classes = node["classes"].asVector<String>({});
	}
	loadDefaults();
}

UIStyleDefinition::~UIStyleDefinition() = default;

std::shared_ptr<const UIStyleDefinition> UIStyleDefinition::getSubStyle(const String& name) const
{
	return getValue(node, styleSheet, styleName, name, pimpl->subStyles);
}

const String& UIStyleDefinition::getName() const
{
	return styleName;
}

const Sprite& UIStyleDefinition::getSprite(const String& name) const
{
	return getValue(node, styleSheet, styleName, name, pimpl->sprites);
}

const TextRenderer& UIStyleDefinition::getTextRenderer(const String& name) const
{
	return getValue(node, styleSheet, styleName, name, pimpl->textRenderers);
}

bool UIStyleDefinition::hasTextRenderer(const String& name) const
{
	return hasValue(node, name, pimpl->textRenderers, { ConfigNodeType::Map });
}

bool UIStyleDefinition::hasColour(const String& name) const
{
	return hasValue(node, name, pimpl->colours, { ConfigNodeType::String });
}

bool UIStyleDefinition::hasSubStyle(const String& name) const
{
	return hasValue(node, name, pimpl->subStyles, { ConfigNodeType::Map });
}

bool UIStyleDefinition::hasSprite(const String& name) const
{
	return hasValue(node, name, pimpl->sprites, { ConfigNodeType::String, ConfigNodeType::Map });
}

bool UIStyleDefinition::hasVector2f(const String& name) const
{
	return hasValue(node, name, pimpl->vector2fs, { ConfigNodeType::Sequence });
}

bool UIStyleDefinition::hasFloat(const String& name) const
{
	return hasValue(node, name, pimpl->floats, { ConfigNodeType::Float, ConfigNodeType::Int });
}

bool UIStyleDefinition::hasString(const String& name) const
{
	return hasValue(node, name, pimpl->strings, { ConfigNodeType::String, ConfigNodeType::Int, ConfigNodeType::Float });
}

Vector4f UIStyleDefinition::getBorder(const String& name) const
{
	return getValue(node, styleSheet, styleName, name, pimpl->borders);
}

Vector4f UIStyleDefinition::getBorder(const String& name, Vector4f defaultValue) const
{
	if (hasValue(node, name, pimpl->borders, { ConfigNodeType::Sequence })) {
		return getBorder(name);
	}

	return defaultValue;
}

const String& UIStyleDefinition::getString(const String& name) const
{
	return getValue(node, styleSheet, styleName, name, pimpl->strings);
}

float UIStyleDefinition::getFloat(const String& name) const
{
	return getValue(node, styleSheet, styleName, name, pimpl->floats) * styleSheet.getUIScale();
}

float UIStyleDefinition::getFloat(const String& name, float defaultValue) const
{
	if (hasValue(node, name, pimpl->floats, { ConfigNodeType::Int, ConfigNodeType::Float })) {
		return getFloat(name);
	}

	return defaultValue * styleSheet.getUIScale();
}

Time UIStyleDefinition::getTime(const String& name) const
{
	return getValue(node, styleSheet, styleName, name, pimpl->times);
}

Time UIStyleDefinition::getTime(const String& name, Time defaultValue) const
{
	if (hasValue(node, name, pimpl->times, { ConfigNodeType::Int, ConfigNodeType::Float })) {
		return getTime(name);
	}

	return defaultValue;	
}

Vector2f UIStyleDefinition::getVector2f(const String& name) const
{
	return getValue(node, styleSheet, styleName, name, pimpl->vector2fs) * styleSheet.getUIScale();
}

Vector2f UIStyleDefinition::getVector2f(const String& name, Vector2f defaultValue) const
{
	if (hasValue(node, name, pimpl->vector2fs, { ConfigNodeType::Sequence })) {
		return getVector2f(name);
	}

	return defaultValue * styleSheet.getUIScale();
}

Colour4f UIStyleDefinition::getColour(const String& name) const
{
	return getValue(node, styleSheet, styleName, name, pimpl->colours);
}

void UIStyleDefinition::reload(const ConfigNode& node)
{
	this->node = &node;
	loadDefaults();
}

gsl::span<const String> UIStyleDefinition::getClasses() const
{
	return classes;
}

bool UIStyleDefinition::hasClass(const String& className) const
{
	return std_ex::contains(classes, className);
}

void UIStyleDefinition::loadDefaults()
{
	pimpl->sprites.clear();
	pimpl->sprites[":default"] = Sprite();
	pimpl->textRenderers.clear();
	pimpl->textRenderers[":default"] = styleSheet.getDefaultTextRenderer();
	pimpl->floats.clear();
	pimpl->floats[":default"] = 0.0f;
	pimpl->vector2fs.clear();
	pimpl->vector2fs[":default"] = Vector2f();
	pimpl->borders.clear();
	pimpl->borders[":default"] = Vector4f();
	pimpl->strings.clear();
	pimpl->strings[":default"] = "";
	pimpl->colours.clear();
	pimpl->colours[":default"] = Colour4f(1, 1, 1, 1);

	pimpl->subStyles.clear();
	pimpl->subStyles[":default"] = {};
}

UIStyleSheet::UIStyleSheet(Resources& resources)
	: resources(resources)
{
	setupDefaultStyle();
}

UIStyleSheet::UIStyleSheet(Resources& resources, std::shared_ptr<const UIColourScheme> colourScheme)
	: resources(resources)
{
	setupDefaultStyle();
	for (auto& style: resources.enumerate<ConfigFile>()) {
		if (style.startsWith("ui_style/")) {
			load(*resources.get<ConfigFile>(style), colourScheme);
		}
	}
}

UIStyleSheet::UIStyleSheet(Resources& resources, const ConfigFile& file, std::shared_ptr<const UIColourScheme> colourScheme)
	: resources(resources)
{
	setupDefaultStyle();
	load(file, std::move(colourScheme));
}

void UIStyleSheet::load(const ConfigFile& file, std::shared_ptr<const UIColourScheme> colourScheme)
{
	load(file.getRoot(), file.getAssetId(), colourScheme);
	observers[file.getAssetId()] = ConfigObserver(file);
}

bool UIStyleSheet::updateIfNeeded()
{
	if (needsUpdate()) {
		update();
		return true;
	}
	return false;
}

void UIStyleSheet::reload(std::shared_ptr<const UIColourScheme> colourScheme)
{
	lastColourScheme = std::move(colourScheme);
	update();
}

bool UIStyleSheet::needsUpdate() const
{
	for (auto& o: observers) {
		if (o.second.needsUpdate()) {
			return true;
		}
	}
	return false;
}

void UIStyleSheet::update()
{
	for (auto& o: observers) {
		if (o.second.needsUpdate()) {
			o.second.update();
			load(o.second.getRoot(), o.first, lastColourScheme);
		}
	}
}

void UIStyleSheet::setupDefaultStyle()
{
	ConfigNode::MapType labelData;
	labelData["font"] = "Ubuntu Bold";
	labelData["size"] = 12;
	labelData["colour"] = "#000000";
	loadStyleData(*this, ":default", labelData, defaultTextRenderer);

	defaultData = ConfigNode::MapType();
	defaultStyle = std::make_unique<UIStyleDefinition>(":default", defaultData, *this);
}

void UIStyleSheet::load(const ConfigNode& root, const String& assetId, std::shared_ptr<const UIColourScheme> colourScheme)
{
	lastColourScheme = colourScheme;
	
	for (const auto& node: root["uiStyle"].asMap()) {
		// If it already exists, update existing instance (as it might be kept by UI elements all around)
		const auto iter = styles.find(node.first);
		if (iter != styles.end()) {
			iter->second->reload(node.second);
		} else {
			styles[node.first] = std::make_unique<UIStyleDefinition>(node.first, node.second, *this);
			styleToObserver[node.first] = assetId;
		}
	}
}

std::shared_ptr<const UIStyleDefinition> UIStyleSheet::getStyle(const String& styleName) const
{
	auto iter = styles.find(styleName);
	if (iter == styles.end()) {
		return defaultStyle;
	}
	return iter->second;
}

std::shared_ptr<UIStyleDefinition> UIStyleSheet::getStyle(const String& styleName)
{
	auto iter = styles.find(styleName);
	if (iter == styles.end()) {
		return defaultStyle;
	}
	return iter->second;
}

Vector<String> UIStyleSheet::getStylesForClass(const String& className) const
{
	Vector<String> result;
	getStylesForClass(result, className);
	return result;
}

void UIStyleSheet::getStylesForClass(Vector<String>& dst, const String& className) const
{
	for (const auto& [k, v]: styles) {
		if (v->hasClass(className)) {
			dst.push_back(v->getName());
		}
	}
}

bool UIStyleSheet::hasStyleObserver(const String& styleName) const
{
	return styleToObserver.find(styleName) != styleToObserver.end();
}

const ConfigObserver& UIStyleSheet::getStyleObserver(const String& styleName) const
{
	return observers.at(styleToObserver.at(styleName));
}

void UIStyleSheet::applyStyles(const UIStyleSheet& other)
{
	for (const auto& [k, v]: other.styles) {
		styles[k] = v;
	}
}

float UIStyleSheet::getUIScale() const
{
	return uiScale;
}

void UIStyleSheet::setUIScale(float scale)
{
	uiScale = scale;
}

const TextRenderer& UIStyleSheet::getDefaultTextRenderer() const
{
	return defaultTextRenderer;
}
