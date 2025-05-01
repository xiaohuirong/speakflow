#include "cardman.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

CardMan::CardMan(QWidget *parent) : QWidget(parent) {
  scrollArea = new QScrollArea(this);
  cardsContainer = new QWidget();
  cardsLayout = new FlowLayout(cardsContainer, 10, 10, 10);

  scrollArea->setWidgetResizable(true);
  scrollArea->setWidget(cardsContainer);
  scrollArea->setFrameShape(QFrame::NoFrame);

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(scrollArea);
  layout->setContentsMargins(0, 0, 0, 0);
}

CardMan::~CardMan() = default;

void CardMan::createCard(const QString &text) {
  auto *card = new QFrame(cardsContainer);
  card->setFixedWidth(120);
  card->setMinimumHeight(20);
  card->setFrameShape(QFrame::StyledPanel);
  card->setLineWidth(1);
  card->setStyleSheet("QFrame {"
                      "   background-color: palette(highlight);"
                      "   color: palette(highlightedText);"
                      "   border-radius: 5px;"
                      "}");

  auto *cardLayout = new QHBoxLayout(card);

  auto *audioIcon = new QLabel(card);
  audioIcon->setPixmap(QIcon::fromTheme("audio-volume-high").pixmap(20, 20));
  cardLayout->addWidget(audioIcon);

  auto *label = new QLabel(text, card);
  label->setWordWrap(true);
  cardLayout->addWidget(label);

  cardLayout->addStretch();

  auto *closeButton = new QPushButton("X", card);
  closeButton->setStyleSheet(
      "QPushButton { border: none; font-size: 16px; color: #999; }"
      "QPushButton:hover { color: #f00; }");
  closeButton->setFixedSize(20, 20);
  connect(closeButton, &QPushButton::clicked, [this, card]() {
    auto it = std::find(cardFrames.begin(), cardFrames.end(), card);
    if (it != cardFrames.end() && backendOps.removeVoice) {
      int index = std::distance(cardFrames.begin(), it);
      backendOps.removeVoice(index);
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
  return {.onVoiceAdded =
              [this](const std::string &length) {
                addCard(QString::fromStdString(length) + "S");
              },
          .onVoiceRemoved = [this](int index) { removeCard(index); },
          .onVoiceCleared = [this]() { clearCards(); }};
}

void CardMan::setBackendOperations(const STTOperations &ops) {
  backendOps = ops;
}

void CardMan::addCard(const QString &text) {
  QMetaObject::invokeMethod(
      this, [this, text]() { createCard(text); }, Qt::QueuedConnection);
}

void CardMan::removeCard(int index) {
  QMetaObject::invokeMethod(
      this,
      [this, index]() {
        if (index >= 0 && index < static_cast<int>(cardFrames.size())) {
          QFrame *card = cardFrames[index];
          cardsLayout->removeWidget(card);
          delete card;
          cardFrames.erase(cardFrames.begin() + index);
        }
      },
      Qt::QueuedConnection);
}

void CardMan::clearCards() {
  QMetaObject::invokeMethod(
      this, [this]() { clearAllCards(); }, Qt::QueuedConnection);
}
