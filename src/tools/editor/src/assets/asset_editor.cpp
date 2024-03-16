#include "asset_editor.h"

using namespace Halley;

AssetEditor::AssetEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type)
	: UIWidget("assetEditor", {}, UISizer())
	, factory(factory)
	, project(project)
	, gameResources(gameResources)
	, assetType(type)
{
	setHandle(UIEventType::TabbedIn, [=] (const auto& event)
	{
		setTabbedIn(true);
	});

	setHandle(UIEventType::TabbedOut, [=] (const auto& event)
	{
		setTabbedIn(false);
	});
}

void AssetEditor::update(Time t, bool moved)
{
	tryLoading();
}

void AssetEditor::setResource(Path filePath, String assetId)
{
	this->assetPath = std::move(filePath);
	this->assetId = std::move(assetId);
	needsLoading = true;
}

void AssetEditor::clearResource()
{
	assetId = "";
	resource.reset();
	reload();
}

void AssetEditor::reload()
{
}

void AssetEditor::refreshAssets()
{
	// If this is set to true, it causes scenes to re-load when anything is modified
	//needsLoading = true;
	tryLoading();
}

void AssetEditor::onDoubleClick()
{
}

bool AssetEditor::isModified()
{
	return false;
}

void AssetEditor::save()
{
}

bool AssetEditor::canSave(bool forceInstantCheck) const
{
	return true;
}

void AssetEditor::onOpenAssetFinder(PaletteWindow& assetFinder)
{
}

void AssetEditor::onTabbedIn()
{
}

void AssetEditor::setTabbedIn(bool value)
{
	tabbedIn = value;
	tryLoading();
	if (value) {
		onTabbedIn();
	}
}

void AssetEditor::tryLoading()
{
	if (tabbedIn && needsLoading) {
		load();
	}
}

void AssetEditor::load()
{
	try {
		needsLoading = false;
		resource = loadResource(assetId);
		reload();
	} catch (const std::exception& e) {
		Logger::logException(e);
	}
}
