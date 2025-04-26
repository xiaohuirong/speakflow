#include "mainwindow.h"
#include "common.h"
#include "inference.h"
#include "previewpage.h"
#include "qpushbutton.h"
#include "ui_mainwindow.h"

#include <QWebChannel>
#include <chrono>
#include <print>
#include <qtimer.h>

MainWindow::MainWindow(QWidget *parent, const whisper_params &params)
    : QMainWindow(parent), ui(new Ui::MainWindow), params(params) {
  ui->setupUi(this);

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

  // init audio
  audio = new audio_async(params.length_ms);
  if (!audio->init(params.capture_id, WHISPER_SAMPLE_RATE,
                   (SDL_bool)params.is_microphone)) {
    println(stderr, "{}: audio.init() failed!", __func__);
    exit(-1);
  }

  // whisper init
  if (params.language != "auto" &&
      whisper_lang_id(params.language.c_str()) == -1) {
    println(stderr, "error: unknown language '{}'", params.language);
    // whisper_print_usage(argc, argv, params);
    exit(0);
  }

  whisperCallback = [this, params](const string &text) {
    string keyword = "明镜与点点";
    string message = text;
    if (message != "" && message.find(keyword) == string::npos) {
      message += params.prompt;
      ui->queue->enqueueMessage(QString::fromStdString(message));
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

  ofstream fout;
  if (params.fname_out.length() > 0) {
    fout.open(params.fname_out);
    if (!fout.is_open()) {
      println(stderr, "{}: failed to open output file '{}'!", __func__,
              params.fname_out);
      exit(-1);
    }
  }

  if (params.save_audio) {
    wavWriter = new wav_writer();

    // Get current date/time for filename
    auto now = chrono::system_clock::now();
    string filename =
        format("{:%Y%m%d%H%M%S}.wav", chrono::current_zone()->to_local(now));

    wavWriter->open(filename, WHISPER_SAMPLE_RATE, 16, 1);
  }

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

  // main audio loop
  if (params.save_audio) {
    wavWriter->write(pcmf32_new.data(), pcmf32_new.size());
  }

  const auto t_now = chrono::high_resolution_clock::now();

  audio->get(2000, pcmf32_new);

  bool cur_status = !::vad_simple(pcmf32_new, WHISPER_SAMPLE_RATE, 1000,
                                  params.vad_thold, params.freq_thold, false);
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

  println("before add Voice");
  model->addVoice(params.no_context, pcmf32);
  // model->inference(params.no_context, pcmf32);
  println("after add Voice");

  return 0;
}
