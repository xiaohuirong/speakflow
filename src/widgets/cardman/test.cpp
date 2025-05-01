#include "cardman.h"
#include "common_stt.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

auto main(int argc, char *argv[]) -> int {
  QApplication app(argc, argv);

  // 创建主窗口
  QWidget mainWindow;
  mainWindow.setWindowTitle("Card Manager");
  auto *mainLayout = new QVBoxLayout(&mainWindow);

  // 创建卡片管理部件
  auto *cardMan = new CardMan();
  mainLayout->addWidget(cardMan, 1);

  // 创建控制面板
  auto *controlPanel = new QWidget();
  auto *controlLayout = new QHBoxLayout(controlPanel);

  auto *inputLine = new QLineEdit();
  auto *addButton = new QPushButton("Add Card");
  auto *removeButton = new QPushButton("Remove Selected");
  auto *clearButton = new QPushButton("Clear All");

  controlLayout->addWidget(inputLine);
  controlLayout->addWidget(addButton);
  controlLayout->addWidget(removeButton);
  controlLayout->addWidget(clearButton);
  mainLayout->addWidget(controlPanel);

  // 设置前后端通信
  const CardWidgetCallbacks frontend_callbacks = cardMan->getCallbacks();

  const STTOperations backend_operations = {
      .addVoice =
          [frontend_callbacks](const std::vector<float> &audio) {
            frontend_callbacks.onVoiceAdded(
                std::to_string(static_cast<int>(audio[0])));
          },
      .removeVoice =
          [frontend_callbacks](size_t index) {
            frontend_callbacks.onVoiceRemoved(index);
            return true;
          },
      .clearVoice =
          [frontend_callbacks]() { frontend_callbacks.onVoiceCleared(); }};

  cardMan->setBackendOperations(backend_operations);

  // 连接按钮信号
  QObject::connect(addButton, &QPushButton::clicked, [&]() {
    QString text = inputLine->text().trimmed();
    if (!text.isEmpty() && backend_operations.addVoice) {
      backend_operations.addVoice(std::vector<float>{text.toFloat()});
      inputLine->clear();
    }
  });

  QObject::connect(removeButton, &QPushButton::clicked, [&]() {
    if (backend_operations.removeVoice) {
      backend_operations.removeVoice(0);
    }
  });

  QObject::connect(clearButton, &QPushButton::clicked, [&]() {
    if (backend_operations.clearVoice) {
      backend_operations.clearVoice();
    }
  });

  mainWindow.show();
  return app.exec();
}
