#include "properties/audio_properties.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"
using namespace Halley;

AudioSwitchProperties::AudioSwitchProperties(const ConfigNode& node)
{
	id = node["id"].asString();
	values = node["values"].asVector<String>();
}

ConfigNode AudioSwitchProperties::toConfigNode() const
{
	ConfigNode::MapType result;
	result["id"] = id;
	result["values"] = values;
	return result;
}

void AudioSwitchProperties::serialize(Serializer& s) const
{
	s << id;
	s << values;
}

void AudioSwitchProperties::deserialize(Deserializer& s)
{
	s >> id;
	s >> values;
}

const String& AudioSwitchProperties::getId() const
{
	return id;
}

void AudioSwitchProperties::setId(String value)
{
	id = std::move(value);
}

gsl::span<const String> AudioSwitchProperties::getValues() const
{
	return values;
}

gsl::span<String> AudioSwitchProperties::getValues()
{
	return values;
}

AudioVariableProperties::AudioVariableProperties(const ConfigNode& node)
{
	id = node["id"].asString();
	range = node["range"].asFloatRange();
}

ConfigNode AudioVariableProperties::toConfigNode() const
{
	ConfigNode::MapType result;
	result["id"] = id;
	result["range"] = range;
	return result;
}

void AudioVariableProperties::serialize(Serializer& s) const
{
	s << id;
	s << range;
}

void AudioVariableProperties::deserialize(Deserializer& s)
{
	s >> id;
	s >> range;
}

const String& AudioVariableProperties::getId() const
{
	return id;
}

void AudioVariableProperties::setId(String value)
{
	id = std::move(value);
}

Range<float> AudioVariableProperties::getRange() const
{
	return range;
}

Range<float>& AudioVariableProperties::getRange()
{
	return range;
}

AudioBusProperties::AudioBusProperties(const ConfigNode& node)
{
	id = node["id"].asString();
	children = node["children"].asVector<AudioBusProperties>();
}

ConfigNode AudioBusProperties::toConfigNode() const
{
	ConfigNode::MapType result;
	result["id"] = id;
	result["children"] = children;
	return result;
}

void AudioBusProperties::serialize(Serializer& s) const
{
	s << id;
	s << children;
}

void AudioBusProperties::deserialize(Deserializer& s)
{
	s >> id;
	s >> children;
}

const String& AudioBusProperties::getId() const
{
	return id;
}

void AudioBusProperties::setId(String value)
{
	id = std::move(value);
}

gsl::span<const AudioBusProperties> AudioBusProperties::getChildren() const
{
	return children;
}

gsl::span<AudioBusProperties> AudioBusProperties::getChildren()
{
	return children;
}

AudioProperties::AudioProperties(const ConfigNode& node)
{
	variables = node["variables"].asVector<AudioVariableProperties>({});
	switches = node["switches"].asVector<AudioSwitchProperties>({});
	buses = node["buses"].asVector<AudioBusProperties>({});
}

ConfigNode AudioProperties::toConfigNode() const
{
	ConfigNode::MapType result;
	result["variables"] = variables;
	result["switches"] = switches;
	result["buses"] = buses;
	return result;
}

void AudioProperties::serialize(Serializer& s) const
{
	s << switches;
	s << variables;
	s << buses;
}

void AudioProperties::deserialize(Deserializer& s)
{
	s >> switches;
	s >> variables;
	s >> buses;
}

gsl::span<const AudioSwitchProperties> AudioProperties::getSwitches() const
{
	return switches;
}

gsl::span<AudioSwitchProperties> AudioProperties::getSwitches()
{
	return switches;
}

gsl::span<const AudioVariableProperties> AudioProperties::getVariables() const
{
	return variables;
}

gsl::span<AudioVariableProperties> AudioProperties::getVariables()
{
	return variables;
}

gsl::span<const AudioBusProperties> AudioProperties::getBuses() const
{
	return buses;
}

gsl::span<AudioBusProperties> AudioProperties::getBuses()
{
	return buses;
}
