#include "halley/game/game.h"

#include "halley/editor_extensions/asset_preview_generator.h"
#include "halley/scripting/script_node_type.h"
#include "halley/ui/ui_factory.h"
using namespace Halley;

Game::~Game() = default;

void Game::init(const Environment&, const Vector<String>&)
{}

ResourceOptions Game::initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator)
{
	return {};
}

String Game::getLogFileName() const
{
    return "log.txt";
}

bool Game::shouldCreateSeparateConsole() const
{
	return isDevMode();
}

Game::ConsoleInfo Game::getConsoleInfo() const
{
	return ConsoleInfo{ getName() + " [Console]", {}, Vector2f(0.5f, 0.5f) };
}

void Game::endGame()
{}

std::unique_ptr<Stage> Game::makeStage(StageID)
{
	return {};
}

std::unique_ptr<BaseFrameData> Game::makeFrameData()
{
	return std::make_unique<DefaultFrameData>();
}

double Game::getTargetFPS() const
{
	return 0.0;
}

double Game::getTargetBackgroundFPS() const
{
	return getTargetFPS();
}

double Game::getFixedUpdateFPS() const
{
	return 60.0;
}

String Game::getDevConAddress() const
{
	return "";
}

int Game::getDevConPort() const
{
	return 12500;
}

std::shared_ptr<GameConsole> Game::getGameConsole() const
{
	return {};
}

void Game::onUncaughtException(const Exception& exception, TimeLine timeLine)
{
	throw exception;
}

std::unique_ptr<ISceneEditor> Game::createSceneEditorInterface()
{
	return {};
}

std::unique_ptr<IEditorCustomTools> Game::createEditorCustomToolsInterface()
{
	return {};
}

std::unique_ptr<AssetPreviewGenerator> Game::createAssetPreviewGenerator(const HalleyAPI& api, Resources& resources, IGameEditorData* gameEditorData)
{
	return std::make_unique<AssetPreviewGenerator>(*this, api, resources);
}

std::unique_ptr<UIFactory> Game::createUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n)
{
	auto factory = std::make_unique<UIFactory>(api, resources, i18n);
	const auto colourScheme = getDefaultColourScheme();
	if (!colourScheme.isEmpty()) {
		factory->setColourScheme(colourScheme);
	}
	factory->loadStyleSheetsFromResources();
	return factory;
}

std::unique_ptr<ScriptNodeTypeCollection> Game::createScriptNodeTypeCollection()
{
	return std::make_unique<ScriptNodeTypeCollection>();
}

Vector<std::unique_ptr<IComponentEditorFieldFactory>> Game::createCustomEditorFieldFactories(Resources& gameResources, IGameEditorData* gameEditorData)
{
	return {};
}

Vector<std::unique_ptr<IComponentEditorFieldFactory>> Game::createCustomScriptEditorFieldFactories(const Scene& scene, Resources& resources, IGameEditorData* gameEditorData)
{
	return {};
}

std::unique_ptr<IGameEditorData> Game::createGameEditorData(const HalleyAPI& api, Resources& resources)
{
	return {};
}

Vector<ConfigBreadCrumb> Game::createConfigBreadCrumbs()
{
	return {};
}

String Game::getLocalisationFileCategory(const String& assetName)
{
	return "unknown";
}

bool Game::canCollectVideoPerformance()
{
	return true;
}

String Game::getDefaultColourScheme()
{
	return "";
}

void Game::attachToEditorDebugConsole(UIDebugConsoleCommands& commands, Resources& gameResources, IProject& project)
{
}

const HalleyAPI& Game::getAPI() const
{
	if (!api) {
		throw Exception("HalleyAPI is only initialized on Game right before call to startGame()", HalleyExceptions::Core);
	}
	return *api;
}

Resources& Game::getResources() const
{
	if (!resources) {
		throw Exception("Resources are only initialized on Game right before call to startGame()", HalleyExceptions::Core);
	}
	return *resources;
}

std::optional<int> Game::getCurrentDisplay() const
{
	auto& video = *getAPI().video;
	if (video.hasWindow()) {
		const auto windowRect = video.getWindow().getWindowRect();
		const auto windowCentre = windowRect.getCenter();

		auto& system = *getAPI().system;
		int nDisplays = system.getNumDisplays();
		for (int i = 0; i < nDisplays; ++i) {
			if (system.getDisplayRect(i).contains(windowCentre)) {
				return i;
			}
		}
		return video.getWindow().getDefinition().getScreen();
	}
	return {};
}

size_t Game::getMaxThreads() const
{
	return std::thread::hardware_concurrency();
}

bool Game::shouldProcessEventsOnFixedUpdate() const
{
	return false;
}
