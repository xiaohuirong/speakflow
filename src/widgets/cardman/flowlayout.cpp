#include "flowlayout.h"
#include <QtWidgets>

// 在 cardman.cpp 中添加 FlowLayout 实现
FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing) {
  setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout() {
  QLayoutItem *item;
  while ((item = takeAt(0)))
    delete item;
}

void FlowLayout::addItem(QLayoutItem *item) { itemList.append(item); }

auto FlowLayout::horizontalSpacing() const -> int {
  if (m_hSpace >= 0) {
    return m_hSpace;
  } else {
    return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
  }
}

auto FlowLayout::verticalSpacing() const -> int {
  if (m_vSpace >= 0) {
    return m_vSpace;
  } else {
    return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
  }
}

auto FlowLayout::count() const -> int { return itemList.size(); }

auto FlowLayout::itemAt(int index) const -> QLayoutItem * {
  return itemList.value(index);
}

auto FlowLayout::takeAt(int index) -> QLayoutItem * {
  if (index >= 0 && index < itemList.size())
    return itemList.takeAt(index);
  return nullptr;
}

auto FlowLayout::expandingDirections() const -> Qt::Orientations {
  return Qt::Horizontal;
}

auto FlowLayout::hasHeightForWidth() const -> bool { return true; }

auto FlowLayout::heightForWidth(int width) const -> int {
  return doLayout(QRect(0, 0, width, 0), true);
}

void FlowLayout::setGeometry(const QRect &rect) {
  QLayout::setGeometry(rect);
  doLayout(rect, false);
}

auto FlowLayout::sizeHint() const -> QSize { return minimumSize(); }

auto FlowLayout::minimumSize() const -> QSize {
  QSize size;
  for (const QLayoutItem *item :
       std::as_const(itemList)) // 使用 std::as_const 替代 qAsConst
    size = size.expandedTo(item->minimumSize());

  const QMargins margins = contentsMargins();
  size +=
      QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
  return size;
}

auto FlowLayout::doLayout(const QRect &rect, bool testOnly) const -> int {
  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
  int x = effectiveRect.x();
  int y = effectiveRect.y();
  int lineHeight = 0;

  for (QLayoutItem *item : itemList) {
    const QWidget *wid = item->widget();
    int spaceX = horizontalSpacing();
    if (spaceX == -1)
      spaceX = wid->style()->layoutSpacing(
          QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
    int spaceY = verticalSpacing();
    if (spaceY == -1)
      spaceY = wid->style()->layoutSpacing(
          QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
    int nextX = x + item->sizeHint().width() + spaceX;
    if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
      x = effectiveRect.x();
      y = y + lineHeight + spaceY;
      nextX = x + item->sizeHint().width() + spaceX;
      lineHeight = 0;
    }

    if (!testOnly)
      item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

    x = nextX;
    lineHeight = qMax(lineHeight, item->sizeHint().height());
  }
  return y + lineHeight - rect.y() + bottom;
}

auto FlowLayout::smartSpacing(QStyle::PixelMetric pm) const -> int {
  QObject *parent = this->parent();
  if (!parent) {
    return -1;
  } else if (parent->isWidgetType()) {
    auto *pw = static_cast<QWidget *>(parent);
    return pw->style()->pixelMetric(pm, nullptr, pw);
  } else {
    return static_cast<QLayout *>(parent)->spacing();
  }
}
