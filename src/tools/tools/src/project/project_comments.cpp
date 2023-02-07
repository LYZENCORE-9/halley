#include "halley/tools/project/project_comments.h"

#include "halley/data_structures/config_node.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/support/logger.h"
#include "halley/tools/file/filesystem.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

ProjectComment::ProjectComment(Vector2f pos, String scene)
	: pos(pos)
	, scene(std::move(scene))
{
}

ProjectComment::ProjectComment(const ConfigNode& node)
{
	pos = node["pos"].asVector2f({});
	text = node["text"].asString({});
	scene = node["scene"].asString({});
	priority = node["priority"].asEnum<ProjectCommentPriority>({});
}

ConfigNode ProjectComment::toConfigNode() const
{
	ConfigNode::MapType result;
	result["pos"] = pos;
	result["text"] = text;
	result["scene"] = scene;
	result["priority"] = priority;
	return result;
}

bool ProjectComment::operator==(const ProjectComment& other) const
{
	return pos == other.pos && text == other.text && priority == other.priority;
}

bool ProjectComment::operator!=(const ProjectComment& other) const
{
	return !(*this == other);
}

ProjectComments::ProjectComments(Path commentsRoot)
	: commentsRoot(std::move(commentsRoot))
	, monitor(commentsRoot)
{
	loadAll();
}

Vector<std::pair<UUID, const ProjectComment*>> ProjectComments::getComments(const String& scene) const
{
	Vector<std::pair<UUID, const ProjectComment*>> result;
	for (auto& [k, v]: comments) {
		if (v.scene == scene) {
			result.emplace_back(k, &v);
		}
	}
	return result;
}

const ProjectComment& ProjectComments::getComment(const UUID& id) const
{
	return comments.at(id);
}

UUID ProjectComments::addComment(ProjectComment comment)
{
	auto id = UUID::generate();
	setComment(id, std::move(comment));
	return id;
}

void ProjectComments::deleteComment(const UUID& id)
{
	deleteFile(id);
	comments.erase(id);

	std_ex::erase(toSave, id);
}

void ProjectComments::setComment(const UUID& id, ProjectComment comment)
{
	saveFile(id, comment);
	comments[id] = std::move(comment);
}

void ProjectComments::updateComment(const UUID& id, std::function<void(ProjectComment&)> f, bool immediate)
{
	auto& comment = comments[id];
	f(comment);

	if (immediate) {
		saveFile(id, comment);
	} else {
		toSave.push_back(id);
	}
}

uint64_t ProjectComments::getVersion() const
{
	return version;
}

void ProjectComments::update(Time t)
{
	monitorTime += t;
	const auto threshold = monitor.hasRealImplementation() ? 0.5 : 2.0;

	if (monitorTime > threshold) {
		monitorTime = 0;
		if (monitor.poll()) {
			loadAll();
		}
	}

	if (toSave.empty()) {
		saveTimeout = 0;
	} else {
		saveTimeout += t;
		if (saveTimeout > 1.0) {
			savePending();
			saveTimeout = 0;
		}
	}
}

void ProjectComments::loadAll()
{
	HashSet<UUID> present;

	bool modified = false;
	for (const auto& file: FileSystem::enumerateDirectory(commentsRoot)) {
		const auto path = commentsRoot / file;
		const auto uuid = getUUID(path);
		if (uuid.isValid()) {
			modified = loadFile(uuid, path) || modified;
			present.emplace(uuid);
		}
	}

	const auto nRemoved = std_ex::erase_if_key(comments, [&](const UUID& key)
	{
		return !present.contains(key);
	});

	if (modified || nRemoved > 0) {
		++version;
		Logger::logDev("Comments loaded from disk");
	}
}

void ProjectComments::savePending()
{
	for (auto& id: toSave) {
		saveFile(id, comments.at(id));
	}
	toSave.clear();
}

void ProjectComments::saveFile(const UUID& id, const ProjectComment& comment)
{
	const auto path = getPath(id);
	FileSystem::createParentDir(path);

	YAMLConvert::EmitOptions options;
	Path::writeFile(path, YAMLConvert::generateYAML(comment.toConfigNode(), options));
	//++version; // Don't modify on save, causes issues with editor

	monitorTime = -2; // At least 2 seconds before attempting to read dir
}

bool ProjectComments::loadFile(const UUID& uuid, const Path& path)
{
	if (uuid.isValid()) {
		const auto configFile = YAMLConvert::parseConfig(Path::readFile(path));
		auto comment = ProjectComment(configFile.getRoot());

		auto iter = comments.find(uuid);
		if (iter != comments.end()) {
			if (iter->second != comment) {
				iter->second = std::move(comment);
				return true;
			}
		} else {
			comments[uuid] = std::move(comment);
			return true;
		}
	}
	return false;
}

void ProjectComments::deleteFile(const UUID& id)
{
	Path::removeFile(getPath(id));
	++version;
}

Path ProjectComments::getPath(const UUID& id) const
{
	return commentsRoot / ("comment_" + id.toString() + ".yaml");
}

UUID ProjectComments::getUUID(const Path& path) const
{
	const auto filename = path.getFilename().replaceExtension("").toString();
	if (filename.startsWith("comment_") && filename.length() == 44) {
		return UUID(filename.substr(8));
	}
	return UUID();
}
