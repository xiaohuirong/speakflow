#include "document.h"

void Document::setText(const QString &text) {
  if (text == m_text)
    return;
  m_text = text;
  emit textChanged(m_text);
}

void Document::appendText(const QString &text) {
  m_text += text;
  m_text += "\n";
  emit textChanged(m_text);
}
