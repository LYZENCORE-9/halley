#include "ui_widget_editor.h"

#include "ui_editor.h"
#include "src/scene/entity_editor.h"
#include "src/ui/project_window.h"

using namespace Halley;

UIWidgetEditor::UIWidgetEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(300, 100), UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "halley/ui_widget_editor");
}

void UIWidgetEditor::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "addSizer", [=] (const UIEvent& event)
	{
		auto sizerBox = getWidget("sizerBox");
		auto addSizerButton = getWidget("addSizer");
		auto removeSizerButton = getWidget("removeSizer");
		sizerBox->setActive(true);
		addSizerButton->setActive(false);
		removeSizerButton->setEnabled(true);
		populateSizerBox(*sizerBox->getWidget("sizerContents"), (*curNode)["sizer"]);

		(*curNode)["sizer"] = ConfigNode::MapType();
		onEntityUpdated(false);
	});

	setHandle(UIEventType::ButtonClicked, "removeSizer", [=] (const UIEvent& event)
	{
		auto sizerBox = getWidget("sizerBox");
		auto addSizerButton = getWidget("addSizer");
		auto removeSizerButton = getWidget("removeSizer");
		sizerBox->setActive(false);
		addSizerButton->setActive(true);
		addSizerButton->setEnabled(true);

		curNode->removeKey("sizer");
		onEntityUpdated(false);
	});

	setHandle(UIEventType::ButtonClicked, "addBehaviour", [=] (const UIEvent& event)
	{
		addBehaviourToWidget();
	});

	setHandle(UIEventType::ButtonClicked, "pasteBehaviours", [=] (const UIEvent& event)
	{
		pasteBehaviours();
	});
}

void UIWidgetEditor::setSelectedWidget(const String& id, ConfigNode* node, const ConfigNode* parent)
{
	curId = id;
	curNode = node;
	curParent = parent;
	refresh();
}

void UIWidgetEditor::setGameResources(Resources& resources)
{
	refresh();
}

void UIWidgetEditor::setUIEditor(UIEditor& editor, ProjectWindow& project)
{
	uiEditor = &editor;
	projectWindow = &project;
	entityFieldFactory = std::make_shared<EntityEditorFactory>(editor.isEditingHalleyUI() ? projectWindow->getHalleyEntityEditorFactoryRoot() : projectWindow->getEntityEditorFactoryRoot(), static_cast<IEntityEditorCallbacks*>(this));
}

void UIWidgetEditor::onEntityUpdated(bool temporary)
{
	uiEditor->onWidgetModified(curId, temporary);
}

void UIWidgetEditor::reloadEntity()
{
}

void UIWidgetEditor::setTool(const String& tool, const String& componentName, const String& fieldName)
{
}

void UIWidgetEditor::setDefaultName(const String& name, const String& prevName)
{
}

void UIWidgetEditor::refresh()
{
	auto parentWidgetBox = getWidget("parentWidgetBox");
	auto widgetBox = getWidget("widgetBox");
	auto genericWidgetBox = getWidget("genericWidgetBox");
	auto fillBox = getWidget("fillBox");
	auto sizerBox = getWidget("sizerBox");
	auto spacerBox = getWidget("spacerBox");
	auto addSizerButton = getWidget("addSizer");
	auto removeSizerButton = getWidget("removeSizer");
	auto behavioursBox = getWidget("behavioursBox");

	parentWidgetBox->setActive(false);
	widgetBox->setActive(false);
	genericWidgetBox->setActive(false);
	fillBox->setActive(false);
	sizerBox->setActive(false);
	spacerBox->setActive(false);
	addSizerButton->setActive(false);
	behavioursBox->setActive(false);
	
	if (curNode && entityFieldFactory) {
		bool hasSizer = curNode->hasKey("sizer") || curNode->hasKey("children");
		bool hasChildren = curNode->hasKey("children") && (*curNode)["children"].getType() == ConfigNodeType::Sequence && !(*curNode)["children"].asSequence().empty();
		bool canHaveSizer = false;
		const bool isSpacer = curNode->hasKey("spacer") || curNode->hasKey("stretchSpacer");

		curNode->asMap().reserve(15); // Important: this prevents references being invalidated during a re-hash

		if (curNode->hasKey("widget")) {
			auto& widgetNode = (*curNode)["widget"];
			widgetNode.ensureType(ConfigNodeType::Map);
			const auto widgetClass = widgetNode["class"].asString();

			const auto properties = uiEditor->getGameFactory().getPropertiesForWidget(widgetNode["class"].asString());
			canHaveSizer = properties.canHaveChildren;

			if (widgetClass != "widget" && !properties.entries.empty()) {
				widgetBox->setActive(true);
				populateWidgetBox(*widgetBox->getWidget("widgetContents"), widgetNode, properties);
			}

			genericWidgetBox->setActive(true);
			populateGenericWidgetBox(*genericWidgetBox->getWidget("genericWidgetContents"), widgetNode);
		}

		sizerBox->setActive(hasSizer);
		addSizerButton->setActive(!hasSizer);
		if (hasSizer) {
			populateSizerBox(*sizerBox->getWidget("sizerContents"), (*curNode)["sizer"]);
			removeSizerButton->setEnabled(!hasChildren);
		} else {
			addSizerButton->setEnabled(canHaveSizer);
		}

		if (isSpacer) {
			spacerBox->setActive(true);
			populateSpacerBox(*spacerBox->getWidget("spacerContents"), (*curNode)[curNode->hasKey("stretchSpacer") ? "stretchSpacer" : "spacer"], *curNode);
		} else {
			fillBox->setActive(true);
			populateFillBox(*fillBox->getWidget("fillContents"), *curNode);
		}

		behavioursBox->setActive(true);
		populateBehavioursBox(*behavioursBox->getWidget("behavioursContents"), *curNode);
	}

	if (curNode && curParent && entityFieldFactory) {
		if (curParent->hasKey("widget")) {
			auto& widgetNode = (*curParent)["widget"];
			const auto parentWidgetClass = widgetNode["class"].asString();
			auto parentProperties = uiEditor->getGameFactory().getPropertiesForWidget(parentWidgetClass);
			if (!parentProperties.childEntries.empty()) {
				parentWidgetBox->setActive(true);
				populateParentWidgetBox(*parentWidgetBox->getWidget("parentWidgetContents"), *curNode, parentProperties);
			}
		}
	}
}

