#include "script_loop.h"

#include "halley/maths/interpolation_curve.h"
#include "halley/maths/tween.h"
#include "halley/support/logger.h"
using namespace Halley;

ScriptForLoopData::ScriptForLoopData(const ConfigNode& node)
{
	iterations = node.asInt(0);
}

ConfigNode ScriptForLoopData::toConfigNode(const EntitySerializationContext& context)
{
	return ConfigNode(iterations);
}

String ScriptForLoop::getLabel(const BaseGraphNode& node) const
{
	return toString(node.getSettings()["loopCount"].asInt(0));
}

String ScriptForLoop::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "Loop Index";
}

gsl::span<const IScriptNodeType::PinType> ScriptForLoop::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Output }
	};
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptForLoop::getSettingTypes() const
{
	return { SettingType{ "loopCount", "int", Vector<String>{"0"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptForLoop::getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const
{
	const int count = node.getSettings()["loopCount"].asInt(0);
	auto str = ColourStringBuilder(true);
	str.append("Loop ");
	str.append(toString(count), settingColour);
	str.append(count == 1 ? " time" : " times");
	return str.moveResults();
}

String ScriptForLoop::getPinDescription(const BaseGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Flow output after loop";
	} else if (elementIdx == 2) {
		return "Flow output for each loop iteration";
	} else if (elementIdx == 3) {
		return "Loop index [0-base]";
	} else {
		return ScriptNodeTypeBase<ScriptForLoopData>::getPinDescription(node, element, elementIdx);
	}
}

IScriptNodeType::Result ScriptForLoop::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptForLoopData& curData) const
{
	const int count = node.getSettings()["loopCount"].asInt(0);
	const bool done = curData.iterations >= count;
	if (!done) {
		++curData.iterations;
	}
	return Result(ScriptNodeExecutionState::Done, 0, done ? 1 : 2);
}

ConfigNode ScriptForLoop::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptForLoopData& curData) const
{
	return ConfigNode(curData.iterations - 1);
}

void ScriptForLoop::doInitData(ScriptForLoopData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data.iterations = 0;
}

bool ScriptForLoop::doIsStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId outPin, ScriptForLoopData& curData) const
{
	return outPin == 2;
}

bool ScriptForLoop::canKeepData() const
{
	return true;
}


gsl::span<const IScriptNodeType::PinType> ScriptWhileLoop::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptWhileLoop::getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	if (node.getPin(1).hasConnection()) {
		const auto desc = getConnectedNodeName(world, node, graph, 1);
		str.append("Loop as long as ");
		str.append(desc, parameterColour);
		str.append(" is true");
	} else {
		str.append("Loop ");
		str.append("forever", settingColour);
	}
	return str.moveResults();
}

IScriptNodeType::Result ScriptWhileLoop::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const bool condition = readDataPin(environment, node, 1).asBool(true);
	return Result(ScriptNodeExecutionState::Done, 0, condition ? 2 : 1);
}

bool ScriptWhileLoop::doIsStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId outPin) const
{
	return outPin == 3;
}



ScriptForEachLoopData::ScriptForEachLoopData(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		iterations = node["iterations"].asInt(0);
		seq = node["seq"].asSequence();
	}
}

ConfigNode ScriptForEachLoopData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["iterations"] = iterations;
	result["seq"] = seq;
	return result;
}

String ScriptForEachLoop::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "curLoopElement";
}

gsl::span<const IGraphNodeType::PinType> ScriptForEachLoop::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 5>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output },
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptForEachLoop::getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Loop for each element in ");
	str.append(getConnectedNodeName(world, node, graph, 3), parameterColour);
	return str.moveResults();
}

String ScriptForEachLoop::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Flow after loop";
	} else if (elementIdx == 2) {
		return "Flow for each element";
	} else if (elementIdx == 3) {
		return "List";
	} else if (elementIdx == 4) {
		return "Current Element";
	}
	return ScriptNodeTypeBase<ScriptForEachLoopData>::getPinDescription(node, elementType, elementIdx);
}

void ScriptForEachLoop::doInitData(ScriptForEachLoopData& data, const ScriptGraphNode& node, const EntitySerializationContext& context,	const ConfigNode& nodeData) const
{
	data = ScriptForEachLoopData(nodeData);
}

