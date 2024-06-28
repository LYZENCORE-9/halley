#include "halley/file_formats/yaml_convert.h"
#include "halley/file_formats/halley-yamlcpp.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/text/encode.h"
using namespace Halley;

ConfigNode YAMLConvert::parseYAMLNode(const YAML::Node& node)
{
	ConfigNode result;

	if (node.IsMap()) {
		ConfigNode::MapType map;
		for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
			String key = it->first.as<std::string>();
			map[std::move(key)] = parseYAMLNode(it->second);
		}
		result = std::move(map);
	} else if (node.IsSequence()) {
		Vector<ConfigNode> list;
		for (auto& n : node) {
			list.emplace_back(parseYAMLNode(n));
		}
		result = std::move(list);
	} else if (node.IsScalar()) {
		if (node.Tag().find("binary") != std::string::npos) {
			auto b = node.as<YAML::Binary>();
			Bytes bs;
			bs.resize(b.size());
			memcpy(bs.data(), b.data(), b.size());
			result = std::move(bs);
		} else {
			auto str = String(node.as<std::string>());
			if (str.isNumber()) {
				if (str.isInteger()) {
					result = str.toInteger();
				} else {
					result = str.toFloat();
				}
			} else {
				if (str == "true") {
					result = true;
				} else if (str == "false") {
					result = false;
				} else {
					result = str;
				}
			}
		}
	}

	result.setOriginalPosition(node.Mark().line, node.Mark().column);
	return result;
}

void YAMLConvert::parseConfig(ConfigFile& config, gsl::span<const gsl::byte> data)
{
	String strData(reinterpret_cast<const char*>(data.data()), data.size());
	config.getRoot() = parseConfig(strData);
}

ConfigFile YAMLConvert::parseConfig(gsl::span<const gsl::byte> data)
{
	ConfigFile config;
	parseConfig(config, data);
	return config;
}

ConfigFile YAMLConvert::parseConfig(const Bytes& data)
{
	return parseConfig(gsl::as_bytes(gsl::span<const Byte>(data.data(), data.size())));
}

ConfigNode YAMLConvert::parseConfig(const String& str)
{
	YAML::Node root = YAML::Load(str.cppStr());
	return parseYAMLNode(root);
}

ConfigFile YAMLConvert::parseConfig(const Path& path)
{
	const auto data = Path::readFile(path);
	return parseConfig(data);
}

String YAMLConvert::generateYAML(const ConfigFile& config, const EmitOptions& options)
{
	return generateYAML(config.getRoot(), options);
}

String YAMLConvert::generateYAML(const ConfigNode& node, const EmitOptions& options)
{
	YAML::Emitter emitter;
	emitNode(node, emitter, options);

	if (!emitter.good()) {
		throw Exception("Error generating YAML: " + emitter.GetLastError(), HalleyExceptions::Tools);
	}
	return emitter.c_str();
}

void YAMLConvert::emitNode(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options)
{
	switch (node.getType()) {
	case ConfigNodeType::Int:
		emitter << node.asInt();
		return;

	case ConfigNodeType::Int64:
		emitter << node.asInt64();
		return;

	case ConfigNodeType::Bool:
		emitter << node.asBool();
		return;

	case ConfigNodeType::Int2:
		{
			const auto vec = node.asVector2i();
			emitter << YAML::Flow << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
		}
		return;

	case ConfigNodeType::Float:
		emitter << node.asFloat();
		return;

	case ConfigNodeType::Float2:
		{
			const auto vec = node.asVector2f();
			emitter << YAML::Flow << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
		}
		return;

	case ConfigNodeType::Sequence:
	case ConfigNodeType::DeltaSequence:
		emitSequence(node, emitter, options);
		return;

	case ConfigNodeType::Map:
	case ConfigNodeType::DeltaMap:
		emitMap(node, emitter, options);
		return;

	case ConfigNodeType::String:
		emitter << node.asString().cppStr();
		return;

	case ConfigNodeType::Bytes:
		emitter << YAML::Binary(node.asBytes().data(), node.asBytes().size());
		return;

	case ConfigNodeType::Noop:
		emitter << "<noop>";
		return;

	case ConfigNodeType::Del:
		emitter << "<del>";
		return;

	default:
		emitter << YAML::Null;
	}
}

void YAMLConvert::emitSequence(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options)
{
	if (isCompactSequence(node, 0, options)) {
		emitter << YAML::Flow;
	}

	emitter << YAML::BeginSeq;
	for (auto& n: node.asSequence()) {
		emitNode(n, emitter, options);
	}
	emitter << YAML::EndSeq;
}

void YAMLConvert::emitMap(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options)
{
	const auto& map = node.asMap();

	// Sort entries by key, using options
	Vector<String> keys;
	keys.reserve(map.size());
	for (auto& kv: map) {
		if (kv.second.getType() != ConfigNodeType::Undefined) {
			keys.push_back(kv.first);
		}
	}

	std::sort(keys.begin(), keys.end(), [&] (const String& a, const String& b)
	{
		const auto& mko = options.mapKeyOrder;
		const auto idxA = std::find(mko.begin(), mko.end(), a);
		const auto idxB = std::find(mko.begin(), mko.end(), b);
		if (idxA != idxB) {
			return idxA < idxB;
		}
		return a < b;
	});

	if (isCompactSequence(node, 0, options)) {
		emitter << YAML::Flow;
	}
	emitter << YAML::BeginMap;

	// Emit keys in order
	for (const auto& k: keys) {
		const auto iter = map.find(k);
		emitter << YAML::Key << k;
		emitter << YAML::Value;
		emitNode(iter->second, emitter, options);
	}
	
	emitter << YAML::EndMap;
}

bool YAMLConvert::isCompactSequence(const ConfigNode& node, int depth, const EmitOptions& options)
{
	bool ok = true;

	switch (node.getType()) {
	case ConfigNodeType::Map:
		if (node.asMap().empty()) {
			return true;
		}
		if (!options.compactMaps) {
			return false;
		}
		if (depth >= 2) {
			return false;
		}
		for (auto& [k, v]: node.asMap()) {
			ok = ok && isCompactSequence(v, depth + 1, options);
		}
		return ok;

	case ConfigNodeType::Int:
	case ConfigNodeType::Bool:
	case ConfigNodeType::Float:
	case ConfigNodeType::String:
		return true;

	case ConfigNodeType::Int2:
	case ConfigNodeType::Float2:
		return depth <= 2;

	case ConfigNodeType::Sequence:
		if (depth >= 2) {
			return false;
		}
		for (auto& n: node.asSequence()) {
			ok = ok && isCompactSequence(n, depth + 1, options);
		}
		return ok;
	}

	return false;
}
