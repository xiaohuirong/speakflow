#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QObject>
#include <QString>

class Document : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString text MEMBER m_text NOTIFY textChanged FINAL)
public:
  explicit Document(QObject *parent = nullptr) : QObject(parent) {}

  void setText(const QString &text);
  void appendText(const QString &text);

signals:
  void textChanged(const QString &text);

private:
  QString m_text;
};

#endif // DOCUMENT_H
