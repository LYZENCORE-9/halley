#pragma once
#include "halley/audio/audio_position.h"
#include "audio_voice.h"
#include "halley/data_structures/hash_map.h"

namespace Halley {
    class AudioEmitter {
    public:
        using VoiceCallback = std::function<void(AudioVoice&)>;

        AudioEmitter() = default;
        explicit AudioEmitter(AudioEmitterId id, AudioPosition position, bool temporary = false, AudioEmitter* fallback = nullptr);

        [[nodiscard]] AudioEmitterId getId() const;

        [[nodiscard]] const AudioPosition& getPosition() const;
        void setPosition(const AudioPosition& pos);

        void addVoice(std::unique_ptr<AudioVoice> voice);
        void removeFinishedVoices(Vector<AudioEventId>& removedIds, Vector<AudioObjectId>& removedObjects);
        [[nodiscard]] gsl::span<const std::unique_ptr<AudioVoice>> getVoices() const;
    	size_t forVoices(AudioObjectId audioObjectId, VoiceCallback callback);

        bool shouldBeRemoved();
        void makeTemporary();

        void setSwitchValue(const String& id, String value);
        void setVariableValue(const String& id, float value);
    	const String& getSwitchValue(const String& id) const;
        float getVariableValue(const String& id) const;

        void setRegion(AudioRegionId regionId);
        AudioRegionId getRegion() const;

    	AudioDebugData::EmitterData getDebugData() const;

    private:
        AudioEmitterId id = 0;
        bool temporary = false;
        AudioPosition position;
        AudioEmitter* fallback = nullptr;
        AudioRegionId regionId = 0;

        Vector<std::unique_ptr<AudioVoice>> voices;
        HashMap<String, String> switchValues;
        HashMap<String, float> variableValues;
    };
}
