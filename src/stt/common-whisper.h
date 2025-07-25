#pragma once

#include <cstdint>
#include <string>
#include <vector>

using namespace std;

// Read WAV audio file and store the PCM data into pcmf32
// fname can be a buffer of WAV data instead of a filename
// The sample rate of the audio must be equal to COMMON_SAMPLE_RATE
// If stereo flag is set and the audio has 2 channels, the pcmf32s will contain
// 2 channel PCM
auto read_audio_data(const string &fname, vector<float> &pcmf32,
                     vector<vector<float>> &pcmf32s, bool stereo) -> bool;

// convert timestamp to string, 6000 -> 01:00.000
auto to_timestamp(int64_t t, bool comma = false) -> string;

// given a timestamp get the sample
auto timestamp_to_sample(int64_t t, int n_samples, int whisper_sample_rate)
    -> int;

// write text to file, and call system("command voice_id file")
auto speak_with_file(const string &command, const string &text,
                     const string &path, int voice_id) -> bool;
