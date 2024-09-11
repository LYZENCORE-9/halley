#include "resource_filesystem.h"
#include "halley/resources/resource_locator.h"
#include <halley/support/exception.h>
#include "resource_pack.h"
#include "halley/support/logger.h"
#include "halley/api/system_api.h"
#include "halley/text/string_converter.h"
#include "halley/resources/resource.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

ResourceLocator::ResourceLocator(SystemAPI& system)
	: system(system)
{
}

void ResourceLocator::add(std::unique_ptr<IResourceLocatorProvider> locator, const Path& path)
{
	loadLocatorData(*locator);
	locatorPaths[path.getString()] = locator.get();
	locators.emplace_back(std::move(locator));
}

void ResourceLocator::loadLocatorData(IResourceLocatorProvider& locator)
{
	auto& db = locator.getAssetDatabase();
	for (auto& asset: db.getAssets()) {
		auto result = assetToLocator.find(asset);
		if (result == assetToLocator.end() || result->second->getPriority() < locator.getPriority()) {
			assetToLocator[asset] = &locator;
		}
	}
}

void ResourceLocator::purge(const String& asset, AssetType type)
{
	auto result = assetToLocator.find(toString(type) + ":" + asset);
	if (result != assetToLocator.end()) {
		// Found the locator for this file, purge it
		result->second->purgeAll(system);
	} else {
		// Couldn't find a locator (new file?), purge everything
		purgeAll();
	}
}

void ResourceLocator::purgeAll()
{
	assetToLocator.clear();
	for (auto& locator: locators) {
		locator->purgeAll(system);
		loadLocatorData(*locator);
	}
}

void ResourceLocator::purgePacks(gsl::span<const String> assetIds, gsl::span<const String> packs)
{
	for (auto& locator: locators) {
		const bool affected = locator->purgeIfAffected(system, assetIds, packs);
		if (affected) {
			std_ex::erase_if_value(assetToLocator, [&] (IResourceLocatorProvider* v) { return v == locator.get(); });
			loadLocatorData(*locator);
		}
	}
}

std::unique_ptr<ResourceData> ResourceLocator::getResource(const String& asset, AssetType type, bool stream, bool throwOnFail) const
{
	auto result = assetToLocator.find(toString(type) + ":" + asset);
	if (result != assetToLocator.end()) {
		auto data = result->second->getData(asset, type, stream);
		if (data) {
			return data;
		} else if (throwOnFail) {
			throw Exception("Unable to load resource: " + toString(type) + ":" + asset, HalleyExceptions::Resources);
		} else {
			return {};
		}
	} else if (throwOnFail) {
		throw Exception("Unable to locate resource: " + toString(type) + ":" + asset, HalleyExceptions::Resources);
	} else {
		//Logger::logError("Unable to locate resource: " + toString(type) + ":" + asset);
		return {};
	}
}

std::unique_ptr<ResourceDataStatic> ResourceLocator::getStatic(const String& asset, AssetType type, bool throwOnFail)
{
	auto rawPtr = getResource(asset, type, false, throwOnFail).release();
	if (!rawPtr) {
		return std::unique_ptr<ResourceDataStatic>();
	}
	
	auto ptr = dynamic_cast<ResourceDataStatic*>(rawPtr);
	if (!ptr) {
		delete rawPtr;
		throw Exception("Resource " + asset + " obtained, but is not static data.", HalleyExceptions::Resources);
	}
	return std::unique_ptr<ResourceDataStatic>(ptr);
}

std::unique_ptr<ResourceDataStream> ResourceLocator::getStream(const String& asset, AssetType type, bool throwOnFail)
{
	auto rawPtr = getResource(asset, type, true, throwOnFail).release();
	if (!rawPtr) {
		return std::unique_ptr<ResourceDataStream>();
	}
	
	auto ptr = dynamic_cast<ResourceDataStream*>(rawPtr);
	if (!ptr) {
		delete rawPtr;
		throw Exception("Resource " + asset + " obtained, but is not stream data.", HalleyExceptions::Resources);
	}
	return std::unique_ptr<ResourceDataStream>(ptr);
}

