#pragma once
#include "halley/audio/audio_fade.h"
#include "halley/audio/audio_source.h"

namespace Halley
{
	class AudioSubObjectLayers;
	class AudioEmitter;
	class AudioObject;

	class AudioSourceLayers final : public AudioSource
	{
	public:
		AudioSourceLayers(AudioEngine& engine, AudioEmitter& emitter, Vector<std::unique_ptr<AudioSource>> layerSources, const AudioSubObjectLayers& layerConfig, AudioFade fadeConfig);

		String getName() const override;
		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		bool isReady() const override;
		size_t getSamplesLeft() const override;
		void restart() override;

	private:
		class Layer {
		public:
			std::unique_ptr<AudioSource> source;
			float prevGain = 0;
			float gain = 0;
			bool playing = false;
			bool layerStarted = false;
			bool synchronised = false;
			size_t idx = 0;
			AudioFader fader;

			Layer(std::unique_ptr<AudioSource> source, size_t idx);
			void init(const AudioSubObjectLayers& layerConfig);
			void restart(const AudioSubObjectLayers& layerConfig, AudioEmitter& emitter);
			void setSourceDelay(float delay);
			void update(float time, const AudioSubObjectLayers& layersConfig, AudioEmitter& emitter, const AudioFade& fade);
		};

		AudioEngine& engine;
		AudioEmitter& emitter;
		const AudioSubObjectLayers& layerConfig;
		Vector<Layer> layers;
		AudioFade fadeConfig;
		bool initialized = false;
	};
}
