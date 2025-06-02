#pragma once

#include <SDL.h>
#include <SDL_audio.h>

#include "audio.h"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

using namespace std;

//
// SDL Audio capture
//

class SDLAudio : public AsyncAudio {
public:
  explicit SDLAudio(int len_ms = 2000);
  ~SDLAudio() override;

  auto init(int sample_rate, const std::string &input = "") -> bool override;

  // start capturing audio via the provided SDL callback
  // keep last len_ms seconds of audio in a circular buffer
  auto resume() -> bool override;
  auto pause() -> bool override;
  auto clear() -> bool override;

  // get audio data from the circular buffer
  void get(int ms, vector<float> &audio) override;

private:
  // callback to be called by SDL
  void callback(uint8_t *stream, int len);

  SDL_AudioDeviceID m_dev_id_in = 0;

  int m_len_ms = 0;
  int m_sample_rate = 0;

  atomic_bool m_running;
  mutex m_mutex;

  vector<float> m_audio;
  size_t m_audio_pos = 0;
  size_t m_audio_len = 0;
};

// Return false if need to quit
auto sdl_poll_events() -> bool;
