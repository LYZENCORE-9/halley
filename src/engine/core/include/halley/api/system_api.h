#pragma once
#include "save_data.h"
#include "clipboard.h"
#include "halley/resources/resource_data.h"
#include "halley/graphics/window.h"
#include "halley/concurrency/concurrent.h"
#include "halley/data_structures/maybe.h"
#include "halley/input/input_keys.h"

namespace Halley
{
	class VideoAPI;
	class InputAPI;
	class HalleyAPIInternal;

	using SystemOnSuspendCallback = std::function<void(void)>;
	using SystemOnResumeCallback = std::function<void(void)>;

	class GLContext
	{
	public:
		virtual ~GLContext() {}
		virtual void bind() = 0;
		virtual std::unique_ptr<GLContext> createSharedContext() = 0;
		virtual void* getGLProcAddress(const char* name) = 0;
	};

	class ISystemMainLoopHandler {
	public:
		virtual ~ISystemMainLoopHandler() = default;
		virtual bool run() = 0;
	};

	class SystemAPI
	{
	public:
		virtual ~SystemAPI() {}

		virtual Path getAssetsPath(const Path& gamePath) const = 0;
		virtual Path getUnpackedAssetsPath(const Path& gamePath) const = 0;

		virtual std::unique_ptr<ResourceDataReader> getDataReader(String path, int64_t start = 0, int64_t end = -1) = 0;
		
		virtual std::unique_ptr<GLContext> createGLContext() = 0;

		virtual std::shared_ptr<Window> createWindow(const WindowDefinition& window) = 0;
		virtual void destroyWindow(std::shared_ptr<Window> window) = 0;

		virtual Vector2i getScreenSize(int n) const = 0;
		virtual int getNumDisplays() const { return 1; }
		virtual Rect4i getDisplayRect(int screen) const = 0;
		virtual void setEnableScreensaver(bool enabled) const {}

		virtual void showCursor(bool show) = 0;
		virtual bool hasBeenDisconnectedFromTheInternet() { return false; }

		virtual std::shared_ptr<ISaveData> getStorageContainer(SaveDataType type, const String& containerName = "") = 0;

		virtual std::thread createThread(const String& name, ThreadPriority priority, std::function<void()> runnable)
		{
			return std::thread([=] () {
				setThreadName(name);
				setThreadPriority(priority);
				runnable();
			});
		}

		virtual void setThreadName(const String& name) {}
		virtual void setThreadPriority(ThreadPriority priority) {}

		virtual bool mustOwnMainLoop() const { return false; }
		virtual void runGame(std::function<void()> runnable) { runnable(); }
		virtual void setGameLoopHandler(std::unique_ptr<ISystemMainLoopHandler> handler) {}
		virtual bool canExit() { return false; }

		virtual std::shared_ptr<IClipboard> getClipboard() const { return {}; }

		virtual void setOnSuspendCallback(SystemOnSuspendCallback callback) {}
		virtual void setOnResumeCallback(SystemOnResumeCallback callback) {}

		virtual std::optional<String> getGameVersion() const { return {}; }

		virtual void onTickMainLoop() {}

		virtual void registerGlobalHotkey(KeyCode key, KeyMods keyMods, std::function<void()> callback) {}

		struct MemoryUsage {
			uint64_t ramUsage = 0;
			std::optional<uint64_t> ramMax;
			uint64_t vramUsage = 0;
			std::optional<uint64_t> vramMax;
		};
		virtual MemoryUsage getMemoryUsage() { return {}; }

		using ResolutionChangeCallback = std::function<void(Vector2i)>;
		virtual void setResolutionChangeCallback(int screen, ResolutionChangeCallback callback) {}

	private:
		friend class HalleyAPI;
		friend class Core;

		virtual bool generateEvents(VideoAPI* video, InputAPI* input) = 0;
	};
}