bool ScriptForEachLoop::doIsStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId outPin, ScriptForEachLoopData& curData) const
{
	return outPin == 2;
}

bool ScriptForEachLoop::canKeepData() const
{
	return true;
}

IScriptNodeType::Result ScriptForEachLoop::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptForEachLoopData& curData) const
{
	if (curData.iterations == 0) {
		auto data = readDataPin(environment, node, 3);
		if (data.getType() == ConfigNodeType::Sequence) {
			curData.seq = data.asSequence();
		} else {
			curData.seq = {};
		}
	}

	const int count = static_cast<int>(curData.seq.size());
	const bool done = curData.iterations >= count;
	if (!done) {
		++curData.iterations;
	}
	return Result(ScriptNodeExecutionState::Done, 0, done ? 1 : 2);
}

ConfigNode ScriptForEachLoop::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptForEachLoopData& curData) const
{
	return ConfigNode(curData.seq[curData.iterations - 1]);
}


ScriptLerpLoopData::ScriptLerpLoopData(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		time = node["time"].asFloat(0.0f);
		state = node["state"].asInt(0);
	} else if (node.getType() != ConfigNodeType::Undefined) {
		time = node.asFloat();
	}
}

ConfigNode ScriptLerpLoopData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["time"] = time;
	result["state"] = state;
	return result;
}

Vector<IScriptNodeType::SettingType> ScriptLerpLoop::getSettingTypes() const
{
	return {
		SettingType{ "time", "float", Vector<String>{"1"} },
		SettingType{ "curve", "Halley::InterpolationCurveLerp", Vector<String>{"linear"} }
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptLerpLoop::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 5>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Input } };
	return data;
}

String ScriptLerpLoop::getLabel(const BaseGraphNode& node) const
{
	if (node.getPin(4).hasConnection()) {
		return "";
	}
	return toString(node.getSettings()["time"].asFloat(1)) + "s";
}

std::pair<String, Vector<ColourOverride>> ScriptLerpLoop::getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Loop over ");

	if (node.getPin(4).hasConnection()) {
		str.append(getConnectedNodeName(world, node, graph, 4), parameterColour);
	}
	else {
		str.append(toString(node.getSettings()["time"].asFloat(1)) + "s", settingColour);
	}

	str.append(" whilst outputting from 0 to 1");
	return str.moveResults();}

String ScriptLerpLoop::getPinDescription(const BaseGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Flow output after loop";
	} else if (elementIdx == 2) {
		return "Flow output for each loop iteration";
	} else if (elementIdx == 3) {
		return "Loop progress (0..1)";
	} else if (elementIdx == 4) {
		return "Time";
	} else {
		return ScriptNodeTypeBase<ScriptLerpLoopData>::getPinDescription(node, element, elementIdx);
	}
}

String ScriptLerpLoop::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId element_idx) const
{
	return "t";
}

void ScriptLerpLoop::doInitData(ScriptLerpLoopData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data = ScriptLerpLoopData(nodeData);
}

IScriptNodeType::Result ScriptLerpLoop::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptLerpLoopData& curData) const
{
	if (curData.state == static_cast<uint8_t>(ScriptLerpLoopData::State::Done)) {
		curData.state = static_cast<uint8_t>(ScriptLerpLoopData::State::Idle);
		return Result(ScriptNodeExecutionState::Done, 0, 1);
	}

	// Important: check for done before incrementing time. This makes sure that we had at least one iteration outputting 1.0f before terminating
	// If time is not set at all, then it's the first iteration, which we'll report as consuming 0 seconds, this is to ensure we also get a proper instant first iteration

	float length = node.getSettings()["time"].asFloat(1);
	if (node.getPin(4).hasConnection()) {
		length = readDataPin(environment, node, 4).asFloat();
	}

	const bool firstRun = curData.state == static_cast<uint8_t>(ScriptLerpLoopData::State::Idle);
	if (firstRun) {
		curData.state = static_cast<uint8_t>(ScriptLerpLoopData::State::Running);
		curData.time = 0.0f;
	}
	const bool done = curData.time >= length;

	const float timeLeft = std::max(length - curData.time, 0.0f);
	curData.time += static_cast<float>(time);

	if (done) {
		curData.state = static_cast<uint8_t>(ScriptLerpLoopData::State::Done);
	}

	return Result(ScriptNodeExecutionState::Done, firstRun ? 0 : std::min(static_cast<Time>(timeLeft), time), 2);
}

