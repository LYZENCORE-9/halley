#pragma once

#include "fields_schema.h"
#include "halley/data_structures/hash_map.h"
#include "halley/data_structures/maybe.h"

namespace YAML
{
	class Node;
}

namespace Halley
{
	class ComponentSchema
	{
	public:
		ComponentSchema();
		explicit ComponentSchema(YAML::Node node, bool generate);

		int id = -1;
		String name;
		String category;
		Vector<ComponentFieldSchema> members;
		HashSet<String> includeFiles;
		std::optional<String> customImplementation;
		Vector<String> componentDependencies;
		Vector<String> componentDependenciesInAncestors;
		bool generate = false;

		bool operator<(const ComponentSchema& other) const;
	};
}