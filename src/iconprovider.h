#pragma once
#include <QQuickImageProvider>

// ThemeIconProvider — minimal XDG icon provider for the polkit agent.
// Resolves "image://icon/<name>" via QIcon::fromTheme().

class ThemeIconProvider : public QQuickImageProvider
{
public:
    ThemeIconProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
