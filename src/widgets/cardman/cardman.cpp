// frontend.cpp
#include "cardman.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <sys/socket.h>

CardMan::CardMan(QWidget *parent) : QWidget(parent) {
  // 创建UI
  auto *layout = new QVBoxLayout(this);

  scrollArea = new QScrollArea(this);
  cardsContainer = new QWidget();
  auto *cardsLayout = new QVBoxLayout(cardsContainer);
  cardsLayout->setAlignment(Qt::AlignTop);

  scrollArea->setWidgetResizable(true);
  scrollArea->setWidget(cardsContainer);
  scrollArea->setFrameShape(QFrame::NoFrame);

  inputLine = new QLineEdit(this);

  addButton = new QPushButton("Add Card", this);
  removeButton = new QPushButton("Remove Selected", this);
  clearButton = new QPushButton("Clear All", this);

  layout->addWidget(scrollArea);
  layout->addWidget(inputLine);
  layout->addWidget(addButton);
  layout->addWidget(removeButton);
  layout->addWidget(clearButton);

  connect(addButton, &QPushButton::clicked, this, &CardMan::onAddClicked);
  connect(removeButton, &QPushButton::clicked, this, &CardMan::onRemoveClicked);
  connect(clearButton, &QPushButton::clicked, this, &CardMan::onClearClicked);
}

CardMan::~CardMan() = default;

void CardMan::createCard(const QString &text) {
  auto *card = new QFrame(cardsContainer);
  card->setFrameShape(QFrame::StyledPanel);
  card->setLineWidth(1);
  card->setStyleSheet(
      "QFrame { background-color: white; border-radius: 5px; padding: 10px; }");

  auto *cardLayout = new QHBoxLayout(card);

  auto *label = new QLabel(text, card);
  cardLayout->addWidget(label);

  cardLayout->addStretch();

  auto *closeButton = new QPushButton("×", card);
  closeButton->setStyleSheet(
      "QPushButton { border: none; font-size: 16px; color: #999; }"
      "QPushButton:hover { color: #f00; }");
  closeButton->setFixedSize(20, 20);
  connect(closeButton, &QPushButton::clicked, [this, card]() {
    if (backendOps.removeVoice) {
      backendOps.removeVoice(0);
    }
  });
  cardLayout->addWidget(closeButton);

  dynamic_cast<QVBoxLayout *>(cardsContainer->layout())->addWidget(card);
  cardFrames.push_back(card);
}

void CardMan::clearAllCards() {
  for (auto card : cardFrames) {
    cardsContainer->layout()->removeWidget(card);
    delete card;
  }
  cardFrames.clear();
}

auto CardMan::getCallbacks() -> CardWidgetCallbacks {
  CardWidgetCallbacks callbacks;

  callbacks.onVoiceAdded = [this](const std::string &length) {
    QMetaObject::invokeMethod(
        this, [this, length]() { createCard(QString::fromStdString(length)); },
        Qt::QueuedConnection);
  };

  callbacks.onVoiceRemoved = [this](const int index) {
    QMetaObject::invokeMethod(
        this,
        [this, index]() {
          if (index >= 0 && index < static_cast<int>(cardFrames.size())) {
            cardsContainer->layout()->removeWidget(cardFrames[index]);
            delete cardFrames[index];
            cardFrames.erase(cardFrames.begin() + index);
          }
        },
        Qt::QueuedConnection);
  };

  callbacks.onVoiceCleared = [this]() {
    QMetaObject::invokeMethod(
        this, [this]() { clearAllCards(); }, Qt::QueuedConnection);
  };

  return callbacks;
}

void CardMan::setBackendOperations(const STTOperations &ops) {
  backendOps = ops;
}

void CardMan::onAddClicked() {
  QString text = inputLine->text().trimmed();
  if (!text.isEmpty() && backendOps.addVoice) {
    backendOps.addVoice(std::vector<float>());
    inputLine->clear();
  }
}

void CardMan::onRemoveClicked() {
  // 在卡片式界面中，移除操作通常通过每个卡片上的关闭按钮完成
  // 这里可以保留或移除这个功能
  if (backendOps.removeVoice) {
    backendOps.removeVoice(0);
  }
}

void CardMan::onClearClicked() {
  if (backendOps.clearVoice) {
    backendOps.clearVoice();
  }
}
