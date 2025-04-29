#include "mainwindow.h"
#include "inference.h"
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
      sentense(params.vad_model) {
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

  sentenceCallback = [this](const vector<float> &sen) {
    spdlog::info("Detected sentense with {}", sen.size(), " samples");
    model->addVoice(this->params.no_context, sen);
  };
  sentense.setSentenceCallback(sentenceCallback);
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
  spdlog::info("mainwindow.h params.language is: {}", params.language);
  model = make_unique<S2T>(this->params, whisperCallback);
  model->start();

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
  chat = make_unique<Chat>(this->params, chatCallback);
  chat->start();
  if (params.init_prompt != "") {
    chat->addMessage(params.init_prompt);
  }
}

MainWindow::~MainWindow() {
  if (is_running) {
    sentense.stop();
  }
  model->stop();
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
