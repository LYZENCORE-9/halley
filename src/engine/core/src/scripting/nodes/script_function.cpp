#include "script_function.h"
using namespace Halley;


String ScriptFunctionCallExternal::getIconName(const BaseGraphNode& node) const
{
	auto icon = node.getSettings().getType() == ConfigNodeType::Map ? node.getSettings()["icon"].asString("") : "";
	if (icon.isEmpty()) {
		return "script_icons/function_call.png";
	} else {
		return icon;
	}
}

Vector<IScriptNodeType::SettingType> ScriptFunctionCallExternal::getSettingTypes() const
{
	return {
		SettingType{ "function", "Halley::ResourceReference<Halley::ScriptGraph>", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptFunctionCallExternal::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Call ");
	str.append(node.getSettings()["function"].asString(""), settingColour);
	return str.moveResults();
}

gsl::span<const IScriptNodeType::PinType> ScriptFunctionCallExternal::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;

	auto& settings = node.getSettings();

	const size_t nOutput = settings["nOutput"].asInt(1);
	const size_t nDataInput = settings["nDataInput"].asInt(0);
	const size_t nTargetInput = settings["nTargetInput"].asInt(0);
	const size_t nDataOutput = settings["nDataOutput"].asInt(0);
	const size_t nTargetOutput = settings["nTargetOutput"].asInt(0);

	static thread_local Vector<PinType> pins;
	pins.clear();
	pins.reserve(16);

	pins.emplace_back(ET::FlowPin, PD::Input);
	for (size_t i = 0; i < nDataInput; ++i) {
		pins.emplace_back(ET::ReadDataPin, PD::Input);
	}
	for (size_t i = 0; i < nTargetInput; ++i) {
		pins.emplace_back(ET::TargetPin, PD::Input);
	}

	for (size_t i = 0; i < nOutput; ++i) {
		pins.emplace_back(ET::FlowPin, PD::Output);
	}
	for (size_t i = 0; i < nDataOutput; ++i) {
		pins.emplace_back(ET::ReadDataPin, PD::Output);
	}
	for (size_t i = 0; i < nTargetOutput; ++i) {
		pins.emplace_back(ET::TargetPin, PD::Output);
	}

	return pins;
}

String ScriptFunctionCallExternal::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx >= 1) {
		const auto numInput = getNumOfInputPins(node);
		const char* key;
		size_t idx;
		if (elementIdx < numInput) {
			idx = elementIdx - 1;
			key = "inputNames";
		} else {
			idx = elementIdx - numInput;
			key = "outputNames";
		}

		const auto& seq = node.getSettings()[key];
		if (seq.getType() == ConfigNodeType::Sequence && seq.getSequenceSize() > idx) {
			auto name = seq.asSequence().at(idx).asString("");
			if (!name.isEmpty()) {
				return name;
			}
		}
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

String ScriptFunctionCallExternal::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getPinDescription(node, ScriptNodeElementType::ReadDataPin, elementIdx);
}

void ScriptFunctionCallExternal::updateSettings(BaseGraphNode& node, const BaseGraph& graph, Resources& resources) const
{
	auto& settings = node.getSettings();
	const auto& functionName = settings["function"].asString("");
	if (functionName.isEmpty() || !resources.exists<ScriptGraph>(functionName)) {
		settings["nOutput"] = 1;
		settings["nDataInput"] = 0;
		settings["nTargetInput"] = 0;
		settings["nDataOutput"] = 0;
		settings["nTargetOutput"] = 0;
		settings.removeKey("inputNames");
		settings.removeKey("outputNames");
		settings.removeKey("icon");
	} else {
		const auto function = resources.get<ScriptGraph>(functionName);
		const auto pars = function->getFunctionParameters();
		settings["nOutput"] = pars.nOutput;
		settings["nDataInput"] = pars.nDataInput;
		settings["nTargetInput"] = pars.nTargetInput;
		settings["nDataOutput"] = pars.nDataOutput;
		settings["nTargetOutput"] = pars.nTargetOutput;
		settings["inputNames"] = pars.inputNames;
		settings["outputNames"] = pars.outputNames;
		settings["icon"] = pars.icon;
	}
}

IScriptNodeType::Result ScriptFunctionCallExternal::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	if (node.getSettings()["function"].asString("").isEmpty()) {
		return Result(ScriptNodeExecutionState::Done);
	} else {
		return Result(ScriptNodeExecutionState::Call);
	}
}

ConfigNode ScriptFunctionCallExternal::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto other = getReturnNodePin(environment, node, pinN);
	if (other) {
		const auto& returnNode = environment.getCurrentGraph()->getNodes()[other->first];
		return environment.readInputDataPin(returnNode, other->second);
	} else {
		return {};
	}
}

