#include "halley/entity/data_interpolator.h"

#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include <components/network_component.h>

#include "halley/entity/entity_factory.h"
#include "halley/entity/world.h"
#include "halley/entity/components/transform_2d_component.h"

class Transform2DComponent;
using namespace Halley;

void DataInterpolatorSet::setInterpolator(std::shared_ptr<IDataInterpolator> interpolator, EntityId entity, std::string_view componentName, std::string_view fieldName)
{
	const auto key = makeKey(entity, componentName, fieldName);
	for (auto& entry: interpolators) {
		if (entry.first == key) {
			entry.second = std::move(interpolator);
			return;
		}
	}
	
	interpolators.emplace_back(key, std::move(interpolator));
}

IDataInterpolator* DataInterpolatorSet::tryGetInterpolator(EntityId entity, std::string_view componentName, std::string_view fieldName)
{
	if (interpolators.empty()) {
		return nullptr;
	}

	const auto key = makeKey(entity, componentName, fieldName);
	for (auto& entry: interpolators) {
		if (entry.first == key) {
			return entry.second.get();
		}
	}
	return nullptr;
}

bool DataInterpolatorSet::setInterpolatorEnabled(EntityId entityId, std::string_view componentName, std::string_view fieldName, bool enabled)
{
	auto* interpolator = tryGetInterpolator(entityId, componentName, fieldName);
	if (interpolator) {
		interpolator->setEnabled(enabled);
		return true;
	}
	return false;
}

bool DataInterpolatorSet::isReady() const
{
	return ready;
}

void DataInterpolatorSet::markReady()
{
	ready = true;
}

void DataInterpolatorSet::update(Time time, World& world) const
{
	for (auto& e: interpolators) {
		const bool modified = e.second->update(time, world, std::get<0>(e.first));

		// This hack is needed to make sure that transform 2D gets marked as dirty properly
		if (modified && std::get<1>(e.first) == "Transform2D") {
			auto entity = world.getEntity(std::get<0>(e.first));
			auto& transform = entity.getComponent<Transform2DComponent>();
			transform.markDirty();
		}
	}
}

size_t DataInterpolatorSet::count() const
{
	return interpolators.size();
}

DataInterpolatorSet::Key DataInterpolatorSet::makeKey(EntityId entity, std::string_view componentName, std::string_view fieldName) const
{
	return Key(entity, componentName, fieldName);
}

DataInterpolatorSetRetriever::DataInterpolatorSetRetriever(EntityRef rootEntity, bool shouldCollectUUIDs)
{
	auto* networkComponent = rootEntity.tryGetComponent<NetworkComponent>();
	if (networkComponent) {
		dataInterpolatorSet = &networkComponent->dataInterpolatorSet;

		if (shouldCollectUUIDs) {
			collectUUIDs(rootEntity);
		}
	}
}

IDataInterpolator* DataInterpolatorSetRetriever::tryGetInterpolator(const EntitySerializationContext& context, std::string_view componentName, std::string_view fieldName) const
{
	if (dataInterpolatorSet) {
		return dataInterpolatorSet->tryGetInterpolator(context.entityContext->getCurrentEntityId(), componentName, fieldName);
	} else {
		return nullptr;
	}
}

IDataInterpolator* DataInterpolatorSetRetriever::tryGetInterpolator(EntityId entityId, std::string_view componentName, std::string_view fieldName) const
{
	if (dataInterpolatorSet && entityId.isValid()) {
		return dataInterpolatorSet->tryGetInterpolator(entityId, componentName, fieldName);
	} else {
		return nullptr;
	}
}

ConfigNode DataInterpolatorSetRetriever::createComponentDelta(const UUID& instanceUUID, const String& componentName, const ConfigNode& from, const ConfigNode& origTo) const
{
	const auto iter = uuids.find(instanceUUID);
	const EntityId entityId = iter != uuids.end() ? iter->second : EntityId();

	ConfigNode to = ConfigNode(origTo);
	for (const auto& [fieldName, fromValue]: from.asMap()) {
		if (auto* interpolator = tryGetInterpolator(entityId, componentName, fieldName)) {
			if (auto newValue = interpolator->prepareFieldForSerialization(fromValue, origTo[fieldName])) {
				to[fieldName] = std::move(newValue.value());
			}
		}
	}
	
	return ConfigNode::createDelta(from, to);
}

void DataInterpolatorSetRetriever::collectUUIDs(EntityRef entity)
{
	uuids[entity.getInstanceUUID()] = entity.getEntityId();
	for (auto c: entity.getChildren()) {
		collectUUIDs(c);
	}
}



bool DeadReckoningInterpolator::update(Time time, World& world, EntityId entityId)
{
	return false;
}

std::optional<ConfigNode> DeadReckoningInterpolator::prepareFieldForSerialization(const ConfigNode& fromValue, const ConfigNode& toValue)
{
	return {};
}

void DeadReckoningInterpolator::setVelocity(Vector2f vel)
{
	outboundVel = vel;
}

void DeadReckoningInterpolator::setVelocityRef(Vector2f& value)
{
	velRef = &value;
}

void DeadReckoningInterpolator::doDeserialize(Vector2f& value, const Vector2f& defaultValue, const EntitySerializationContext& context, const ConfigNode& node)
{
}

DeadReckoningVelocityInterpolator::DeadReckoningVelocityInterpolator(std::shared_ptr<DeadReckoningInterpolator> parent)
	: parent(std::move(parent))
{
}

bool DeadReckoningVelocityInterpolator::update(Time time, World& world, EntityId entityId)
{
	return false;
}

std::optional<ConfigNode> DeadReckoningVelocityInterpolator::prepareFieldForSerialization(const ConfigNode& fromValue, const ConfigNode& toValue)
{
	parent->setVelocity(toValue.asVector2f());
	return ConfigNode(ConfigNode::NoopType());
}

void DeadReckoningVelocityInterpolator::doDeserialize(Vector2f& value, const Vector2f& defaultValue, const EntitySerializationContext& context, const ConfigNode& node)
{
	parent->setVelocityRef(value);
}
