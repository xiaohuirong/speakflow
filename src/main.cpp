// Real-time speech recognition of input from a microphone
#include "mainwindow.h"
#include "parse.h"

#include <QApplication>
#include <cstdio>
#include <nlohmann/json.hpp>
#include <whisper.h>

int main(int argc, char **argv) {
  whisper_params params;

  if (whisper_params_parse(argc, argv, params) == false) {
    return 1;
  }

  QApplication a(argc, argv);
  MainWindow w(nullptr, params);
  w.show();
  return a.exec();
}
