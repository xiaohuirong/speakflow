#include "mainwindow.h"
#include "common.h"
#include "inference.h"
#include "monitorwindow.h"
#include "previewpage.h"
#include "qpushbutton.h"
#include "ui_mainwindow.h"

#include <QWebChannel>
#include <chrono>
#include <print>
#include <qtimer.h>
#include <spdlog/spdlog.h>

MainWindow::MainWindow(QWidget *parent, const whisper_params &params)
    : QMainWindow(parent), ui(new Ui::MainWindow), params(params),
      vad(params.vad_model) {
  ui->setupUi(this);

  monitorwindow = new MonitorWindow(this);

  connect(ui->openmonitor, &QPushButton::clicked, this->monitorwindow,
          &MonitorWindow::show);

  ui->preview->setContextMenuPolicy(Qt::NoContextMenu);

  page = new PreviewPage();
  ui->preview->setPage(page);

  connect(ui->clickButton, &QPushButton::clicked, this,
          &MainWindow::handleClick);

  auto *channel = new QWebChannel(this);
  channel->registerObject(QStringLiteral("content"), &m_content);
  page->setWebChannel(channel);

  ui->preview->setUrl(QUrl("qrc:/index.html"));

  ui->statusbar->showMessage("Whisper未启动...");

  chatCallback = [this](const string &message, bool is_response) {
    m_content.appendText(QString::fromStdString(message));
    if (!is_response) {
      QMetaObject::invokeMethod(ui->queue, "dequeueMessage",
                                Qt::QueuedConnection);
    }
  };

#ifdef USE_SDL_AUDIO
  // init audio
  audio = new audio_async(params.length_ms);
  if (!audio->init(params.capture_id, WHISPER_SAMPLE_RATE,
                   (SDL_bool)params.is_microphone)) {
    spdlog::error("{}: audio.init() failed!", __func__);
    exit(-1);
  }
#elif defined(USE_QT_AUDIO)
  audio = new AudioAsync(params.length_ms);
  if (!audio->init(-1, WHISPER_SAMPLE_RATE, true)) {
    spdlog::error("Failed to initialize audio");
  }
#endif

  // whisper init
  if (params.language != "auto" &&
      whisper_lang_id(params.language.c_str()) == -1) {
    spdlog::error("error: unknown language '{}'", params.language);
    // whisper_print_usage(argc, argv, params);
    exit(0);
  }

  whisperCallback = [this](const string &text) {
    string keyword = "明镜与点点";
    string message = text;
    if (message != "" && message.find(keyword) == string::npos) {
      message += this->params.prompt;
      QMetaObject::invokeMethod(
          ui->queue, "enqueueMessage", Qt::QueuedConnection,
          Q_ARG(QString, QString::fromStdString(message)));

      mychat->addMessage(message);
    }
  };

  model = new S2T(this->params, whisperCallback);
  mychat = new Chat(this->params, chatCallback);

  pcmf32 = vector<float>(params.n_samples_30s, 0.0f);
  pcmf32_new = vector<float>(params.n_samples_30s, 0.0f);

  timer = new QTimer();
  connect(timer, &QTimer::timeout, this, &MainWindow::running);

  model->start();
  mychat->start();
  if (params.init_prompt != "") {
    mychat->addMessage(params.init_prompt);
  }
}

MainWindow::~MainWindow() {
  if (is_running) {
    audio->pause();
  }
  model->stop();
  mychat->stop();
  delete ui;
  delete model;
  delete mychat;
}

void MainWindow::handleClick() {
  if (!is_running) {
    is_running = true;
    ui->clickButton->setText("停止");
    ui->statusbar->showMessage("Whisper实时记录中...");
    audio->resume();
    t_change = chrono::high_resolution_clock::now();
    timer->start(2000);
  } else {
    is_running = false;
    ui->clickButton->setText("启动");
    ui->statusbar->showMessage("Whisper未启动...");
    audio->pause();
    timer->stop();
  }
}

auto MainWindow::running() -> int {
  if (ui->check->isChecked()) {
    page->scrollToBottom();
  }

  const auto t_now = chrono::high_resolution_clock::now();

  audio->get(2000, pcmf32_new);

  bool cur_status = !::vad_simple(pcmf32_new, WHISPER_SAMPLE_RATE, 1000,
                                  params.vad_thold, params.freq_thold, false);

  if (cur_status) {
    this->monitorwindow->add_point(t_now, 1);
  } else {
    this->monitorwindow->add_point(t_now, 0);
  }
  spdlog::info("status: {}", cur_status);

  if (cur_status ^ last_status) {
    last_status = cur_status;
    if (!cur_status) {
      const auto diff_len =
          chrono::duration_cast<chrono::milliseconds>(t_now - t_change);
      audio->get(diff_len.count() + 2000, pcmf32);
    } else {
      t_change = t_now;
      return 0;
    }
  } else {
    if (cur_status &&
        chrono::duration_cast<chrono::milliseconds>(t_now - t_change).count() >=
            30000) {
      audio->get(30000, pcmf32);
      t_change = t_now;
    } else {
      return 0;
    }
  }

  spdlog::info("before vad");
  // Process the audio.
  vad.process(pcmf32);
  // Retrieve the speech timestamps (in samples).
  vector<timestamp_t> stamps = vad.get_speech_timestamps();
  // Convert timestamps to seconds and round to one decimal place (for
  // 16000 Hz).
  const float sample_rate_float = 16000.0f;
  for (size_t i = 0; i < stamps.size(); i++) {
    float start_sec =
        rint((stamps[i].start / sample_rate_float) * 10.0f) / 10.0f;
    float end_sec = rint((stamps[i].end / sample_rate_float) * 10.0f) / 10.0f;
    cout << "Speech detecte from " << fixed << setprecision(1) << start_sec
         << " s to " << fixed << setprecision(1) << end_sec << " s" << endl;
  }
  // Optionally, reset the internal state.
  vad.reset();
  spdlog::info("after vad");

  spdlog::info("before add Voice");
  model->addVoice(params.no_context, pcmf32);
  spdlog::info("after add Voice");

  return 0;
}
