#include "mainwindow.h"
#include "cardman.h"
#include "events.h"
#include "monitorwindow.h"
#include "previewpage.h"
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
  ui->audio_man->setEventBus(eventBus);

  monitorwindow = make_unique<MonitorWindow>(this);
  connect(ui->monitor, &QAction::triggered, this,
          [this]() { monitorwindow->show(); });

  ui->preview->setContextMenuPolicy(Qt::NoContextMenu);
  page = make_unique<PreviewPage>();
  ui->preview->setPage(page.get());
  auto *channel = new QWebChannel(this);
  channel->registerObject(QStringLiteral("content"), &m_content);
  page->setWebChannel(channel);
  ui->preview->setUrl(QUrl("qrc:/index.html"));

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

  spdlog::info("mainwindow.h params.language is: {}", params.language);
  set_params();
  stt = make_unique<STT>(this->cparams, this->wparams, params.model,
                         params.language, this->params.no_context, eventBus);

  eventBus->publish<StartServiceEvent>("stt");

  eventBus->subscribe<MessageAddedEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto messageEvent = std::static_pointer_cast<MessageAddedEvent>(event);
        if (messageEvent->serviceName == "chat") {
          auto text = messageEvent->message;
          QMetaObject::invokeMethod(this, [this, text]() {
            m_content.appendText(QString::fromStdString(text));
            ui->preview->page()->runJavaScript(
                "window.scrollTo(0, document.body.scrollHeight);");
          });
        }
      });

  chat =
      make_unique<Chat>(this->params.url, this->params.token, this->params.llm,
                        this->params.timeout, this->params.system, eventBus);

  eventBus->publish<StartServiceEvent>("chat");
  if (params.init_prompt != "") {
    eventBus->publish<MessageAddedEvent>("stt", params.init_prompt);
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
    eventBus->publish<StopServiceEvent>("sentense");
  }
  eventBus->publish<StopServiceEvent>("stt");
  eventBus->publish<StopServiceEvent>("chat");
}