void UIWidgetEditor::populateParentWidgetBox(UIWidget& root, ConfigNode& node, const UIFactoryWidgetProperties& properties)
{
	root.clear();

	auto label = getWidgetAs<UILabel>("parentClassLabel");
	label->setText(LocalisedString::fromUserString(properties.childName));

	populateBox(root, node, properties.childEntries);
}

void UIWidgetEditor::populateWidgetBox(UIWidget& root, ConfigNode& node, const UIFactoryWidgetProperties& properties)
{
	root.clear();

	auto icon = getWidgetAs<UIImage>("classIcon");
	auto label = getWidgetAs<UILabel>("classLabel");
	label->setText(LocalisedString::fromUserString(properties.name));
	icon->setSprite(Sprite().setImage(uiEditor->getGameFactory().getResources(), properties.iconName));

	populateBox(root, node, properties.entries);
}

void UIWidgetEditor::populateGenericWidgetBox(UIWidget& root, ConfigNode& node)
{
	root.clear();

	populateBox(root, node, uiEditor->getGameFactory().getGlobalWidgetProperties().entries);
}

void UIWidgetEditor::populateFillBox(UIWidget& root, ConfigNode& node)
{
	root.clear();

	using Entry = UIFactoryWidgetProperties::Entry;
	std::array<Entry, 3> entries = {
		Entry{ "Fill", "fill", "Halley::UISizerAlignFlags::Type", Vector<String>{"fill"} },
		Entry{ "Proportion", "proportion", "int", Vector<String>{"0"} },
		Entry{ "Border", "border", "Halley::Vector4f", Vector<String>{"0", "0", "0", "0"}}
	};
	populateBox(root, node, entries);
}

void UIWidgetEditor::populateSpacerBox(UIWidget& root, ConfigNode& node, ConfigNode& rootNode)
{
	root.clear();

	using Entry = UIFactoryWidgetProperties::Entry;
	std::array<Entry, 1> sizeEntry = {
		Entry{ "Size", "size", "float", Vector<String>{"0"}},
	};
	std::array<Entry, 1> proportionEntry = {
		Entry{ "Proportion", "proportion", "int", Vector<String>{"0"} },
	};
	populateBox(root, node, sizeEntry);
	populateBox(root, rootNode, proportionEntry);
}

void UIWidgetEditor::populateSizerBox(UIWidget& root, ConfigNode& node)
{
	node.ensureType(ConfigNodeType::Map);
	root.clear();

	using Entry = UIFactoryWidgetProperties::Entry;
	std::array<Entry, 4> entries = {
		Entry{ "Type", "type", "Halley::UISizerType", Vector<String>{"horizontal"} },
		Entry{ "Gap", "gap", "float", Vector<String>{"1"} },
		Entry{ "Columns", "columns", "int", Vector<String>{"1"}},
		Entry{ "Column Proportions", "columnProportions", "Halley::Vector<int>", Vector<String>{}}
	};
	populateBox(root, node, entries);
}

