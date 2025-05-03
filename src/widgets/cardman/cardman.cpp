#include "cardman.h"
#include "events.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <algorithm>
#include <qpushbutton.h>

CardMan::CardMan(std::shared_ptr<EventBus> bus, QWidget *parent)
    : QWidget(parent), eventBus(std::move(bus)) {
  scrollArea = new QScrollArea(this);
  cardsContainer = new QWidget();
  cardsLayout = new FlowLayout(cardsContainer, 10, 10, 10);

  scrollArea->setWidgetResizable(true);
  scrollArea->setWidget(cardsContainer);
  scrollArea->setFrameShape(QFrame::NoFrame);

  auto *layout = new QVBoxLayout(this);
  setupControlButtons();
  layout->addWidget(scrollArea);
  layout->setContentsMargins(0, 0, 0, 0);

  eventBus->subscribe<AudioAddedEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto audioEvent = std::static_pointer_cast<AudioAddedEvent>(event);
        addCard(QString::number(audioEvent->audio[0], 'f', 0) + "S");
      });

  eventBus->subscribe<AudioRemovedEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto audioEvent = std::static_pointer_cast<AudioRemovedEvent>(event);
        removeCard(audioEvent->index);
      });

  eventBus->subscribe<AudioClearedEvent>(
      [this](const std::shared_ptr<Event> &event) { clearCards(); });

  eventBus->subscribe<AudioSentEvent>(
      [this](const std::shared_ptr<Event> &event) { clearCards(); });
}

void CardMan::setupControlButtons() {
  auto *controlPanel = new QWidget(this);
  auto *controlLayout = new QHBoxLayout(controlPanel);
  controlPanel->setLayout(controlLayout);
  controlLayout->setContentsMargins(10, 10, 10, 10);

  // Add record button with microphone icon
  recordButton = new QPushButton(controlPanel);
  recordButton->setIcon(QIcon::fromTheme("microphone-sensitivity-high"));
  // recordButton->setIconSize(QSize(24, 24));
  // recordButton->setFixedSize(50, 50);
  recordButton->setCheckable(true);

  // Add auto trigger checkbox with better styling
  autoTriggerCheckBox = new QCheckBox("Auto Mode", controlPanel);
  recordButton->setCheckable(false);

  sendButton = new QPushButton(controlPanel);
  sendButton->setText("发送");

  // Add spacer to push items to the left
  controlLayout->addWidget(recordButton);
  controlLayout->addSpacing(15);
  controlLayout->addWidget(sendButton);
  controlLayout->addSpacing(15);
  controlLayout->addWidget(autoTriggerCheckBox);
  controlLayout->addStretch();

  // 修改信号连接，改为点击切换状态
  connect(recordButton, &QPushButton::clicked, [this]() {
    if (autoTriggerCheckBox->isChecked())
      return;
    if (isRecording) {
      eventBus->publish<StartServiceEvent>("sentense");
    } else {
      eventBus->publish<StopServiceEvent>("sentense");
    }
    isRecording = !isRecording;
  });

  connect(sendButton, &QPushButton::clicked,
          [this]() { eventBus->publish<AudioSentEvent>(); });

  connect(autoTriggerCheckBox, &QCheckBox::checkStateChanged,
          [this](Qt::CheckState state) {
            if (state == Qt::Checked) {
              recordButton->setEnabled(false);
              eventBus->publish<AutoModeSetEvent>("stt", true);
            } else {
              recordButton->setEnabled(true);
              eventBus->publish<AutoModeSetEvent>("stt", false);
            }
          });

  // Add control panel to main layout
  this->layout()->addWidget(controlPanel);
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
    auto it = std::ranges::find(cardFrames, card);
    if (it != cardFrames.end()) {
      int index = std::distance(cardFrames.begin(), it);
      eventBus->publish<AudioRemovedEvent>(index);
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
