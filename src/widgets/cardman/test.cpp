#include "cardman.h"
#include "events.h"
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

  auto eventBus = std::make_shared<EventBus>();

  eventBus->subscribe<StartServiceEvent>(
      [eventBus](const std::shared_ptr<Event> &event) {
        auto startEvent = std::static_pointer_cast<StartServiceEvent>(event);
        if (startEvent->serviceName == "sentense") {
          eventBus->publish<ServiceStatusEvent>("sentense", true);
        }
      });

  eventBus->subscribe<StopServiceEvent>(
      [eventBus](const std::shared_ptr<Event> &event) {
        auto stopEvent = std::static_pointer_cast<StopServiceEvent>(event);
        if (stopEvent->serviceName == "sentense") {
          eventBus->publish<ServiceStatusEvent>("sentense", false);
        }
      });

  // 创建卡片管理部件
  auto *cardMan = new CardMan();
  cardMan->setEventBus(eventBus);
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

  // 连接按钮信号
  QObject::connect(addButton, &QPushButton::clicked, [&]() {
    QString text = inputLine->text().trimmed();
    if (!text.isEmpty()) {
      eventBus->publish<AudioAddedEvent>(std::vector<float>{text.toFloat()});
      inputLine->clear();
    }
  });

  QObject::connect(removeButton, &QPushButton::clicked,
                   [&]() { eventBus->publish<AudioRemovedEvent>(0); });

  QObject::connect(clearButton, &QPushButton::clicked,
                   [&]() { eventBus->publish<AudioClearedEvent>(); });

  mainWindow.show();
  return app.exec();
}
