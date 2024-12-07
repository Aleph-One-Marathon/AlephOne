#ifndef __MUSIC_PLAYER_H
#define __MUSIC_PLAYER_H

#include "AudioPlayer.h"

struct MusicParameters {
	float volume = 1;
	bool loop = true;
};

class MusicPlayer : public AudioPlayer {
public:
	MusicPlayer(std::shared_ptr<StreamDecoder> decoder, const MusicParameters& parameters); //Must not be used outside OpenALManager (public for make_shared)
	float GetPriority() const override { return 5; } //Doesn't really matter, just be above maximum volume (1) to be prioritized over sounds
	void UpdateParameters(const MusicParameters& musicParameters) { parameters.Store(musicParameters); }
	MusicParameters GetParameters() const { return parameters.Get(); }
private:
	int GetNextData(uint8* data, int length) override;
	SetupALResult SetUpALSourceIdle() override;
	bool LoadParametersUpdates() override { return parameters.Update(); }
protected:
	AtomicStructure<MusicParameters> parameters;
	std::shared_ptr<StreamDecoder> decoder;

	friend class OpenALManager;
};

class DynamicMusicPlayer : public MusicPlayer {
public:

	class Segment {
	private:
		std::shared_ptr<StreamDecoder> decoder;
		std::unordered_map<int, int> presets_mapping; //preset index - next segment index
	public:
		Segment(std::shared_ptr<StreamDecoder> decoder) { this->decoder = decoder; }
		std::shared_ptr<StreamDecoder> GetDecoder() const { return decoder; }
		int GetNextSegmentIndex(int preset_index) const { return presets_mapping.find(preset_index) != presets_mapping.end() ? presets_mapping.find(preset_index)->second : NONE; }
		void SetNextSegment(int preset_index, int segment_index) { presets_mapping[preset_index] = segment_index; }
	};

	class Preset {
	private:
		std::vector<Segment> segments;
	public:
		void AddSegment(const Segment& segment) { segments.push_back(segment); }
		Segment* GetSegment(int index) { return index >= 0 && index < segments.size() ? &segments[index] : nullptr; }
		const std::vector<Segment>& GetSegments() const { return segments; }
	};

	DynamicMusicPlayer(std::vector<Preset>& presets, int starting_preset_index, int starting_segment_index, const MusicParameters& parameters); //Must not be used outside OpenALManager (public for make_shared)
	bool RequestPresetTransition(int preset_index);

private:
	int GetNextData(uint8* data, int length) override;
	std::vector<Preset> music_presets;
	int current_preset_index;
	int current_segment_index;
	std::atomic_int requested_preset_index;

	friend class OpenALManager;
};

#endif