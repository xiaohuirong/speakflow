#include "qtaudio.h"

// Implementation
QTAudio::QTAudio(int len_ms, QObject *parent)
    : QObject(parent), m_len_ms(len_ms) {}

QTAudio::~QTAudio() {
  if (m_audioSource) {
    m_audioSource->stop();
    delete m_audioSource;
  }
  if (m_audioIO) {
    m_audioIO->close();
  }
}

auto QTAudio::init(int sample_rate, const std::string &input) -> bool {
  int capture_id = -1;
  auto audioInputs = QMediaDevices::audioInputs();
  qDebug() << "Found" << audioInputs.size() << "capture devices:";
  for (int i = 0; i < audioInputs.size(); ++i) {
    qDebug() << "   - Capture device #" << i << ":"
             << audioInputs[i].description();
  }

  if (capture_id >= 0 && capture_id < audioInputs.size()) {
    m_device = audioInputs[capture_id];
    qDebug() << "Attempt to open capture device" << capture_id << ":"
             << m_device.description();
  } else {
    m_device = QMediaDevices::defaultAudioInput();
    qDebug() << "Attempt to open default capture device:"
             << m_device.description();
  }

  if (m_device.isNull()) {
    qDebug() << "Couldn't find any audio capture device!";
    return false;
  }

  QAudioFormat format;
  format.setSampleRate(sample_rate);
  format.setChannelCount(1);
  format.setSampleFormat(QAudioFormat::Float);

  if (!m_device.isFormatSupported(format)) {
    qDebug()
        << "Requested format not supported, using nearest supported format";
    format = m_device.preferredFormat();
  }

  m_sample_rate = format.sampleRate();
  m_audio.resize((m_sample_rate * m_len_ms) / 1000);

  m_audioSource = new QAudioSource(m_device, format, this);
  connect(m_audioSource, &QAudioSource::stateChanged, this,
          &QTAudio::handleStateChanged);

  return true;
}

auto QTAudio::resume() -> bool {
  if (!m_audioSource) {
    qDebug() << "No audio device to resume!";
    return false;
  }

  if (m_running) {
    qDebug() << "Already running!";
    return false;
  }

  m_audioIO = m_audioSource->start();
  connect(m_audioIO, &QIODevice::readyRead, this, &QTAudio::readAudioData,
          Qt::DirectConnection);

  m_running = true;
  return true;
}

auto QTAudio::pause() -> bool {
  if (!m_audioSource) {
    qDebug() << "No audio device to pause!";
    return false;
  }

  if (!m_running) {
    qDebug() << "Already paused!";
    return false;
  }

  m_audioSource->suspend();
  m_running = false;
  return true;
}

auto QTAudio::clear() -> bool {
  if (!m_audioSource) {
    qDebug() << "No audio device to clear!";
    return false;
  }

  if (!m_running) {
    qDebug() << "Not running!";
    return false;
  }

  QMutexLocker locker(&m_mutex);
  m_audio_pos = 0;
  m_audio_len = 0;
  return true;
}

void QTAudio::handleStateChanged(QAudio::State newState) {
  switch (newState) {
  case QAudio::StoppedState:
    if (m_audioSource->error() != QAudio::NoError) {
      qDebug() << "Audio error:" << m_audioSource->error();
    }
    break;
  case QAudio::ActiveState:
    qDebug() << "Audio capture started";
    break;
  case QAudio::SuspendedState:
    qDebug() << "Audio capture suspended";
    break;
  case QAudio::IdleState:
    qDebug() << "Audio capture idle (no data)";
    break;
  }
}

void QTAudio::readAudioData() {
  if (!m_running)
    return;

  const qint64 bytesAvailable = m_audioIO->bytesAvailable();
  const size_t n_samples = bytesAvailable / sizeof(float);

  if (n_samples > m_audio.size()) {
    m_audioIO->read(reinterpret_cast<char *>(m_audio.data()),
                    m_audio.size() * sizeof(float));
    return;
  }

  QMutexLocker locker(&m_mutex);

  if (m_audio_pos + n_samples > m_audio.size()) {
    const size_t n0 = m_audio.size() - m_audio_pos;
    m_audioIO->read(reinterpret_cast<char *>(&m_audio[m_audio_pos]),
                    n0 * sizeof(float));
    m_audioIO->read(reinterpret_cast<char *>(&m_audio[0]),
                    (n_samples - n0) * sizeof(float));
  } else {
    m_audioIO->read(reinterpret_cast<char *>(&m_audio[m_audio_pos]),
                    n_samples * sizeof(float));
  }

  m_audio_pos = (m_audio_pos + n_samples) % m_audio.size();
  m_audio_len = std::min(m_audio_len + n_samples, m_audio.size());
}

void QTAudio::get(int ms, std::vector<float> &result) {
  if (!m_audioSource) {
    qDebug() << "No audio device to get audio from!";
    return;
  }

  if (!m_running) {
    qDebug() << "Not running!";
    return;
  }

  result.clear();

  QMutexLocker locker(&m_mutex);

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
    std::copy(m_audio.begin() + s0, m_audio.end(), result.begin());
    std::copy(m_audio.begin(), m_audio.begin() + (n_samples - n0),
              result.begin() + n0);
  } else {
    std::copy(m_audio.begin() + s0, m_audio.begin() + s0 + n_samples,
              result.begin());
  }
}

auto AsyncAudio::create(const std::string &type, int len_ms)
    -> std::unique_ptr<AsyncAudio> {
  if (type == "qt") {
    return std::make_unique<QTAudio>(len_ms);
  }
  return nullptr;
}
