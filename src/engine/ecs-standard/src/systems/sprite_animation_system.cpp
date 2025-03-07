#include <systems/sprite_animation_system.h>

using namespace Halley;

class SpriteAnimationSystem final : public SpriteAnimationSystemBase<SpriteAnimationSystem> {
public:
	void update(Time time) {
		updateAnimators(time);
		updateReplicators();
	}

	void onMessageReceived(const PlayAnimationMessage& msg, MainFamily& e)
	{
		e.spriteAnimation.player.setSequence(msg.sequence);
	}

	void onMessageReceived(const PlayAnimationOnceMessage& msg, MainFamily& e)
	{
		e.spriteAnimation.player.playOnce(msg.sequence);
	}

	void onEntitiesAdded(Span<MainFamily> es)
	{
		for (auto& e: es) {
			e.spriteAnimation.player.update(0.0f);
			if (e.spriteAnimation.updateSprite) {
				e.spriteAnimation.player.updateSprite(e.sprite.sprite);
			}
		}
	}

	void onEntitiesReloaded(Span<MainFamily*> es)
	{
		for (auto& e : es) {
			e->spriteAnimation.player.update(0.0f);
			if (e->spriteAnimation.updateSprite) {
				e->spriteAnimation.player.updateSprite(e->sprite.sprite);
			}
		}
	}

private:

	void updateAnimators(Time time)
	{
		const auto viewPort = getScreenService().getCameraViewPort().grow(10, 10, 10, 10);
		for (auto& e : mainFamily) {
			auto& sprite = e.sprite.sprite;

			auto& player = e.spriteAnimation.player;
			player.update(time);

			if (e.spriteAnimation.updateSprite && player.hasAnimation()) {
				auto spriteBounds = Rect4f(player.getAnimation().getBounds()) + e.transform2D.getGlobalPositionWithHeight();
				if (spriteBounds.overlaps(viewPort)) {
					player.updateSprite(sprite);
				}
			}
		}
	}

	void updateReplicators()
	{
		// Because replicators can be nested, this needs to be a multi-step algorithm
		// This ensures that a parent's replicator is always updated before the child's, avoiding introducing frame delays

		HashSet<EntityId> replicatorsUpdated;
		HashSet<EntityId> availableReplicators;
		HashMap<EntityId, Vector<EntityId>> dependencies;
		std::list<EntityId> toReplicate;

		auto tryReplicating = [&] (EntityId id)
		{
			auto entity = getWorld().getEntity(id);
			if (const auto parent = entity.tryGetParent(); parent.has_value()) {
				if (const auto* parentAnimation = parent->tryGetComponent<SpriteAnimationComponent>()) {
					const bool parentHasReplicator = availableReplicators.contains(parent->getEntityId());
					if (parentHasReplicator && !replicatorsUpdated.contains(parent->getEntityId())) {
						// We'll do this one later
						dependencies[parent->getEntityId()].push_back(id);
						return;
					}

					// These two are guaranteed to be here by the family
					auto& spriteAnimation = entity.getComponent<SpriteAnimationComponent>();
					auto& sprite = entity.getComponent<SpriteComponent>();

					spriteAnimation.player.syncWith(parentAnimation->player, false);
					if (spriteAnimation.updateSprite) {
						spriteAnimation.player.updateSprite(sprite.sprite);
					}
					replicatorsUpdated.emplace(id);

					if (dependencies.contains(id)) {
						for (const auto child: dependencies.at(id)) {
							toReplicate.push_back(child);
						}
					}
				}
			}
		};

		// Try the originals...
		for (auto& e : replicatorFamily) {
			availableReplicators.emplace(e.entityId);
		}
		for (auto& e : replicatorFamily) {
			tryReplicating(e.entityId);
		}

		// Anything that needed to wait on parent is waiting here, keep going until this is empty
		while (!toReplicate.empty()) {
			const auto e = toReplicate.front();
			toReplicate.pop_front();
			tryReplicating(e);
		}
	}
};

REGISTER_SYSTEM(SpriteAnimationSystem)

