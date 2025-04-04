#pragma once

#include <SDL.h>
#include <SDL_audio.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

//
// SDL Audio capture
//

class audio_async {
public:
  audio_async(int len_ms);
  ~audio_async();

  auto init(int capture_id, int sample_rate, SDL_bool is_microphone) -> bool;

  // start capturing audio via the provided SDL callback
  // keep last len_ms seconds of audio in a circular buffer
  auto resume() -> bool;
  auto pause() -> bool;
  auto clear() -> bool;

  // callback to be called by SDL
  void callback(uint8_t *stream, int len);

  // get audio data from the circular buffer
  void get(int ms, std::vector<float> &audio);

private:
  SDL_AudioDeviceID m_dev_id_in = 0;

  int m_len_ms = 0;
  int m_sample_rate = 0;

  std::atomic_bool m_running;
  std::mutex m_mutex;

  std::vector<float> m_audio;
  size_t m_audio_pos = 0;
  size_t m_audio_len = 0;
};

// Return false if need to quit
auto sdl_poll_events() -> bool;