ConfigNode ScriptLerpLoop::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptLerpLoopData& curData) const
{
	float length = node.getSettings()["time"].asFloat(1);
	if (node.getPin(4).hasConnection()) {
		length = readDataPin(environment, node, 4).asFloat();
	}

	const auto curve = InterpolationCurve(node.getSettings()["curve"], true);
	return ConfigNode(curve.evaluate(clamp(curData.time / length, 0.0f, 1.0f)));
}

bool ScriptLerpLoop::doIsStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId outPin, ScriptLerpLoopData& curData) const
{
	return outPin == 2;
}



String ScriptWhileLoop::getPinDescription(const BaseGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Condition";
	} else if (elementIdx == 2) {
		return "Flow output after loop";
	} else if (elementIdx == 3) {
		return "Flow output for each loop iteration";
	} else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, element, elementIdx);
	}
}




ScriptEveryFrameData::ScriptEveryFrameData(const ConfigNode& node)
{
	lastFrameN = node["lastFrameN"].asInt();
}

ConfigNode ScriptEveryFrameData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["lastFrameN"] = lastFrameN;
	return result;
}

gsl::span<const IScriptNodeType::PinType> ScriptEveryFrame::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptEveryFrame::getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Pulse every ");
	str.append("frame", settingColour);
	return str.moveResults();
}

String ScriptEveryFrame::getPinDescription(const BaseGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Frame delta time";
	} else {
		return ScriptNodeTypeBase<ScriptEveryFrameData>::getPinDescription(node, element, elementIdx);
	}
}

String ScriptEveryFrame::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "Frame delta time";
}

IScriptNodeType::Result ScriptEveryFrame::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptEveryFrameData& curData) const
{
	const auto curFrame = environment.getCurrentFrameNumber();
	if (curFrame != curData.lastFrameN) {
		curData.lastFrameN = curFrame;
		return Result(ScriptNodeExecutionState::Fork, 0, 1);
	} else {
		return Result(ScriptNodeExecutionState::Executing, time);
	}
}

ConfigNode ScriptEveryFrame::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptEveryFrameData& curData) const
{
	return ConfigNode(static_cast<float>(environment.getDeltaTime()));
}

void ScriptEveryFrame::doInitData(ScriptEveryFrameData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data.lastFrameN = -1;
}

bool ScriptEveryFrame::canKeepData() const
{
	return true;
}



ScriptEveryTimeData::ScriptEveryTimeData(const ConfigNode& node)
{
	time = node["time"].asFloat(0);
}

ConfigNode ScriptEveryTimeData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["time"] = time;
	return result;
}

String ScriptEveryTime::getLabel(const BaseGraphNode& node) const
{
	return toString(node.getSettings()["time"].asFloat(1.0f)) + " s";
}

Vector<IScriptNodeType::SettingType> ScriptEveryTime::getSettingTypes() const
{
	return { SettingType{ "time", "float", Vector<String>{"1"} } };
}

gsl::span<const IScriptNodeType::PinType> ScriptEveryTime::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptEveryTime::getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Pulse every ");
	str.append(toString(node.getSettings()["time"].asFloat(1.0f)), settingColour);
	str.append(" s");
	return str.moveResults();
}

IScriptNodeType::Result ScriptEveryTime::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptEveryTimeData& curData) const
{
	const float period = node.getSettings()["time"].asFloat(1.0f);
	const float timeToNextPulse = period - curData.time;
	if (timeToNextPulse < static_cast<float>(time)) {
		curData.time = 0;
		return Result(ScriptNodeExecutionState::Fork, static_cast<Time>(timeToNextPulse), 1);
	} else {
		curData.time += static_cast<float>(time);
		return Result(ScriptNodeExecutionState::Executing, time);
	}
}

void ScriptEveryTime::doInitData(ScriptEveryTimeData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data.time = node.getSettings()["time"].asFloat(1.0f);
}

bool ScriptEveryTime::canKeepData() const
{
	return true;
}
