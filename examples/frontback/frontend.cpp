// frontend.cpp
#include "frontend.h"

Frontend::Frontend(QWidget *parent) : QWidget(parent) {
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

  connect(addButton, &QPushButton::clicked, this, &Frontend::onAddClicked);
  connect(removeButton, &QPushButton::clicked, this,
          &Frontend::onRemoveClicked);
  connect(clearButton, &QPushButton::clicked, this, &Frontend::onClearClicked);
}

Frontend::~Frontend() = default;

auto Frontend::getCallbacks() -> FrontendCallbacks {
  FrontendCallbacks callbacks;

  callbacks.onItemAdded = [this](const std::string &item) {
    QMetaObject::invokeMethod(
        this,
        [this, item]() { listWidget->addItem(QString::fromStdString(item)); },
        Qt::QueuedConnection);
  };

  callbacks.onItemRemoved = [this](const std::string &item) {
    QMetaObject::invokeMethod(
        this,
        [this, item]() {
          auto items = listWidget->findItems(QString::fromStdString(item),
                                             Qt::MatchExactly);
          for (auto *itemWidget : items) {
            delete listWidget->takeItem(listWidget->row(itemWidget));
          }
        },
        Qt::QueuedConnection);
  };

  callbacks.onListCleared = [this]() {
    QMetaObject::invokeMethod(
        this, [this]() { listWidget->clear(); }, Qt::QueuedConnection);
  };

  return callbacks;
}

void Frontend::setBackendOperations(const BackendOperations &ops) {
  backendOps = ops;
  updateList();
}

void Frontend::updateList() {
  if (backendOps.getAllItems) {
    auto items = backendOps.getAllItems();
    listWidget->clear();
    for (const auto &item : items) {
      listWidget->addItem(QString::fromStdString(item));
    }
  }
}

void Frontend::onAddClicked() {
  QString text = inputLine->text().trimmed();
  if (!text.isEmpty() && backendOps.addItem) {
    backendOps.addItem(text.toStdString());
    inputLine->clear();
  }
}

void Frontend::onRemoveClicked() {
  auto selected = listWidget->currentItem();
  if (selected && backendOps.removeItem) {
    backendOps.removeItem(selected->text().toStdString());
  }
}

void Frontend::onClearClicked() {
  if (backendOps.clearList) {
    backendOps.clearList();
  }
}
