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
  cardsLayout = new FlowLayout(cardsContainer, 10, 10, 10); // 使用 FlowLayout

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
  card->setFixedWidth(150);   // 设置固定宽度
  card->setMinimumHeight(20); // 设置最小高度
  card->setFrameShape(QFrame::StyledPanel);
  card->setLineWidth(1);
  card->setStyleSheet(
      "QFrame {"
      "   background-color: palette(highlight);" // 使用系统高亮色
      "   color: palette(highlightedText);" // 使用高亮文本色（确保文字可见）
      "   border-radius: 5px;"
      "   padding: 5px;"
      "}");

  auto *cardLayout = new QHBoxLayout(card);

  auto *label = new QLabel(text, card);
  label->setWordWrap(true);
  cardLayout->addWidget(label);

  cardLayout->addStretch();

  auto *closeButton = new QPushButton("×", card);
  closeButton->setStyleSheet(
      "QPushButton { border: none; font-size: 16px; color: #999; }"
      "QPushButton:hover { color: #f00; }");
  closeButton->setFixedSize(20, 20);
  connect(closeButton, &QPushButton::clicked, [this, card]() {
    if (backendOps.removeVoice) {
      // 使用 std::find 查找卡片索引
      auto it = std::find(cardFrames.begin(), cardFrames.end(), card);
      if (it != cardFrames.end()) {
        int index = std::distance(cardFrames.begin(), it);
        backendOps.removeVoice(index);
      }
    }
  });
  cardLayout->addWidget(closeButton);

  cardsLayout->addWidget(card);
  cardFrames.push_back(card);
}

void CardMan::clearAllCards() {
  for (auto card : cardFrames) {
    cardsLayout->removeWidget(card);
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
            QFrame *card = cardFrames[index];
            cardsLayout->removeWidget(card);
            delete card;
            cardFrames.erase(cardFrames.begin() +
                             index); // 使用 erase 替代 remove
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
    backendOps.addVoice(std::vector<float>(1, text.toFloat()));
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
