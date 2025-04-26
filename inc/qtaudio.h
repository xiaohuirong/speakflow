// this is broken
#include <QAudioDevice>
#include <QAudioSource>
#include <QDebug>
#include <QIODevice>
#include <QMutex>
#include <vector>

class audio_async : public QObject {
  Q_OBJECT

public:
  audio_async(QObject *parent = nullptr)
      : QObject(parent), m_audioSource(nullptr), m_audioIO(nullptr),
        m_sampleRate(16000), m_channelCount(1), m_paused(false) {}

  ~audio_async() {
    if (m_audioSource) {
      m_audioSource->stop();
      delete m_audioSource;
    }
    if (m_audioIO) {
      m_audioIO->close();
      delete m_audioIO;
    }
  }

  bool init(const QAudioDevice &deviceInfo, int sampleRate = 16000,
            int channelCount = 1) {
    QMutexLocker locker(&m_mutex);

    if (m_audioSource) {
      m_audioSource->stop();
      delete m_audioSource;
      m_audioSource = nullptr;
    }

    if (m_audioIO) {
      m_audioIO->close();
      delete m_audioIO;
      m_audioIO = nullptr;
    }

    m_sampleRate = sampleRate;
    m_channelCount = channelCount;
    m_buffer.clear();

    QAudioFormat format;
    format.setSampleRate(m_sampleRate);
    format.setChannelCount(m_channelCount);
    format.setSampleFormat(QAudioFormat::Float);

    if (!deviceInfo.isFormatSupported(format)) {
      qWarning() << "Requested format not supported, trying to use nearest";
      format = deviceInfo.preferredFormat();
    }

    m_audioSource = new QAudioSource(deviceInfo, format, this);
    connect(m_audioSource, &QAudioSource::stateChanged, this,
            &audio_async::handleStateChanged);

    m_audioIO = m_audioSource->start();
    connect(m_audioIO, &QIODevice::readyRead, this,
            &audio_async::handleReadyRead);

    return true;
  }

  void pause() {
    QMutexLocker locker(&m_mutex);
    if (m_audioSource && m_audioSource->state() == QAudio::ActiveState) {
      m_paused = true;
      m_audioSource->suspend();
    }
  }

  void resume() {
    QMutexLocker locker(&m_mutex);
    if (m_audioSource && m_audioSource->state() == QAudio::SuspendedState) {
      m_paused = false;
      m_audioSource->resume();
    }
  }

  bool get(int ms, std::vector<float> &result) {
    QMutexLocker locker(
        &m_mutex); // Use QMutexLocker instead of std::lock_guard

    const size_t n_samples = (m_sampleRate * ms) / 1000;
    if (n_samples == 0 || m_buffer.size() < n_samples) {
      return false;
    }

    result.resize(n_samples);
    std::copy(m_buffer.begin(), m_buffer.begin() + n_samples, result.begin());
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + n_samples);

    return true;
  }

  bool isPaused() const { return m_paused; }

  int sampleRate() const { return m_sampleRate; }

  int channelCount() const { return m_channelCount; }

signals:
  void errorOccurred(const QString &message);

private slots:
  void handleReadyRead() {
    QMutexLocker locker(&m_mutex);

    const qint64 bytesReady = m_audioIO->bytesAvailable();
    const qint64 samplesReady = bytesReady / sizeof(float);

    QByteArray byteArray = m_audioIO->readAll();
    const float *samples =
        reinterpret_cast<const float *>(byteArray.constData());

    m_buffer.reserve(m_buffer.size() + samplesReady);
    for (qint64 i = 0; i < samplesReady; ++i) {
      m_buffer.push_back(samples[i]);
    }
  }

  void handleStateChanged(QAudio::State newState) {
    switch (newState) {
    case QAudio::StoppedState:
      if (m_audioSource->error() != QAudio::NoError) {
        emit errorOccurred("Audio error: " +
                           QString::number(m_audioSource->error()));
      }
      break;
    case QAudio::ActiveState:
      break;
    case QAudio::SuspendedState:
      break;
    case QAudio::IdleState:
      break;
    }
  }

private:
  QAudioSource *m_audioSource;
  QIODevice *m_audioIO;
  std::vector<float> m_buffer;
  QMutex m_mutex;
  int m_sampleRate;
  int m_channelCount;
  bool m_paused;
};
