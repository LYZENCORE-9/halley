#pragma once
#include "halley/data_structures/hash_map.h"
#include "halley/file/directory_monitor.h"
#include "halley/file/path.h"
#include "halley/maths/uuid.h"
#include "halley/maths/vector2.h"
#include "halley/time/halleytime.h"

namespace Halley {
    enum class ProjectCommentPriority {
        Note,
	    Low,
        Medium,
        High
    };

	template <>
	struct EnumNames<ProjectCommentPriority> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
                "note",
				"low",
                "medium",
                "high"
			}};
		}
	};

    enum class ProjectCommentCategory {
	    Misc,
        Art,
        Implementation,
        Music,
        Sound,
        Writing
    };

	template <>
	struct EnumNames<ProjectCommentCategory> {
		constexpr std::array<const char*, 6> operator()() const {
			return{{
				"misc",
                "art",
                "implementation",
                "music",
                "sound",
                "writing"
			}};
		}
	};

    class ProjectComment {
    public:
        Vector2f pos;
        String text;
        String scene;
        ProjectCommentCategory category = ProjectCommentCategory::Misc;
        ProjectCommentPriority priority = ProjectCommentPriority::Note;

        explicit ProjectComment(Vector2f pos = {}, String scene = "", ProjectCommentCategory category = ProjectCommentCategory::Misc, ProjectCommentPriority priority = ProjectCommentPriority::Note);
        ProjectComment(const ConfigNode& node);
        ConfigNode toConfigNode() const;

        bool operator==(const ProjectComment& other) const;
        bool operator!=(const ProjectComment& other) const;
    };

    class ProjectComments {
    public:
        ProjectComments(Path commentsRoot);

        Vector<std::pair<UUID, const ProjectComment*>> getComments(const String& scene) const;
        const ProjectComment& getComment(const UUID& id) const;

        UUID addComment(ProjectComment comment);
        void deleteComment(const UUID& id);
        void setComment(const UUID& id, ProjectComment comment);
        void updateComment(const UUID& id, std::function<void(ProjectComment&)> f, bool immediate = false);
        void exportAll(std::optional<ProjectCommentCategory> category, const Path& path);

        uint64_t getVersion() const;
        void update(Time t);

    private:
        Path commentsRoot;
        DirectoryMonitor monitor;   // Important, this needs to be after commentsRoot, as it has dependent initialization
        Time monitorTime = 0;
        Time saveTimeout = 0;

        HashMap<UUID, ProjectComment> comments;
        uint64_t version = 0;
        HashSet<UUID> toSave;
        HashMap<UUID, uint64_t> lastSeenHash;

        void loadAll();
        void savePending();
        void saveFile(const UUID& id, const ProjectComment& comment);
        bool loadFile(const UUID& id, const Path& path);
        void deleteFile(const UUID& id);

        Path getPath(const UUID& id) const;
        UUID getUUID(const Path& path) const;
    };
}
