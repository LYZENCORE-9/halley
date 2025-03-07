#pragma once
#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"
#include "halley/api/audio_api.h"
#include "audio_buffer.h"

namespace Halley
{
	class AudioBuffersRef;
	class AudioBufferPool;
	class ResourceLoader;
	class VorbisData;

	class IAudioClip
	{
	public:
		virtual ~IAudioClip() = default;

		virtual String getName() const { return ""; }
		virtual size_t copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const = 0;
		virtual uint8_t getNumberOfChannels() const = 0;
		virtual size_t getLength() const = 0; // in samples
		virtual size_t getLoopPoint() const { return 0; } // in samples
		virtual bool isLoaded() const { return true; }
	};

	class AudioClip final : public AsyncResource, public IAudioClip
	{
	public:
		AudioClip(uint8_t numChannels);
		~AudioClip();

		AudioClip& operator=(AudioClip&& other) noexcept;

		void loadFromStatic(std::shared_ptr<ResourceDataStatic> data, Metadata meta);
		void loadFromStream(std::shared_ptr<ResourceDataStream> data, Metadata meta);

		String getName() const override;
		size_t copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const override;
		uint8_t getNumberOfChannels() const override;
		size_t getLength() const override; // in samples
		size_t getLoopPoint() const override; // in samples
		bool isLoaded() const override;

		ResourceMemoryUsage getMemoryUsage() const override;

		static std::shared_ptr<AudioClip> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioClip; }
		void reload(Resource&& resource) override;

	private:
		size_t sampleLength = 0;
		size_t loopPoint = 0;
		mutable size_t streamPos = 0;
		uint8_t numChannels = 0;
		bool streaming = false;

		std::array<std::unique_ptr<VorbisData>, 2> vorbisData;

		mutable Vector<Vector<AudioSample>> samples;
		mutable Vector<Vector<AudioSample>> buffer;

		VorbisData* getVorbisData(size_t targetPos) const;
	};
}
