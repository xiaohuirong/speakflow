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

using json = nlohmann::json;

MainWindow::MainWindow(QWidget *parent, const whisper_params &params)
    : QMainWindow(parent), ui(new Ui::MainWindow), params(params) {
  ui->setupUi(this);

  ui->editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  ui->preview->setContextMenuPolicy(Qt::NoContextMenu);

  page = new PreviewPage(this);
  ui->preview->setPage(page);

  connect(ui->editor, &QPlainTextEdit::textChanged,
          [this]() { m_content.setText(ui->editor->toPlainText()); });

  connect(ui->clickButton, &QPushButton::clicked, this,
          &MainWindow::handleClick);

  auto *channel = new QWebChannel(this);
  channel->registerObject(QStringLiteral("content"), &m_content);
  page->setWebChannel(channel);

  ui->preview->setUrl(QUrl("qrc:/index.html"));

  ui->statusbar->showMessage("Whisper未启动...");

  namedCallback = [this](const std::string &response) {
    QMetaObject::invokeMethod(ui->editor, "appendPlainText",
                              Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString("### AI")));
    QMetaObject::invokeMethod(ui->editor, "appendPlainText",
                              Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(response)));
  };

  // init audio
  audio = new audio_async(params.length_ms);
  if (!audio->init(params.capture_id, WHISPER_SAMPLE_RATE,
                   (SDL_bool)params.is_microphone)) {
    std::println(stderr, "{}: audio.init() failed!", __func__);
    exit(-1);
  }

  // whisper init
  if (params.language != "auto" &&
      whisper_lang_id(params.language.c_str()) == -1) {
    std::println(stderr, "error: unknown language '{}'", params.language);
    // whisper_print_usage(argc, argv, params);
    exit(0);
  }

  model = new S2T(this->params);
  mychat = new Chat(this->params);

  pcmf32 = std::vector<float>(params.n_samples_30s, 0.0f);
  pcmf32_new = std::vector<float>(params.n_samples_30s, 0.0f);

  std::ofstream fout;
  if (params.fname_out.length() > 0) {
    fout.open(params.fname_out);
    if (!fout.is_open()) {
      std::println(stderr, "{}: failed to open output file '{}'!", __func__,
                   params.fname_out);
      exit(-1);
    }
  }

  if (params.save_audio) {
    wavWriter = new wav_writer();

    // Get current date/time for filename
    time_t now = time(0);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", localtime(&now));
    std::string filename = std::string(buffer) + ".wav";

    wavWriter->open(filename, WHISPER_SAMPLE_RATE, 16, 1);
  }

  timer = new QTimer();
  connect(timer, &QTimer::timeout, this, &MainWindow::running);
}

MainWindow::~MainWindow() {
  audio->pause();
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
    mychat->start();
    t_change = std::chrono::high_resolution_clock::now();
    timer->start(2000);
  } else {
    is_running = false;
    ui->clickButton->setText("启动");
    ui->statusbar->showMessage("Whisper未启动...");
    audio->pause();
    mychat->stop();
    timer->stop();
  }
}

auto MainWindow::running() -> int {
  page->scrollToBottom();

  // main audio loop
  if (params.save_audio) {
    wavWriter->write(pcmf32_new.data(), pcmf32_new.size());
  }

  const auto t_now = std::chrono::high_resolution_clock::now();

  audio->get(2000, pcmf32_new);

  bool cur_status = !::vad_simple(pcmf32_new, WHISPER_SAMPLE_RATE, 1000,
                                  params.vad_thold, params.freq_thold, false);
  if (cur_status ^ last_status) {
    last_status = cur_status;
    if (!cur_status) {
      const auto diff_len =
          std::chrono::duration_cast<std::chrono::milliseconds>(t_now -
                                                                t_change);
      audio->get(diff_len.count() + 2000, pcmf32);
    } else {
      t_change = t_now;
      return 0;
    }
  } else {
    if (cur_status &&
        std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_change)
                .count() >= 30000) {
      audio->get(30000, pcmf32);
      t_change = t_now;
    } else {
      return 0;
    }
  }

  std::string message = model->inference(params, pcmf32);

  ui->editor->appendPlainText("### User");
  ui->editor->appendPlainText(QString::fromStdString(message));

  mychat->sendMessage(message, namedCallback);

  return 0;
}