void UIWidgetEditor::populateBehavioursBox(UIWidget& root, ConfigNode& node)
{
	root.clear();

	if (!node.hasKey("behaviours")) {
		return;
	}

	auto& seq = node["behaviours"].asSequence();
	for (size_t i = 0; i < seq.size(); ++i) {
		if (auto widget = makeBehaviourUI(seq[i], i)) {
			root.add(widget);
		}
	}
}

void UIWidgetEditor::populateBox(UIWidget& root, ConfigNode& node, gsl::span<const UIFactoryWidgetProperties::Entry> entries)
{
	node.ensureType(ConfigNodeType::Map);
	for (const auto& e: entries) {
		const auto params = ComponentFieldParameters("", ComponentDataRetriever(node, e.name, e.label), e.defaultValue, {}, ConfigNode(e.options));
		auto field = entityFieldFactory->makeField(e.type, params, ComponentEditorLabelCreation::Always);
		root.add(field);
	}
}

void UIWidgetEditor::addBehaviourToWidget()
{
	const auto window = std::make_shared<ChooseUIWidgetWindow>(factory, factory, false, ChooseUIWidgetWindow::Mode::Behaviour, [=] (std::optional<String> result)
	{
		if (result) {
			addBehaviourToWidget(result.value());
		}
	});
	getRoot()->addChild(window);
}

void UIWidgetEditor::addBehaviourToWidget(const String& id)
{
	(*curNode)["behaviours"].ensureType(ConfigNodeType::Sequence);
	auto& seq = (*curNode)["behaviours"].asSequence();

	ConfigNode::MapType behaviourEntry;
	behaviourEntry["class"] = id;
	seq.push_back(behaviourEntry);

	onEntityUpdated(false);
	refresh();
}

void UIWidgetEditor::deleteBehaviour(size_t idx)
{
	auto& seq = (*curNode)["behaviours"].asSequence();
	seq.erase(seq.begin() + idx);
	if (seq.empty()) {
		curNode->removeKey("behaviours");
	}

	onEntityUpdated(false);
	refresh();
}

std::shared_ptr<UIWidget> UIWidgetEditor::makeBehaviourUI(ConfigNode& behaviourConfig, size_t idx)
{
	const auto behaviourClass = behaviourConfig["class"].asString("");
	if (behaviourClass.isEmpty()) {
		return {};
	}

	auto widget = factory.makeUI("halley/ui_widget_behaviour_editor");

	const auto& properties = factory.getPropertiesForBehaviour(behaviourClass);
	widget->getWidgetAs<UILabel>("behaviourName")->setText(LocalisedString::fromUserString(properties.name));
	populateBox(*widget->getWidget("behaviourFields"), behaviourConfig, properties.entries);

	widget->setHandle(UIEventType::ButtonClicked, "copyButton", [this, &behaviourConfig] (const UIEvent& event)
	{
		ConfigNode result;
		result["behaviours"].ensureType(ConfigNodeType::Sequence);

		if ((event.getKeyMods() & KeyMods::Ctrl) != KeyMods::None) {
			// Append
			result["behaviours"] = getBehavioursFromClipboard();
		}

		result["behaviours"].asSequence().push_back(behaviourConfig);

		auto clipboard = projectWindow->getAPI().system->getClipboard();
		clipboard->setData(YAMLConvert::generateYAML(result, {}));
	});

	widget->setHandle(UIEventType::ButtonClicked, "deleteButton", [this, idx] (const UIEvent& event)
	{
		Concurrent::execute(Executors::getMainUpdateThread(), [this, idx]
		{
			deleteBehaviour(idx);
		});
	});

	return widget;
}

void UIWidgetEditor::pasteBehaviours()
{
	auto& behs = (*curNode)["behaviours"];
	behs.ensureType(ConfigNodeType::Sequence);

	bool modified = false;
	for (const auto& e: getBehavioursFromClipboard()) {
		behs.asSequence().push_back(e);
		modified = true;
	}

	if (modified) {
		onEntityUpdated(false);
		refresh();
	}
}

Vector<ConfigNode> UIWidgetEditor::getBehavioursFromClipboard() const
{
	Vector<ConfigNode> result;

	const auto clipboard = projectWindow->getAPI().system->getClipboard();
	const auto data = clipboard->getStringData();
	if (data && !data->isEmpty()) {
		try {
			auto config = YAMLConvert::parseConfig(*data, {});
			if (config.hasKey("behaviours")) {
				result = config["behaviours"].asSequence();
			}
		} catch (...) {}
	}

	return result;
}
