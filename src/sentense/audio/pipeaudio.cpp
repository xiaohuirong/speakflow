#include "pipeaudio.h"
#include <iostream>

const struct pw_stream_events PipeWireAudio::stream_events_ = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = nullptr,
    .control_info = nullptr,
    .io_changed = nullptr,
    .param_changed = PipeWireAudio::on_stream_param_changed,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = PipeWireAudio::on_process,
    .drained = nullptr,
    .command = nullptr};

PipeWireAudio::PipeWireAudio(int len_ms) : AsyncAudio(len_ms) {}

PipeWireAudio::~PipeWireAudio() { cleanup(); }

auto PipeWireAudio::init(int sample_rate, const std::string &input) -> bool {
  if (is_initialized_) {
    return true;
  }
  std::cout << "input string: " << input << std::endl;

  sample_rate_ = sample_rate;
  if (!input.empty()) {
    if (input == "default_output") {
      input_type_ = InputType::DefaultOutput;
    } else {
      input_type_ = InputType::SpecificApplication;
    }
    target_ = input;
  }

  pw_init(nullptr, nullptr);
  loop_ = pw_main_loop_new(nullptr);
  if (!loop_) {
    return false;
  }

  pw_properties *props =
      pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                        "Capture", PW_KEY_MEDIA_ROLE, "Music", nullptr);

  if (input_type_ == InputType::SpecificApplication && !target_.empty()) {
    pw_properties_set(props, PW_KEY_TARGET_OBJECT, target_.c_str());
  } else if (input_type_ == InputType::DefaultOutput) {
    pw_properties_set(props, PW_KEY_STREAM_CAPTURE_SINK, "true");
  }

  stream_ = pw_stream_new_simple(pw_main_loop_get_loop(loop_), "audio-capture",
                                 props, &stream_events_, this);

  if (!stream_) {
    pw_main_loop_destroy(loop_);
    return false;
  }

  uint8_t buffer[1024];
  struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

  const struct spa_pod *params[1];
  struct spa_audio_info_raw audio_info;
  audio_info.format = SPA_AUDIO_FORMAT_F32;
  audio_info.flags = 0;
  audio_info.rate = static_cast<uint32_t>(sample_rate_);
  audio_info.channels = 1;
  params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &audio_info);

  auto flags = static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                            PW_STREAM_FLAG_MAP_BUFFERS |
                                            PW_STREAM_FLAG_RT_PROCESS);

  int res = pw_stream_connect(stream_, PW_DIRECTION_INPUT, PW_ID_ANY, flags,
                              params, 1);

  if (res < 0) {
    cleanup();
    return false;
  }

  buffer_.reserve(sample_rate_ * max_buffer_len_ms_ / 1000);
  is_initialized_ = true;
  is_paused_ = false;

  thread_ = std::thread([this]() { pw_main_loop_run(loop_); });

  return true;
}

auto PipeWireAudio::resume() -> bool {
  if (!is_initialized_) {
    return false;
  }
  is_paused_ = false;
  return true;
}

auto PipeWireAudio::pause() -> bool {
  if (!is_initialized_) {
    return false;
  }
  is_paused_ = true;
  return true;
}

auto PipeWireAudio::clear() -> bool {
  std::lock_guard<std::mutex> lock(buffer_mutex_);
  buffer_.clear();
  return true;
}

void PipeWireAudio::get(int ms, std::vector<float> &audio) {
  if (!is_initialized_ || is_paused_) {
    audio.clear();
    return;
  }

  size_t samples_needed = sample_rate_ * ms / 1000;
  std::unique_lock<std::mutex> lock(buffer_mutex_);

  if (buffer_.size() < samples_needed) {
    buffer_cv_.wait_for(
        lock, std::chrono::milliseconds(100),
        [this, samples_needed] { return buffer_.size() >= samples_needed; });
  }

  if (buffer_.size() >= samples_needed) {
    auto start = buffer_.end() - samples_needed;
    audio.assign(start, buffer_.end());
  } else {
    audio.assign(buffer_.begin(), buffer_.end());
  }
}

void PipeWireAudio::cleanup() {
  if (loop_) {
    pw_main_loop_quit(loop_);
  }

  if (thread_.joinable()) {
    thread_.join();
  }

  if (stream_) {
    pw_stream_destroy(stream_);
    stream_ = nullptr;
  }

  if (loop_) {
    pw_main_loop_destroy(loop_);
    loop_ = nullptr;
  }

  is_initialized_ = false;
}

void PipeWireAudio::on_process(void *userdata) {
  auto *self = static_cast<PipeWireAudio *>(userdata);

  if (self->is_paused_) {
    return;
  }

  struct pw_buffer *b = pw_stream_dequeue_buffer(self->stream_);
  if (!b) {
    return;
  }

  struct spa_buffer *buf = b->buffer;
  auto *samples = static_cast<float *>(buf->datas[0].data);
  if (!samples) {
    pw_stream_queue_buffer(self->stream_, b);
    return;
  }

  uint32_t n_samples = buf->datas[0].chunk->size / sizeof(float);

  {
    std::lock_guard<std::mutex> lock(self->buffer_mutex_);
    size_t max_samples = self->sample_rate_ * self->max_buffer_len_ms_ / 1000;

    self->buffer_.insert(self->buffer_.end(), samples, samples + n_samples);

    if (self->buffer_.size() > max_samples) {
      self->buffer_.erase(self->buffer_.begin(),
                          self->buffer_.begin() +
                              (self->buffer_.size() - max_samples));
    }
  }

  self->buffer_cv_.notify_one();
  pw_stream_queue_buffer(self->stream_, b);
}

void PipeWireAudio::on_stream_param_changed(void *userdata, uint32_t id,
                                            const struct spa_pod *param) {
  auto *self = static_cast<PipeWireAudio *>(userdata);

  if (param == nullptr || id != SPA_PARAM_Format) {
    return;
  }

  struct spa_audio_info info;
  if (spa_format_parse(param, &info.media_type, &info.media_subtype) < 0) {
    return;
  }

  if (info.media_type != SPA_MEDIA_TYPE_audio ||
      info.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
    return;
  }

  spa_format_audio_raw_parse(param, &info.info.raw);
  self->sample_rate_ = info.info.raw.rate;
}

auto AsyncAudio::create(const std::string &type, int len_ms)
    -> std::unique_ptr<AsyncAudio> {
  if (type == "pipewire") {
    return std::make_unique<PipeWireAudio>(len_ms);
  }
  return nullptr;
}
