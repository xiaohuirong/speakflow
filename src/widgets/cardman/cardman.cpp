#include "cardman.h"
#include "events.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <algorithm>
#include <memory>
#include <qpushbutton.h>
#include <spdlog/spdlog.h>

CardMan::CardMan(QWidget *parent) : QWidget(parent) {
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
}

void CardMan::setEventBus(std::shared_ptr<EventBus> bus) {
  eventBus = std::move(bus);

  eventBus->subscribe<AudioAddedEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto audioEvent = std::static_pointer_cast<AudioAddedEvent>(event);
        addCard(QString::number(audioEvent->audio.size() / 16000.0, 'f', 1) +
                "S");
      });

  eventBus->subscribe<AudioRemovedEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto audioEvent = std::static_pointer_cast<AudioRemovedEvent>(event);
        removeCard(audioEvent->index);
      });

  eventBus->subscribe<AudioClearedEvent>(
      [this](const std::shared_ptr<Event> &event) { clearCards(); });

  eventBus->subscribe<ServiceStatusEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto serviceEvent = std::static_pointer_cast<ServiceStatusEvent>(event);

        spdlog::info("receive service status event {}",
                     serviceEvent->serviceName);
        if (serviceEvent->serviceName == "sentense") {
          if (serviceEvent->isRunning) {
            isRecording = true;
            recordButton.setChecked(true);
            spdlog::info("set to true.");
          } else {
            isRecording = false;
            recordButton.setChecked(false);
            spdlog::info("set to false.");
          }
        }
      });

  eventBus->publish<AutoModeSetEvent>("stt", false);
}

void CardMan::setupControlButtons() {
  auto *controlPanel = new QWidget(this);
  auto *controlLayout = new QHBoxLayout(controlPanel);
  controlPanel->setLayout(controlLayout);
  controlLayout->setContentsMargins(10, 10, 10, 10);

  // Add record button with microphone icon
  recordButton.setIcon(QIcon::fromTheme("microphone-sensitivity-high"));
  recordButton.setIconSize(QSize(24, 24));
  // recordButton->setFixedSize(50, 50);
  recordButton.setCheckable(true);
  recordButton.setChecked(false);
  recordButton.setFocusPolicy(Qt::NoFocus);

  // Add auto trigger checkbox with better styling
  autoTriggerCheckBox = new QCheckBox("Auto Mode");
  autoTriggerCheckBox->setFocusPolicy(Qt::NoFocus);

  sendButton.setText("发送");
  sendButton.setCheckable(false);
  sendButton.setFocusPolicy(Qt::NoFocus);

  clearButton.setText("清除");

  // Add spacer to push items to the left
  controlLayout->addWidget(&recordButton);
  controlLayout->addSpacing(15);
  controlLayout->addWidget(&sendButton);
  controlLayout->addSpacing(15);
  controlLayout->addWidget(&clearButton);
  controlLayout->addSpacing(15);
  controlLayout->addWidget(autoTriggerCheckBox);
  controlLayout->addStretch();

  // 修改信号连接，改为点击切换状态
  connect(&recordButton, &QPushButton::clicked, [this]() {
    if (isRecording) {
      eventBus->publish<StopServiceEvent>("sentense");
    } else {
      eventBus->publish<StartServiceEvent>("sentense");
    }
  });

  connect(&sendButton, &QPushButton::clicked,
          [this]() { eventBus->publish<AudioSentEvent>(); });

  connect(autoTriggerCheckBox, &QCheckBox::checkStateChanged,
          [this](Qt::CheckState state) {
            if (state == Qt::Checked) {
              sendButton.setEnabled(false);
              eventBus->publish<AutoModeSetEvent>("stt", true);
            } else {
              sendButton.setEnabled(true);
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
