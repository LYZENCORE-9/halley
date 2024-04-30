#include <systems/network_receive_system.h>

using namespace Halley;

class NetworkReceiveSystem final : public NetworkReceiveSystemBase<NetworkReceiveSystem> {
public:
	
	void init()
	{
		if (getSessionService().isMultiplayer()) {
			getSessionService().getMultiplayerSession().getEntityNetworkSession()->setWorld(getWorld(), getMessageBridge());

			setupCheats();
		}
	}

	void update(Time t)
	{
		auto& world = getWorld();
		const bool sessionAlive = getSessionService().getSession()->update();

		if (!sessionAlive && !requestedExit) {
			requestedExit = true;
			if (auto* exitGameInterface = world.tryGetInterface<IExitGameInterface>()) {
				exitGameInterface->exitGame();
			} else {
				getAPI().core->quit();
			}
		}

		for (auto& e: networkFamily) {
			e.network.dataInterpolatorSet.update(t, world);
		}
	}

private:

	bool requestedExit = false;

	void setupCheats()
	{
		auto getQuality = UIDebugConsoleSyntax::Callback([=] () -> Vector<String>
		{
			return { "best", "average", "bad", "veryBad" };
		});
		
		getDevService().getConsoleCommands().addCommand("networkQuality", [=](Vector<String> args) -> String
		{
			const auto level = fromString<NetworkService::Quality>(args[0]);
			getSessionService().getMultiplayerSession().setNetworkQuality(level);
			return "Set network quality level to " + args[0];
		}, { { "quality", getQuality } });
	}

	std::optional<NetworkSession::PeerId> getPeerId() const
	{
		if (!getSessionService().isMultiplayer()) {
			return {};
		}
		auto& mpSession = getSessionService().getMultiplayerSession();
		return mpSession.getNetworkSession()->getMyPeerId();
	}
};

REGISTER_SYSTEM(NetworkReceiveSystem)
