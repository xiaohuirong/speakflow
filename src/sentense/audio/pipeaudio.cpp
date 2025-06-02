#include "pipeaudio.h"
#include <iostream>
#include <spa/param/audio/format-utils.h>
#include <spa/utils/defs.h>
#include <stdexcept>

PipeWireAudio::PipeWireAudio(int len_ms) : Audio(len_ms) {
  pw_init(nullptr, nullptr);
}

PipeWireAudio::~PipeWireAudio() { stop(); }

auto PipeWireAudio::init(int sample_rate, const std::string &input) -> bool {
  if (is_initialized_)
    return true;

  sample_rate_ = sample_rate;
  input_ = input;

  try {
    setupPipeWire();
    startCaptureThread();
    is_initialized_ = true;
    return true;
  } catch (const std::runtime_error &e) {
    std::cerr << "Initialization failed: " << e.what() << std::endl;
    cleanup();
    return false;
  }
}

auto PipeWireAudio::resume() -> bool {
  if (!is_initialized_ || !is_paused_)
    return false;
  is_paused_ = false;
  buffer_cv_.notify_one();
  return true;
}

auto PipeWireAudio::pause() -> bool {
  if (!is_initialized_ || is_paused_)
    return false;
  is_paused_ = true;
  return true;
}

auto PipeWireAudio::clear() -> bool {
  if (!is_initialized_)
    return false;
  std::lock_guard<std::mutex> lock(buffer_mutex_);
  buffer_.clear();
  return true;
}

void PipeWireAudio::get(int ms, std::vector<float> &audio) {
  audio.clear();
  if (!is_initialized_)
    return;

  const size_t samples_needed = (sample_rate_ * ms) / 1000;
  audio.resize(samples_needed, 0.0f);

  std::lock_guard<std::mutex> lock(buffer_mutex_);
  const size_t available = buffer_.size();

  if (available >= samples_needed) {
    auto start = buffer_.end() - samples_needed;
    std::copy(start, buffer_.end(), audio.begin());
  } else if (available > 0) {
    std::copy(buffer_.begin(), buffer_.end(), audio.end() - available);
  }
}

void PipeWireAudio::PipeWireDeleter::operator()(pw_thread_loop *loop) const {
  if (loop)
    pw_thread_loop_destroy(loop);
}

void PipeWireAudio::PipeWireDeleter::operator()(pw_context *ctx) const {
  if (ctx)
    pw_context_destroy(ctx);
}

void PipeWireAudio::PipeWireDeleter::operator()(pw_core *core) const {
  if (core)
    pw_core_disconnect(core);
}

void PipeWireAudio::PipeWireDeleter::operator()(pw_stream *stream) const {
  if (stream)
    pw_stream_destroy(stream);
}

void PipeWireAudio::stop() {
  quit_ = true;
  buffer_cv_.notify_one();

  if (capture_thread_.joinable()) {
    capture_thread_.join();
  }

  cleanup();
  is_initialized_ = false;
}

void PipeWireAudio::cleanup() {
  stream_.reset();
  core_.reset();
  context_.reset();
  mainloop_.reset();
}

void PipeWireAudio::setupPipeWire() {
  mainloop_.reset(pw_thread_loop_new("audio-capture", nullptr));
  if (!mainloop_)
    throw std::runtime_error("Failed to create PipeWire thread loop");

  context_.reset(
      pw_context_new(pw_thread_loop_get_loop(mainloop_.get()), nullptr, 0));
  if (!context_)
    throw std::runtime_error("Failed to create PipeWire context");

  core_.reset(pw_context_connect(context_.get(), nullptr, 0));
  if (!core_)
    throw std::runtime_error("Failed to connect PipeWire core");

  setupStream();
}

void PipeWireAudio::setupStream() {
  uint8_t buffer[1024];
  spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

  params_[0] = reinterpret_cast<const spa_pod *>(spa_pod_builder_add_object(
      &builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
      SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_audio),
      SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
      SPA_FORMAT_AUDIO_format, SPA_POD_Id(SPA_AUDIO_FORMAT_F32),
      SPA_FORMAT_AUDIO_rate, SPA_POD_Int(sample_rate_),
      SPA_FORMAT_AUDIO_channels, SPA_POD_Int(1)));

  pw_properties *props =
      pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                        "Capture", PW_KEY_MEDIA_ROLE, "Communication", nullptr);

  if (!input_.empty()) {
    pw_properties_set(props, PW_KEY_TARGET_OBJECT, input_.c_str());
    input_type_ = InputType::SpecificApplication;
  }

  stream_.reset(pw_stream_new(core_.get(), "audio-capture", props));
  if (!stream_)
    throw std::runtime_error("Failed to create PipeWire stream");

  stream_events_.version = PW_VERSION_STREAM_EVENTS;
  stream_events_.process = &onProcess;

  pw_stream_add_listener(stream_.get(), &stream_listener_, &stream_events_,
                         this);
}

void PipeWireAudio::startCaptureThread() {
  if (pw_thread_loop_start(mainloop_.get()) != 0) {
    throw std::runtime_error("Failed to start PipeWire thread loop");
  }

  capture_thread_ = std::thread([this]() {
    while (!quit_) {
      if (is_paused_) {
        std::unique_lock<std::mutex> lock(buffer_mutex_);
        buffer_cv_.wait(lock, [this] { return !is_paused_ || quit_; });
        if (quit_)
          break;
      }

      if (pw_stream_get_state(stream_.get(), nullptr) ==
          PW_STREAM_STATE_UNCONNECTED) {
        pw_thread_loop_lock(mainloop_.get());
        pw_stream_connect(
            stream_.get(), PW_DIRECTION_INPUT, PW_ID_ANY,
            static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                         PW_STREAM_FLAG_MAP_BUFFERS),
            params_, 1);
        pw_thread_loop_unlock(mainloop_.get());
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });
}

void PipeWireAudio::onProcess(void *userdata) {
  auto *self = static_cast<PipeWireAudio *>(userdata);
  pw_thread_loop_lock(self->mainloop_.get());

  struct pw_buffer *b = pw_stream_dequeue_buffer(self->stream_.get());
  if (!b) {
    pw_thread_loop_unlock(self->mainloop_.get());
    return;
  }

  spa_buffer *buf = b->buffer;
  auto *data = static_cast<float *>(buf->datas[0].data);
  if (data && buf->datas[0].chunk->size > 0) {
    const uint32_t n_samples = buf->datas[0].chunk->size / sizeof(float);

    std::lock_guard<std::mutex> lock(self->buffer_mutex_);
    if (!self->is_paused_) {
      self->buffer_.insert(self->buffer_.end(), data, data + n_samples);

      const size_t max_samples =
          (self->sample_rate_ * self->max_buffer_len_ms_) / 1000;
      if (self->buffer_.size() > max_samples) {
        self->buffer_.erase(self->buffer_.begin(),
                            self->buffer_.begin() +
                                (self->buffer_.size() - max_samples));
      }
    }
  }

  pw_stream_queue_buffer(self->stream_.get(), b);
  pw_thread_loop_unlock(self->mainloop_.get());
}

auto Audio::create(const std::string &type, int len_ms)
    -> std::unique_ptr<Audio> {
  if (type == "pipewire") {
    return std::make_unique<PipeWireAudio>(len_ms);
  }
  return nullptr;
}
