#include <QLayout>
#include <QRect>
#include <QStyle>

class FlowLayout : public QLayout {
public:
  explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1,
                      int vSpacing = -1);
  ~FlowLayout() override;

  void addItem(QLayoutItem *item) override;
  [[nodiscard]] auto horizontalSpacing() const -> int;
  [[nodiscard]] auto verticalSpacing() const -> int;
  [[nodiscard]] auto expandingDirections() const -> Qt::Orientations override;
  [[nodiscard]] auto hasHeightForWidth() const -> bool override;
  [[nodiscard]] auto heightForWidth(int) const -> int override;
  [[nodiscard]] auto count() const -> int override;
  [[nodiscard]] auto itemAt(int index) const -> QLayoutItem * override;
  [[nodiscard]] auto minimumSize() const -> QSize override;
  void setGeometry(const QRect &rect) override;
  [[nodiscard]] auto sizeHint() const -> QSize override;
  auto takeAt(int index) -> QLayoutItem * override;

private:
  [[nodiscard]] auto doLayout(const QRect &rect, bool testOnly) const -> int;
  [[nodiscard]] auto smartSpacing(QStyle::PixelMetric pm) const -> int;

  QList<QLayoutItem *> itemList;
  int m_hSpace;
  int m_vSpace;
};