Vector<String> ResourceLocator::enumerate(const AssetType type)
{
	Vector<String> result;
	for (auto& l: locators) {
		for (auto& r: l->getAssetDatabase().enumerate(type)) {
			result.push_back(std::move(r));
		}
	}
	return result;
}

void ResourceLocator::addFileSystem(const Path& path, IFileSystemCache* cache)
{
	add(std::make_unique<FileSystemResourceLocator>(system, path, cache), path);
}

void ResourceLocator::addPack(const Path& path, std::optional<Encrypt::AESKey> encryptionKey, bool preLoad, bool allowFailure, std::optional<int> priority)
{
	auto dataReader = system.getDataReader(path.string());
	if (dataReader) {
		auto resourceLocator = std::make_unique<PackResourceLocator>(std::move(dataReader), path, encryptionKey, preLoad, priority);
		add(std::move(resourceLocator), path);
	} else {
		if (allowFailure) {
			Logger::logWarning("Resource pack not found: \"" + path.string() + "\"");
		} else {
			throw Exception("Unable to load resource pack \"" + path.string() + "\"", HalleyExceptions::Resources);
		}
	}
}

void ResourceLocator::removePack(const Path& path)
{
	auto* locatorToRemove = locatorPaths.find(path.getString())->second;
	auto& dbToRemove = locatorToRemove->getAssetDatabase();
	for (auto& asset : dbToRemove.getAssets()) {
		auto result = assetToLocator.find(asset);
		if (result != assetToLocator.end()) {
			assetToLocator.erase(asset);
		}
	}
	auto locaterIter = std::find_if(locators.begin(), locators.end(), [&](std::unique_ptr<IResourceLocatorProvider>& locator) { return locator.get() == locatorToRemove; });
	locators.erase(locaterIter);
	
	for (const auto& locator : locators)
	{
		auto& db = locator->getAssetDatabase();
		for (auto& asset : db.getAssets()) {
			auto result = assetToLocator.find(asset);
			if (result == assetToLocator.end() || result->second->getPriority() < locator->getPriority()) {
				assetToLocator[asset] = locator.get();
			}
		}
	}
}

Vector<String> ResourceLocator::getAssetsFromPack(const Path& path, std::optional<Encrypt::AESKey> encryptionKey) const
{
	auto dataReader = system.getDataReader(path.string());
	if (dataReader) {
		std::unique_ptr<IResourceLocatorProvider> resourceLocator = std::make_unique<PackResourceLocator>(std::move(dataReader), path, encryptionKey, true);
		auto& db = resourceLocator->getAssetDatabase();
		return db.getAssets();
	}
	else {
		throw Exception("Unable to load resource pack \"" + path.string() + "\"", HalleyExceptions::Resources);
	}
}

const Metadata* ResourceLocator::getMetaData(const String& asset, AssetType type) const
{
	auto result = assetToLocator.find(toString(type) + ":" + asset);
	if (result != assetToLocator.end()) {
		return &result->second->getAssetDatabase().getDatabase(type).get(asset).meta;
	} else {
		return &dummyMetadata;
	}
}

bool ResourceLocator::exists(const String& asset, AssetType type)
{
	return assetToLocator.find(toString(type) + ":" + asset) != assetToLocator.end();
}

size_t ResourceLocator::getLocatorCount() const
{
	return assetToLocator.size();
}

void ResourceLocator::generateMemoryReport() const
{
	size_t totalSize = 0;
	std::map<String, size_t> sizes;
	for (const auto& locator: locators) {
		const auto size = locator->getMemoryUsage();
		totalSize += size;
		sizes[locator->getName()] = size;
	}

	Logger::logInfo("Resource locator memory usage: " + String::prettySize(totalSize));
	for (const auto& [k, v]: sizes) {
		Logger::logInfo("\t" + k + ": " + String::prettySize(v));
	}
}

const Metadata ResourceLocator::dummyMetadata;
