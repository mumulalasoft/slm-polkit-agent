#include "iconprovider.h"
#include <QIcon>
#include <QPainter>

ThemeIconProvider::ThemeIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap ThemeIconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    const int sz = (requestedSize.width() > 0) ? requestedSize.width() : 64;

    QIcon icon = QIcon::fromTheme(id);
    if (icon.isNull())
        icon = QIcon::fromTheme(QStringLiteral("dialog-password"));

    QPixmap px = icon.pixmap(sz, sz);
    if (size) *size = px.size();
    return px;
}
