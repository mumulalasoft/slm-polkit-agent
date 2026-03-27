#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <sys/resource.h>

#include "slmpolkitagent.h"
#include "authdialogcontroller.h"
#include "authsessionmanager.h"
#include "dbus/agentnotifier.h"

int main(int argc, char *argv[])
{
    // Security: disable core dumps to prevent password leaks
    struct rlimit rl{};
    rl.rlim_cur = 0;
    rl.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &rl);

    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("SLM"));
    app.setApplicationName(QStringLiteral("slm-polkit-agent"));

    AgentNotifier notifier;
    AuthDialogController dialogController;
    AuthSessionManager sessionManager(&dialogController, &notifier);
    SlmPolkitAgent agent(&sessionManager);

    if (!agent.registerAgent()) {
        qCritical("Failed to register polkit agent. Is a session active?");
        return 1;
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("AuthDialog"), &dialogController);
    engine.loadFromModule(QStringLiteral("SlmPolkitAgent"), QStringLiteral("AuthDialog"));

    return app.exec();
}
