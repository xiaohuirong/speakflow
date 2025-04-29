#include "mainwindow.h"
#include "inference.h"
#include "monitorwindow.h"
#include "previewpage.h"
#include "qpushbutton.h"
#include "ui_mainwindow.h"

#include <QWebChannel>
#include <print>
#include <qtimer.h>
#include <spdlog/spdlog.h>

MainWindow::MainWindow(QWidget *parent, const whisper_params &params)
    : QMainWindow(parent), ui(new Ui::MainWindow), params(params),
      sentense(params.vad_model) {
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

  sentense.setSentenceCallback([this](const std::vector<float> &sen) {
    spdlog::info("Detected sentense with {}", sen.size(), " samples");
    model->addVoice(this->params.no_context, sen);
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

  model = new S2T(params, whisperCallback);
  mychat = new Chat(params, chatCallback);

  model->start();
  mychat->start();
  if (params.init_prompt != "") {
    mychat->addMessage(params.init_prompt);
  }
}

MainWindow::~MainWindow() {
  if (is_running) {
    sentense.stop();
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
    sentense.start();
  } else {
    is_running = false;
    ui->clickButton->setText("启动");
    ui->statusbar->showMessage("Whisper未启动...");
    sentense.stop();
  }
}
