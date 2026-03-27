#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <sys/resource.h>

#include "slmpolkitagent.h"
#include "authdialogcontroller.h"
#include "authsessionmanager.h"
#include "dbus/agentnotifier.h"
#include "iconprovider.h"

int main(int argc, char *argv[])
{
    // Security: disable core dumps to prevent password residue in dump files.
    struct rlimit rl{};
    rl.rlim_cur = 0;
    rl.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &rl);

    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("SLM"));
    app.setApplicationName(QStringLiteral("slm-polkit-agent"));

    // Verify GLib event dispatcher is active.
    // polkit-agent-1 emits GLib signals (GObject signal system) which require
    // the GLib main context to be integrated into the Qt event loop.
    // On Linux, Qt defaults to QEventDispatcherGlib when compiled with GLib support.
    {
        const QByteArray cls = app.eventDispatcher()->metaObject()->className();
        if (!cls.contains("Glib")) {
            qCritical("slm-polkit-agent: event dispatcher is '%s', not GLib-based.\n"
                      "polkit GLib signals will not fire. "
                      "Rebuild Qt with GLib event loop support.", cls.constData());
            return 1;
        }
    }

    // Use the desktop icon theme so polkit action icons render correctly.
    QIcon::setFallbackThemeName(QStringLiteral("hicolor"));

    AgentNotifier        notifier;
    AuthDialogController dialogController;
    AuthSessionManager   sessionManager(&dialogController, &notifier);
    SlmPolkitAgent       agent(&sessionManager);

    if (!agent.registerAgent()) {
        qCritical("Failed to register polkit agent. "
                  "Is a graphical session active and polkitd running?");
        return 1;
    }

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("icon"), new ThemeIconProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("AuthDialog"), &dialogController);
    engine.loadFromModule(QStringLiteral("SlmPolkitAgent"), QStringLiteral("AuthDialog"));

    if (engine.rootObjects().isEmpty()) {
        qCritical("Failed to load AuthDialog QML root.");
        return 1;
    }

    return app.exec();
}
