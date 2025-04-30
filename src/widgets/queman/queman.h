#ifndef QUEUEMANAGERWIDGET_H
#define QUEUEMANAGERWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
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
  virtual void mergeSelected() = 0;
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
  void mergeSelected() override;
  void clearQueue() override;

  [[nodiscard]] auto getListWidget() const -> QListWidget * {
    return listWidget;
  }

private:
  void setupUI();
  void updateListWidget();
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
  QList<QListWidgetItem *> selectedItems = listWidget->selectedItems();
  if (!selectedItems.isEmpty()) {
    // Sort indices in descending order to avoid shifting issues
    QList<int> indices;
    for (QListWidgetItem *item : selectedItems) {
      indices.append(listWidget->row(item));
    }
    std::ranges::sort(indices, std::greater<int>{});

    for (int index : indices) {
      if (index >= 0 && index < queue.size()) {
        queue.removeAt(index);
      }
    }
  } else {
    // 当没有选择任何项目时，找到触发按钮的对应行
    QPushButton *deleteBtn = qobject_cast<QPushButton *>(sender());
    if (deleteBtn) {
      // 找到按钮所在的itemWidget
      QWidget *itemWidget = qobject_cast<QWidget *>(deleteBtn->parent());
      if (itemWidget) {
        // 遍历所有items找到对应的行
        for (int i = 0; i < listWidget->count(); ++i) {
          if (listWidget->itemWidget(listWidget->item(i))) {
            if (listWidget->itemWidget(listWidget->item(i)) == itemWidget) {
              if (i >= 0 && i < queue.size()) {
                queue.removeAt(i);
              }
              break;
            }
          }
        }
      }
    }
  }

  updateListWidget();
  emit queueChanged();
}

template <typename T> void QueueManagerWidget<T>::mergeSelected() {
  QList<QListWidgetItem *> selectedItems = listWidget->selectedItems();
  if (selectedItems.size() < 2) {
    // Need at least 2 items to merge
    return;
  }

  // Sort indices in descending order for easier removal
  QList<int> indices;
  for (QListWidgetItem *item : selectedItems) {
    indices.append(listWidget->row(item));
  }
  std::ranges::sort(indices, std::greater<int>{});

  if (!mergeFunction) {
    return;
  }

  // Get the first item (will be at the position of the highest index after
  // sorting)
  T merged = queue[indices.last()];

  // Merge all other selected items into it
  for (int i = indices.size() - 2; i >= 0; --i) {
    int index = indices[i];
    merged = mergeFunction(merged, queue[index]);
  }

  // Remove all selected items except the first one (which we'll replace with
  // merged)
  int firstIndex = indices.last();
  for (int index : indices) {
    if (index != firstIndex) {
      queue.removeAt(index);
    }
  }

  // Replace the first selected item with the merged result
  queue[firstIndex] = merged;

  updateListWidget();
  emit queueChanged();
}

template <typename T> void QueueManagerWidget<T>::clearQueue() { clear(); }

template <typename T> void QueueManagerWidget<T>::setupUI() {
  auto *mainLayout = new QVBoxLayout(this);
  listWidget = new QListWidget(this);
  mainLayout->addWidget(listWidget);

  listWidget->setSelectionMode(
      QAbstractItemView::ExtendedSelection); // Changed to allow multiple
                                             // selection
  listWidget->setDragDropMode(QAbstractItemView::InternalMove);
  listWidget->setDefaultDropAction(Qt::MoveAction);

  connect(listWidget->model(), &QAbstractItemModel::rowsMoved, this, [this]() {
    QList<T> newQueue;
    for (int i = 0; i < listWidget->count(); ++i) {
      QListWidgetItem *item = listWidget->item(i);
      QWidget *widget = listWidget->itemWidget(item);
      auto *label = widget->findChild<QLabel *>();
      if (label) {
        QString text = label->text();
        for (const T &originalItem : queue) {
          if (itemToString(originalItem) == text) {
            newQueue.append(originalItem);
            break;
          }
        }
      }
    }
    queue = newQueue;
    emit queueChanged();
  });
}

template <typename T> void QueueManagerWidget<T>::updateListWidget() {
  listWidget->clear();
  for (int i = 0; i < queue.size(); ++i) {
    auto *itemWidget = new QWidget();
    auto *layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(4, 0, 4, 0);

    auto *label = new QLabel(itemToString(queue[i]));
    auto *deleteBtn = new QPushButton("删除");
    deleteBtn->setFixedSize(50, 24);
    auto *mergeBtn = new QPushButton("合并");
    mergeBtn->setFixedSize(50, 24);

    layout->addWidget(label);
    layout->addStretch();
    layout->addWidget(mergeBtn);
    layout->addWidget(deleteBtn);

    auto *item = new QListWidgetItem();
    listWidget->addItem(item);
    listWidget->setItemWidget(item, itemWidget);
    item->setSizeHint(itemWidget->sizeHint());

    // **关键修改：使用 QPalette 动态获取系统颜色**
    auto updateLabelColor = [label, item, this]() {
      const QPalette &palette = listWidget->palette();
      if (item->isSelected()) {
        label->setPalette(palette); // 使用系统选中颜色
        label->setForegroundRole(QPalette::HighlightedText);
      } else {
        label->setPalette(palette); // 使用系统普通文本颜色
        label->setForegroundRole(QPalette::Text);
      }
    };

    // 初始设置一次颜色
    updateLabelColor();

    // 监听选中状态变化
    QObject::connect(listWidget, &QListWidget::itemSelectionChanged, label,
                     updateLabelColor);

    connect(deleteBtn, &QPushButton::clicked, this,
            [this]() { deleteSelected(); });

    connect(mergeBtn, &QPushButton::clicked, this,
            [this]() { mergeSelected(); });
  }
}

#endif // QUEUEMANAGERWIDGET_H
