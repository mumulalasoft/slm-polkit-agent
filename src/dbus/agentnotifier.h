#pragma once
#include <QObject>
#include <QDBusConnection>

// AgentNotifier — publishes org.slm.desktop.AuthAgent D-Bus interface.
//
// Allows the shell to know when a polkit dialog is active so it can
// apply visual effects (dim, blur, disable input) without tight coupling.
//
// Interface: org.slm.desktop.AuthAgent
// Object path: /org/slm/desktop/AuthAgent
// Signal: AuthDialogActive(bool active)

class AgentNotifier : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.slm.desktop.AuthAgent")

public:
    explicit AgentNotifier(QObject *parent = nullptr);

    void setAuthDialogActive(bool active);

signals:
    Q_SCRIPTABLE void AuthDialogActive(bool active);

private:
    bool m_registered = false;
};
