#include "sdlaudio.h"

#include <cstdio>
#include <print>

audio_async::audio_async(int len_ms) {
  m_len_ms = len_ms;

  m_running = false;
}

audio_async::~audio_async() {
  if (m_dev_id_in) {
    SDL_CloseAudioDevice(m_dev_id_in);
  }
}

auto audio_async::init(int capture_id, int sample_rate, SDL_bool is_microphone)
    -> bool {
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n",
                 SDL_GetError());
    return false;
  }

  SDL_SetHintWithPriority(SDL_HINT_AUDIO_RESAMPLING_MODE, "medium",
                          SDL_HINT_OVERRIDE);

  {
    int nDevices = SDL_GetNumAudioDevices(is_microphone);
    println(stderr, "{}: found {} capture devices:", __func__, nDevices);
    for (int i = 0; i < nDevices; i++) {
      println(stderr, "{}:    - Capture device #{}: '{}'", __func__, i,
              SDL_GetAudioDeviceName(i, is_microphone));
    }
  }

  SDL_AudioSpec capture_spec_requested;
  SDL_AudioSpec capture_spec_obtained;

  SDL_zero(capture_spec_requested);
  SDL_zero(capture_spec_obtained);

  capture_spec_requested.freq = sample_rate;
  capture_spec_requested.format = AUDIO_F32;
  capture_spec_requested.channels = 1;
  capture_spec_requested.samples = 1024;
  capture_spec_requested.callback = [](void *userdata, uint8_t *stream,
                                       int len) {
    auto *audio = (audio_async *)userdata;
    audio->callback(stream, len);
  };
  capture_spec_requested.userdata = this;

  if (capture_id >= 0) {
    println(stderr, "{}: attempt to open capture device {} : '{}' ...",
            __func__, capture_id,
            SDL_GetAudioDeviceName(capture_id, is_microphone));
    m_dev_id_in = SDL_OpenAudioDevice(
        SDL_GetAudioDeviceName(capture_id, is_microphone), is_microphone,
        &capture_spec_requested, &capture_spec_obtained, 0);
  } else {
    println(stderr, "{}: attempt to open default capture device ...", __func__);
    m_dev_id_in =
        SDL_OpenAudioDevice(nullptr, is_microphone, &capture_spec_requested,
                            &capture_spec_obtained, 0);
  }

  if (!m_dev_id_in) {
    println(stderr, "{}: couldn't open an audio device for capture: {}!",
            __func__, SDL_GetError());
    m_dev_id_in = 0;

    return false;
  } else {
    println(stderr,
            "{}: obtained spec for input device (SDL Id = {}):", __func__,
            m_dev_id_in);
    println(stderr, "{}:     - sample rate:       {}", __func__,
            capture_spec_obtained.freq);
    println(stderr, "{}:     - format:            {} (required: {})", __func__,
            capture_spec_obtained.format, capture_spec_requested.format);
    println(stderr, "{}:     - channels:          {} (required: {})", __func__,
            capture_spec_obtained.channels, capture_spec_requested.channels);
    println(stderr, "{}:     - samples per frame: {}", __func__,
            capture_spec_obtained.samples);
  }

  m_sample_rate = capture_spec_obtained.freq;

  m_audio.resize((m_sample_rate * m_len_ms) / 1000);

  return true;
}

auto audio_async::resume() -> bool {
  if (!m_dev_id_in) {
    println(stderr, "{}: no audio device to resume!", __func__);
    return false;
  }

  if (m_running) {
    println(stderr, "{}: already running!", __func__);
    return false;
  }

  SDL_PauseAudioDevice(m_dev_id_in, 0);

  m_running = true;

  return true;
}

auto audio_async::pause() -> bool {
  if (!m_dev_id_in) {
    println(stderr, "{}: no audio device to pause!", __func__);
    return false;
  }

  if (!m_running) {
    println(stderr, "{}: already paused!", __func__);
    return false;
  }

  SDL_PauseAudioDevice(m_dev_id_in, 1);

  m_running = false;

  return true;
}

auto audio_async::clear() -> bool {
  if (!m_dev_id_in) {
    println(stderr, "{}: no audio device to clear!", __func__);
    return false;
  }

  if (!m_running) {
    println(stderr, "{}: not running!", __func__);
    return false;
  }

  {
    lock_guard<mutex> lock(m_mutex);

    m_audio_pos = 0;
    m_audio_len = 0;
  }

  return true;
}

// callback to be called by SDL
void audio_async::callback(uint8_t *stream, int len) {
  if (!m_running) {
    return;
  }

  size_t n_samples = len / sizeof(float);

  if (n_samples > m_audio.size()) {
    n_samples = m_audio.size();

    stream += (len - (n_samples * sizeof(float)));
  }

  // fprintf(stderr, "%s: %zu samples, pos %zu, len %zu\n", __func__, n_samples,
  // m_audio_pos, m_audio_len);

  {
    lock_guard<mutex> lock(m_mutex);

    if (m_audio_pos + n_samples > m_audio.size()) {
      const size_t n0 = m_audio.size() - m_audio_pos;

      memcpy(&m_audio[m_audio_pos], stream, n0 * sizeof(float));
      memcpy(&m_audio[0], stream + n0 * sizeof(float),
             (n_samples - n0) * sizeof(float));
    } else {
      memcpy(&m_audio[m_audio_pos], stream, n_samples * sizeof(float));
    }
    m_audio_pos = (m_audio_pos + n_samples) % m_audio.size();
    m_audio_len = min(m_audio_len + n_samples, m_audio.size());
  }
}

void audio_async::get(int ms, vector<float> &result) {
  if (!m_dev_id_in) {
    println(stderr, "{}: no audio device to get audio from!", __func__);
    return;
  }

  if (!m_running) {
    println(stderr, "{}: not running!", __func__);
    return;
  }

  result.clear();

  {
    lock_guard<mutex> lock(m_mutex);

    if (ms <= 0) {
      ms = m_len_ms;
    }

    size_t n_samples = (m_sample_rate * ms) / 1000;
    if (n_samples > m_audio_len) {
      n_samples = m_audio_len;
    }

    result.resize(n_samples);

    int s0 = m_audio_pos - n_samples;
    if (s0 < 0) {
      s0 += m_audio.size();
    }

    if (s0 + n_samples > m_audio.size()) {
      const size_t n0 = m_audio.size() - s0;

      memcpy(result.data(), &m_audio[s0], n0 * sizeof(float));
      memcpy(&result[n0], &m_audio[0], (n_samples - n0) * sizeof(float));
    } else {
      memcpy(result.data(), &m_audio[s0], n_samples * sizeof(float));
    }
  }
}

auto sdl_poll_events() -> bool {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT: {
      return false;
    }
    default:
      break;
    }
  }

  return true;
}
