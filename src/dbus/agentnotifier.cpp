#include "agentnotifier.h"
#include <QDebug>

AgentNotifier::AgentNotifier(QObject *parent)
    : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(QStringLiteral("org.slm.desktop.AuthAgent"))) {
        qWarning() << "AgentNotifier: could not register D-Bus service:"
                   << bus.lastError().message();
    }
    if (!bus.registerObject(QStringLiteral("/org/slm/desktop/AuthAgent"),
                            this,
                            QDBusConnection::ExportScriptableSignals)) {
        qWarning() << "AgentNotifier: could not register D-Bus object:"
                   << bus.lastError().message();
    } else {
        m_registered = true;
    }
}

void AgentNotifier::setAuthDialogActive(bool active)
{
    if (m_registered)
        emit AuthDialogActive(active);
}
