#include "mainwindow.h"
#include "common.h"
#include "qpushbutton.h"
#include "ui_mainwindow.h"

#include "inference.h"
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <qtimer.h>

using json = nlohmann::json;

MainWindow::MainWindow(QWidget *parent, const whisper_params &params)
    : QMainWindow(parent), ui(new Ui::MainWindow), params(params) {
  ui->setupUi(this);
  connect(ui->clickButton, &QPushButton::clicked, this,
          &MainWindow::handleClick);

  // init audio
  audio = new audio_async(params.length_ms);
  if (!audio->init(params.capture_id, WHISPER_SAMPLE_RATE,
                   (SDL_bool)params.is_microphone)) {
    fprintf(stderr, "%s: audio.init() failed!\n", __func__);
    exit(-1);
  }

  audio->resume();

  // whisper init
  if (params.language != "auto" &&
      whisper_lang_id(params.language.c_str()) == -1) {
    fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
    // whisper_print_usage(argc, argv, params);
    exit(0);
  }

  model = new S2T(this->params);
  mychat = new Chat(this->params);

  pcmf32 = std::vector<float>(params.n_samples_30s, 0.0f);
  pcmf32_new = std::vector<float>(params.n_samples_30s, 0.0f);

  is_running = true;

  std::ofstream fout;
  if (params.fname_out.length() > 0) {
    fout.open(params.fname_out);
    if (!fout.is_open()) {
      fprintf(stderr, "%s: failed to open output file '%s'!\n", __func__,
              params.fname_out.c_str());
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
  delete ui;
  audio->pause();
  delete model;
  delete mychat;
}

void MainWindow::handleClick() {
  ui->logOutput->append("Hello world!");
  t_change = std::chrono::high_resolution_clock::now();
  timer->start(2000);
}

int MainWindow::running() {

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
    if (cur_status == 0) {
      const auto diff_len =
          std::chrono::duration_cast<std::chrono::milliseconds>(t_now -
                                                                t_change);
      audio->get(diff_len.count() + 2000, pcmf32);
    } else {
      t_change = t_now;
      return 0;
    }
  } else {
    return 0;
  }

  std::string result = model->inference(params, pcmf32);

  std::cout << result << std::endl;

  // auto callback = [this](const std::string &response) {
  //   json data = json::parse(response);
  //   std::string ai_response = data["choices"][0]["message"]["content"];
  //   std::cout << "Response received: " << ai_response << std::endl;
  //   // ui->logOutput->append(QString::fromStdString(ai_response));
  //   // this->ui->logOutput->append(QString::fromStdString(ai_response));
  // };
  //
  // mychat->send_async(result, callback);

  return 0;
}
