#ifndef __MUSIC_H
#define __MUSIC_H

/*

	Copyright (C) 1991-2001 and beyond by Bungie Studios, Inc.
	and the "Aleph One" developers.
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

	Handles both intro and level music

*/

#include "cseries.h"
#include "Decoder.h"
#include "FileHandler.h"
#include "Random.h"
#include "SoundManager.h"
#include "MusicPlayer.h"
#include <vector>

class Music
{
public:
	static Music *instance() { 
		static Music *m_instance = nullptr;
		if (!m_instance) 
			m_instance = new Music(); 
		return m_instance; 
	}

	static constexpr int reserved_music_slots = 2;
	enum MusicSlot {
		Intro = 0,
		Level = 1
	};

	class Slot {
	private:
		uint32 music_fade_start = 0;
		uint32 music_fade_duration = 0;
		float music_fade_limit_volume;
		float music_fade_start_volume;
		bool music_fade_stop_no_volume;
	protected:
		std::shared_ptr<MusicPlayer> musicPlayer;
		MusicParameters parameters;
	public:
		void Fade(float limitVolume, short duration, bool stopOnNoVolume = true);
		bool Playing() const { return IsInit() && musicPlayer && musicPlayer->IsActive(); }
		void Pause();
		virtual void Close();
		virtual bool IsInit() const = 0;
		virtual void Play() = 0;
		bool SetParameters(bool loop, float volume);
		float GetLimitFadeVolume() const { return music_fade_limit_volume; }
		bool IsFading() const { return music_fade_start; }
		bool StopPlayerAfterFadeOut() const { return music_fade_stop_no_volume; }
		void StopFade() { music_fade_start = 0; }
		void SetVolume(float volume);
		float GetVolume() const { return parameters.volume; }
		std::pair<bool, float> ComputeFadingVolume() const;
	};

	class StandardSlot : public Slot {
	private:
		std::shared_ptr<StreamDecoder> decoder;
		FileSpecifier music_file;
	public:
		bool Open(FileSpecifier* file);
		void Close() override;
		void Play() override;
		bool IsInit() const override { return decoder != nullptr; };
	};

	class DynamicSlot : public Slot {
	public:
		bool IsInit() const override { return true; };
		void Close() override;
		void Play() override;
		int LoadTrack(FileSpecifier* file);
		int AddPreset();
		int AddSegmentToPreset(int preset_index, int track_index);
		bool SelectStartingSegment(int preset_index, int segment_index);
		bool IsSegmentIndexValid(int preset_index, int segment_index) const;
		bool SetNextSegment(int preset_index, int segment_index, int transition_preset_index, int transition_segment_index);
		bool SetPresetTransition(int preset_index);
	private:
		std::vector<DynamicMusicPlayer::Segment> dynamic_music_tracks;
		std::vector<DynamicMusicPlayer::Preset> dynamic_music_presets;
		int default_preset_index = 0;
		int default_segment_index = 0;
	};

	bool SetupIntroMusic(FileSpecifier& file);
	void RestartIntroMusic();
	Slot* GetSlot(int index) const { return index >= reserved_music_slots && index < music_slots.size() ? music_slots[index].get() : nullptr; }
	DynamicSlot* GetDynamicSlot(int index) const { return index >= reserved_music_slots && index < music_slots.size() ? dynamic_cast<DynamicSlot*>(music_slots[index].get()) : nullptr; }

	void Fade(float limitVolume, short duration, bool stopOnNoVolume = true, int index = NONE);
	void Pause(int index = NONE);
	bool Playing(int index = NONE);
	int Load(FileSpecifier& file, bool loop, float volume);
	int Load(float volume);
	void Idle();
	void StopLevelMusic() { music_slots[MusicSlot::Level]->Close(); }
	void ClearLevelMusic();
	void PushBackLevelMusic(const FileSpecifier& file);
	void LevelMusicRandom(bool fRandom) { random_order = fRandom; }
	void SeedLevelMusic();
	void SetClassicLevelMusic(short song_index);
	bool HasClassicLevelMusic() const { return marathon_1_song_index >= 0; }
private:
	std::vector<std::unique_ptr<Slot>> music_slots;

	Music();
	FileSpecifier* GetLevelMusic();
	bool LoadLevelMusic();

	// level music
	short marathon_1_song_index;
	std::vector<FileSpecifier> playlist;
	size_t song_number;
	bool random_order;
	GM_Random randomizer;
};

#endif
