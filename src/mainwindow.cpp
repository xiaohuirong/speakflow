#include "mainwindow.h"
#include "events.h"
#include "monitorwindow.h"
#include "previewpage.h"
#include "qpushbutton.h"
#include "ui_mainwindow.h"

#include <QWebChannel>
#include <memory>
#include <print>
#include <qtimer.h>
#include <spdlog/spdlog.h>

MainWindow::MainWindow(QWidget *parent, const whisper_params &params)
    : QMainWindow(parent), ui(make_unique<Ui::MainWindow>()), params(params),
      eventBus(std::make_shared<EventBus>()),
      sentense(params.vad_model, eventBus),
      cparams(whisper_context_default_params()),
      wparams(whisper_full_default_params(params.beam_size > 1
                                              ? WHISPER_SAMPLING_BEAM_SEARCH
                                              : WHISPER_SAMPLING_GREEDY)) {

  ui->setupUi(this);
  ui->statusbar->showMessage("Whisper未启动...");

  monitorwindow = make_unique<MonitorWindow>(this);
  connect(ui->openmonitor, &QPushButton::clicked, this,
          [this]() { monitorwindow->show(); });

  ui->preview->setContextMenuPolicy(Qt::NoContextMenu);
  page = make_unique<PreviewPage>();
  ui->preview->setPage(page.get());
  auto *channel = new QWebChannel(this);
  channel->registerObject(QStringLiteral("content"), &m_content);
  page->setWebChannel(channel);
  ui->preview->setUrl(QUrl("qrc:/index.html"));

  connect(ui->clickButton, &QPushButton::clicked, this,
          &MainWindow::handleClick);

  eventBus->subscribe<AudioAddedEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto dataEvent = std::static_pointer_cast<AudioAddedEvent>(event);
        auto sen = dataEvent->audio;

        spdlog::info("Detected sentense with {}", sen.size(), " samples");
        stt->addVoice(sen);
      });
  if (!sentense.initialize()) {
    spdlog::error("sentense initialize failed");
  }

  // whisper init
  if (params.language != "auto" &&
      whisper_lang_id(params.language.c_str()) == -1) {
    spdlog::error("error: unknown language '{}'", params.language);
    // whisper_print_usage(argc, argv, params);
    exit(0);
  }
  whisperCallback = [this](const string &text) {
    QMetaObject::invokeMethod(this, [this, text]() {
      string keyword = "明镜与点点";
      string message = text;
      if (message != "" && message.find(keyword) == string::npos) {
        message += this->params.prompt;
        ui->queue->enqueueMessage(QString::fromStdString(message));
        chat->addMessage(message);
      }
    });
  };
  QueueCallback queueCallback(
      [](const vector<size_t> &sizes) { spdlog::info(sizes.size()); });

  spdlog::info("mainwindow.h params.language is: {}", params.language);
  set_params();
  stt = make_unique<STT>(this->cparams, this->wparams, params.model,
                         params.language, this->params.no_context,
                         whisperCallback);

  stt->start();

  chatCallback = [this](const string &message, bool is_response) {
    QMetaObject::invokeMethod(this, [this, message, is_response]() {
      m_content.appendText(QString::fromStdString(message));
      ui->preview->page()->runJavaScript(
          "window.scrollTo(0, document.body.scrollHeight);");
      if (!is_response) {
        ui->queue->dequeueMessage();
      }
    });
  };
  chat = make_unique<Chat>(this->params.url, this->params.token,
                           this->params.llm, this->params.timeout,
                           this->params.system, chatCallback);
  chat->start();
  if (params.init_prompt != "") {
    chat->addMessage(params.init_prompt);
  }
}

void MainWindow::set_params() {
  cparams.use_gpu = params.use_gpu;
  cparams.flash_attn = params.flash_attn;

  wparams.print_progress = false;
  wparams.print_special = params.print_special;
  wparams.print_realtime = false;
  wparams.print_timestamps = !params.no_timestamps;
  wparams.translate = params.translate;
  wparams.single_segment = !params.use_vad;
  wparams.max_tokens = params.max_tokens;
  wparams.n_threads = params.n_threads;
  wparams.beam_search.beam_size = params.beam_size;

  wparams.audio_ctx = params.audio_ctx;

  wparams.tdrz_enable = params.tinydiarize; // [TDRZ]

  // disable temperature fallback
  // wparams.temperature_inc  = -1.0f;
  wparams.temperature_inc = params.no_fallback ? 0.0f : wparams.temperature_inc;
}

MainWindow::~MainWindow() {
  if (is_running) {
    sentense.stop();
  }
  stt->stop();
  chat->stop();
}

void MainWindow::handleClick() {
  if (!is_running) {
    is_running = true;
    ui->clickButton->setText("停止");
    ui->statusbar->showMessage("Whisper实时记录中...");
    sentense.start();
  } else {
    is_running = false;
    ui->clickButton->setText("启动");
    ui->statusbar->showMessage("Whisper未启动...");
    sentense.stop();
  }
}
