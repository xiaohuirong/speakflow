// frontend.cpp
#include "cardman.h"
#include <sys/socket.h>

CardMan::CardMan(QWidget *parent) : QWidget(parent) {
  // 创建UI
  auto *layout = new QVBoxLayout(this);

  listWidget = new QListWidget(this);
  inputLine = new QLineEdit(this);

  addButton = new QPushButton("Add", this);
  removeButton = new QPushButton("Remove Selected", this);
  clearButton = new QPushButton("Clear All", this);

  layout->addWidget(listWidget);
  layout->addWidget(inputLine);
  layout->addWidget(addButton);
  layout->addWidget(removeButton);
  layout->addWidget(clearButton);

  connect(addButton, &QPushButton::clicked, this, &CardMan::onAddClicked);
  connect(removeButton, &QPushButton::clicked, this, &CardMan::onRemoveClicked);
  connect(clearButton, &QPushButton::clicked, this, &CardMan::onClearClicked);
}

CardMan::~CardMan() = default;

auto CardMan::getCallbacks() -> CardWidgetCallbacks {
  CardWidgetCallbacks callbacks;

  callbacks.onVoiceAdded = [this](const std::string &length) {
    QMetaObject::invokeMethod(
        this,
        [this, length]() {
          listWidget->addItem(QString::fromStdString(length));
        },
        Qt::QueuedConnection);
  };

  callbacks.onVoiceRemoved = [this](const int index) {
    QMetaObject::invokeMethod(this, [this, index]() {}, Qt::QueuedConnection);
  };

  callbacks.onVoiceCleared = [this]() {
    QMetaObject::invokeMethod(
        this, [this]() { listWidget->clear(); }, Qt::QueuedConnection);
  };

  return callbacks;
}

void CardMan::setBackendOperations(const STTOperations &ops) {
  backendOps = ops;
  // updateList();
}

// void CardMan::updateList() {
//   if (backendOps.getAllItems) {
//     auto items = backendOps.getAllItems();
//     listWidget->clear();
//     for (const auto &item : items) {
//       listWidget->addItem(QString::fromStdString(item));
//     }
//   }
// }

void CardMan::onAddClicked() {
  QString text = inputLine->text().trimmed();
  if (!text.isEmpty() && backendOps.addVoice) {
    backendOps.addVoice(std::vector<float>());
    inputLine->clear();
  }
}

void CardMan::onRemoveClicked() {
  auto selected = listWidget->currentItem();
  if (selected && backendOps.removeVoice) {
    backendOps.removeVoice(0);
  }
}

void CardMan::onClearClicked() {
  if (backendOps.clearVoice) {
    backendOps.clearVoice();
  }
}
