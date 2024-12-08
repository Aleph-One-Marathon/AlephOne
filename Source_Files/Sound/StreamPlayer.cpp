#include "StreamPlayer.h"

StreamPlayer::StreamPlayer(CallBackStreamPlayer callback, int rate, bool stereo, AudioFormat audioFormat)
	: AudioPlayer(rate, stereo, audioFormat) {
	CallBackFunction = callback;
}

int StreamPlayer::GetNextData(uint8* data, int length) {
	return CallBackFunction(data, std::min(length, buffer_samples));
}