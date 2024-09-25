#include "MusicPlayer.h"
#include "OpenALManager.h"

MusicPlayer::MusicPlayer(std::shared_ptr<StreamDecoder> decoder, MusicParameters parameters) : AudioPlayer(decoder->Rate(), decoder->IsStereo(), decoder->GetAudioFormat()) {
	this->decoder = decoder;
	this->parameters = parameters;
	this->decoder->Rewind();
}

int MusicPlayer::GetNextData(uint8* data, int length) {
	const int dataSize = decoder->Decode(data, length);
	if (dataSize == length || !parameters.Get().loop) return dataSize;
	decoder->Rewind();
	return dataSize + GetNextData(data + dataSize, length - dataSize);
}

SetupALResult MusicPlayer::SetUpALSourceIdle() {
	float default_music_volume = OpenALManager::Get()->GetMusicVolume() * OpenALManager::Get()->GetMasterVolume();
	alSourcef(audio_source->source_id, AL_MAX_GAIN, default_music_volume);
	alSourcef(audio_source->source_id, AL_GAIN, default_music_volume * parameters.Get().volume);
	return SetupALResult(alGetError() == AL_NO_ERROR, true);
}

DynamicMusicPlayer::DynamicMusicPlayer(std::vector<Preset>& presets, int starting_preset_index, int starting_segment_index, MusicParameters parameters) 
	: MusicPlayer(presets[starting_preset_index].GetSegment(starting_segment_index)->GetDecoder(), parameters) {
	music_presets = presets;
	current_preset_index = starting_preset_index;
	current_segment_index = starting_segment_index;
	requested_preset_index = current_preset_index;
}

int DynamicMusicPlayer::GetNextData(uint8* data, int length) {
	const int dataSize = decoder->Decode(data, length);
	if (dataSize == length) return dataSize;
	decoder->Rewind();

	const int nextPresetIndex = requested_preset_index.load();
	const int nextSegmentIndex = music_presets[current_preset_index].GetSegment(current_segment_index)->GetNextSegmentIndex(nextPresetIndex);

	if (nextSegmentIndex != NONE)
	{
		current_segment_index = nextSegmentIndex;
		current_preset_index = nextPresetIndex;
	}

	decoder = music_presets[current_preset_index].GetSegment(current_segment_index)->GetDecoder();
	Init(decoder->Rate(), decoder->IsStereo(), decoder->GetAudioFormat());
	return BufferFormatChanged() ? dataSize : dataSize + GetNextData(data + dataSize, length - dataSize);
}

bool DynamicMusicPlayer::RequestPresetTransition(int preset_index) {
	if (preset_index < 0 || preset_index >= music_presets.size()) return false;
	requested_preset_index.store(preset_index);
	return true;
}