EntityId ScriptFunctionCallExternal::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const
{
	const auto other = getReturnNodePin(environment, node, pinN);
	if (other) {
		const auto& returnNode = environment.getCurrentGraph()->getNodes()[other->first];
		return environment.readInputEntityId(returnNode, other->second);
	} else {
		return {};
	}
}

size_t ScriptFunctionCallExternal::getNumOfInputPins(const BaseGraphNode& node) const
{
	const auto& settings = node.getSettings();
	const size_t nDataInput = settings["nDataInput"].asInt(0);
	const size_t nTargetInput = settings["nTargetInput"].asInt(0);
	return 1 + nDataInput + nTargetInput;
}

std::optional<std::pair<GraphNodeId, GraphPinId>> ScriptFunctionCallExternal::getStartNodePin(ScriptEnvironment& environment, const BaseGraphNode& node, size_t pinN) const
{
	// Find the "start" node
	const auto& graph = environment.getCurrentGraph();
	const auto returnNodeId = graph->getCallee(node.getId());
	if (!returnNodeId) {
		return std::nullopt;
	}
	
	return std::pair<GraphNodeId, GraphPinId>{ *returnNodeId, static_cast<GraphPinId>(pinN) };
}

std::optional<std::pair<GraphNodeId, GraphPinId>> ScriptFunctionCallExternal::getReturnNodePin(ScriptEnvironment& environment, const BaseGraphNode& node, size_t pinN) const
{
	// Find the "return" node
	const auto& graph = environment.getCurrentGraph();
	const auto returnNodeId = graph->getReturnFrom(node.getId());
	if (!returnNodeId) {
		return std::nullopt;
	}

	// Find the pin on the return node
	const size_t returnPinN = pinN - getNumOfInputPins(node);

	return std::pair<GraphNodeId, GraphPinId>{ *returnNodeId, static_cast<GraphPinId>(returnPinN) };
}


Vector<IScriptNodeType::SettingType> ScriptFunctionReturn::getSettingTypes() const
{
	return {
		SettingType{ "icon", "Halley::ResourceReference<Halley::SpriteResource>", Vector<String>{""} },
		SettingType{ "flowPins", "Halley::Vector<Halley::String>", Vector<String>{"Output"} },
		SettingType{ "dataPins", "Halley::Vector<Halley::String>", Vector<String>{""} },
		SettingType{ "targetPins", "Halley::Vector<Halley::String>", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptFunctionReturn::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Return to caller");
	return str.moveResults();
}

gsl::span<const IScriptNodeType::PinType> ScriptFunctionReturn::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;

	const size_t nInput = node.getSettings()["flowPins"].getSequenceSize(1);
	const size_t nDataInput = node.getSettings()["dataPins"].getSequenceSize(0);
	const size_t nTargetInput = node.getSettings()["targetPins"].getSequenceSize(0);

	if (nDataInput == 0 && nTargetInput == 0) {
		// Simple, common case
		using ET = ScriptNodeElementType;
		using PD = GraphNodePinDirection;
		const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Input } };
		return gsl::span<const PinType>(data).subspan(0, nInput);
	} else {
		static thread_local Vector<PinType> pins;
		pins.clear();
		pins.reserve(16);

		for (size_t i = 0; i < nInput; ++i) {
			pins.emplace_back(ET::FlowPin, PD::Input);
		}
		for (size_t i = 0; i < nDataInput; ++i) {
			pins.emplace_back(ET::ReadDataPin, PD::Input);
		}
		for (size_t i = 0; i < nTargetInput; ++i) {
			pins.emplace_back(ET::TargetPin, PD::Input);
		}
		return pins;
	}
}

String ScriptFunctionReturn::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	const size_t nInput = node.getSettings()["flowPins"].getSequenceSize(1);
	const size_t nDataInput = node.getSettings()["dataPins"].getSequenceSize(0);
	const size_t nTargetInput = node.getSettings()["targetPins"].getSequenceSize(0);

	size_t idx = 0;
	const char* key = nullptr;

	if (elementIdx < nInput) {
		idx = elementIdx;
		key = "flowPins";
	} else if (elementIdx < nInput + nDataInput) {
		idx = elementIdx - nInput;
		key = "dataPins";
	} else if (elementIdx < nInput + nDataInput + nTargetInput) {
		idx = elementIdx - nInput - nDataInput;
		key = "targetPins";
	}

	if (key) {
		auto name = node.getSettings()[key].asSequence().at(idx).asString();
		if (!name.isEmpty()) {
			return name;
		}
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

IScriptNodeType::Result ScriptFunctionReturn::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const uint8_t inputPinMask = static_cast<uint8_t>(1 << environment.getCurrentInputPin());
	return Result(ScriptNodeExecutionState::Return, 0, inputPinMask);
}
