#pragma once
#include "halley/utils/utils.h"
#include "halley/text/halleystring.h"
#include <memory>
#include <gsl/span>
#include "halley/resources/resource_data.h"

namespace Halley {
	enum class AssetType;
	class Deserializer;
	class Serializer;
	class AssetDatabase;
	class ResourceData;
	class ResourceDataReader;

	struct AssetPackHeader {
		std::array<char, 8> identifier;
		std::array<char, 16> iv;
		uint64_t assetDbStartPos;
		uint64_t dataStartPos;

		void init(size_t assetDbSize);
	};

    class AssetPack {
    public:
		AssetPack();
		AssetPack(const AssetPack& other) = delete;
		AssetPack(AssetPack&& other) noexcept;
		AssetPack(std::unique_ptr<ResourceDataReader> reader, const String& encryptionKey = "", bool preLoad = false);
		~AssetPack();

		AssetPack& operator=(const AssetPack& other) = delete;
		AssetPack& operator=(AssetPack&& other) noexcept;

		AssetDatabase& getAssetDatabase();
		const AssetDatabase& getAssetDatabase() const;
		Bytes& getData();
		const Bytes& getData() const;

		Bytes writeOut() const;

		std::unique_ptr<ResourceData> getData(const String& asset, AssetType type, bool stream);

		void readToMemory();
		void encrypt(const String& key);
		void decrypt(const String& key);
	    
    	void readData(size_t pos, gsl::span<gsl::byte> dst);

		std::unique_ptr<ResourceDataReader> extractReader();

		std::shared_ptr<bool> getAliveToken() const;

		size_t getMemoryUsage() const;

    private:
		std::unique_ptr<AssetDatabase> assetDb;
		std::unique_ptr<ResourceDataReader> reader;
		std::atomic<bool> hasReader;
		std::mutex readerMutex;
		size_t dataOffset = 0;
		Bytes data;
		std::array<char, 16> iv;
		mutable std::shared_ptr<bool> aliveToken;
    };


	class PackDataReader final : public ResourceDataReader {
	public:
		PackDataReader(AssetPack& pack, size_t startPos, size_t fileSize);

		size_t size() const override;
		int read(gsl::span<gsl::byte> dst) override;
		void seek(int64_t pos, int whence) override;
		size_t tell() const override;
		void close() override;
		bool isAvailable() const override;

	private:
		AssetPack& pack;
		const size_t startPos;
		const size_t fileSize;
		size_t curPos = 0;
		mutable std::mutex mutex;
		std::shared_ptr<bool> aliveToken;
	};
}
