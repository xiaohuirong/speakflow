#ifndef QUEUEMANAGERWIDGET_H
#define QUEUEMANAGERWIDGET_H

#include <QListWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>

class QueueManagerBase : public QWidget {
  Q_OBJECT
public:
  explicit QueueManagerBase(QWidget *parent = nullptr);

  virtual void clear() = 0;
  [[nodiscard]] virtual auto count() const -> int = 0;
  [[nodiscard]] virtual auto itemText(int index) const -> QString = 0;
  virtual void deleteAtPosition(int index) = 0;
  virtual void mergeItems(int start, int count) = 0;
  virtual void moveItem(int index, int distance) = 0;
  virtual void moveToFront(int index) = 0;
  virtual void insertAtPosition(int index, const QString &value) = 0;
  virtual void append(const QString &value) = 0;

public slots:
  virtual void deleteSelected() = 0;
  virtual void clearQueue() = 0;

signals:
  void queueChanged();
};

template <typename T> class QueueManagerWidget : public QueueManagerBase {
public:
  explicit QueueManagerWidget(QWidget *parent = nullptr);

  void setItems(const QList<T> &items);
  [[nodiscard]] auto getItems() const -> QList<T>;
  void setMergeFunction(std::function<T(const T &, const T &)> func);
  void setDisplayFunction(std::function<QString(const T &)> func);

  // Implement pure virtual functions
  void clear() override;
  [[nodiscard]] auto count() const -> int override;
  [[nodiscard]] auto itemText(int index) const -> QString override;
  void deleteAtPosition(int index) override;
  void mergeItems(int start, int count) override;
  void moveItem(int index, int distance) override;
  void moveToFront(int index) override;
  void insertAtPosition(int index, const QString &value) override;
  void append(const QString &value) override;

  void deleteSelected() override;
  void clearQueue() override;

  QListWidget *getListWidget() const { return listWidget; }

private:
  void setupUI();
  void updateListWidget(); // Add this declaration
  [[nodiscard]] inline auto itemToString(const T &item) const -> QString {
    if (displayFunction) {
      return displayFunction(item);
    }
    return QString::number(item);
  }

  QListWidget *listWidget{};
  QList<T> queue;
  std::function<T(const T &, const T &)> mergeFunction;
  std::function<QString(const T &)> displayFunction;
};

template <>
inline auto QueueManagerWidget<QString>::itemToString(const QString &item) const
    -> QString {
  if (displayFunction) {
    return displayFunction(item);
  }
  return item;
}

template <typename T>
QueueManagerWidget<T>::QueueManagerWidget(QWidget *parent)
    : QueueManagerBase(parent) {
  setupUI();
}

template <typename T>
void QueueManagerWidget<T>::setItems(const QList<T> &items) {
  queue = items;
  updateListWidget();
}

template <typename T> auto QueueManagerWidget<T>::getItems() const -> QList<T> {
  return queue;
}

template <typename T>
void QueueManagerWidget<T>::setMergeFunction(
    std::function<T(const T &, const T &)> func) {
  mergeFunction = func;
}

template <typename T>
void QueueManagerWidget<T>::setDisplayFunction(
    std::function<QString(const T &)> func) {
  displayFunction = func;
  updateListWidget();
}

template <typename T> void QueueManagerWidget<T>::clear() {
  queue.clear();
  updateListWidget();
  emit queueChanged();
}

template <typename T> auto QueueManagerWidget<T>::count() const -> int {
  return queue.size();
}

template <typename T>
auto QueueManagerWidget<T>::itemText(int index) const -> QString {
  if (index >= 0 && index < queue.size()) {
    return itemToString(queue.at(index));
  }
  return {};
}

template <typename T> void QueueManagerWidget<T>::deleteAtPosition(int index) {
  if (index >= 0 && index < queue.size()) {
    queue.removeAt(index);
    updateListWidget();
    emit queueChanged();
  }
}

template <typename T>
void QueueManagerWidget<T>::mergeItems(int start, int count) {
  if (start < 0 || count < 2 || start + count > queue.size()) {
    return;
  }

  if (!mergeFunction) {
    return;
  }

  T merged = queue[start];
  for (int i = 1; i < count; ++i) {
    merged = mergeFunction(merged, queue[start + i]);
  }

  for (int i = 0; i < count - 1; ++i) {
    queue.removeAt(start + 1);
  }

  queue[start] = merged;
  updateListWidget();
  emit queueChanged();
}

template <typename T>
void QueueManagerWidget<T>::moveItem(int index, int distance) {
  int newPos = index + distance;
  if (index >= 0 && newPos >= 0 && newPos < queue.size()) {
    queue.move(index, newPos);
    updateListWidget();
    emit queueChanged();
  }
}

template <typename T> void QueueManagerWidget<T>::moveToFront(int index) {
  if (index > 0 && index < queue.size()) {
    queue.move(index, 0);
    updateListWidget();
    emit queueChanged();
  }
}

template <typename T>
void QueueManagerWidget<T>::insertAtPosition(int index, const QString &value) {
  if (index >= 0 && index <= queue.size()) {
    queue.insert(index, value);
    updateListWidget();
    emit queueChanged();
  }
}

template <typename T> void QueueManagerWidget<T>::append(const QString &value) {
  queue.append(value);
  updateListWidget();
  emit queueChanged();
}

template <typename T> void QueueManagerWidget<T>::deleteSelected() {
  deleteAtPosition(listWidget->currentRow());
}

template <typename T> void QueueManagerWidget<T>::clearQueue() { clear(); }

template <typename T> void QueueManagerWidget<T>::setupUI() {
  auto *mainLayout = new QVBoxLayout(this);
  listWidget = new QListWidget(this);
  mainLayout->addWidget(listWidget);
}

template <typename T> void QueueManagerWidget<T>::updateListWidget() {
  listWidget->clear();
  for (const T &item : queue) {
    listWidget->addItem(itemToString(item));
  }
}

#endif // QUEUEMANAGERWIDGET_H
