#pragma once

#include <gsl/span>
#include <array>
#include "halley/api/audio_api.h"

namespace Halley
{
	class AudioSource {
	public:
		virtual ~AudioSource() {}

		virtual String getName() const { return String::emptyString(); }
		virtual uint8_t getNumberOfChannels() const = 0;
		virtual size_t getSamplesLeft() const = 0;
		virtual bool isReady() const { return true; }
		virtual bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) = 0;
		virtual void restart() = 0;
	};
}
