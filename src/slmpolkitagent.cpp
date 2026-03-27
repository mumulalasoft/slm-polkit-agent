#include "slmpolkitagent.h"
#include "authsessionmanager.h"

#include <QDebug>
#include <unistd.h>  // getpid()

// GLib/Polkit — isolated to this translation unit only.
#include <polkit/polkit.h>
#include <polkitagent/polkitagent.h>
#include <gio/gio.h>

// ── GObject subclass of PolkitAgentListener ───────────────────────────────
// Minimal C/GLib glue. All business logic lives in AuthSessionManager (Qt).

struct SlmListenerPrivate {
    AuthSessionManager *manager = nullptr;
};

#define SLM_TYPE_LISTENER (slm_listener_get_type())
G_DECLARE_FINAL_TYPE(SlmListener, slm_listener, SLM, LISTENER, PolkitAgentListener)

struct _SlmListener {
    PolkitAgentListener parent;
    SlmListenerPrivate *priv;
};

G_DEFINE_TYPE(SlmListener, slm_listener, POLKIT_AGENT_TYPE_LISTENER)

static void slm_listener_initiate_authentication(
    PolkitAgentListener  *listener,
    const gchar          *action_id,
    const gchar          *message,
    const gchar          *icon_name,
    PolkitDetails        *details,
    const gchar          *cookie,
    GList                *identities,
    GCancellable         *cancellable,
    GAsyncReadyCallback   callback,
    gpointer              user_data)
{
    Q_UNUSED(details)
    SlmListener *self = SLM_LISTENER(listener);
    self->priv->manager->initiateAuthentication(
        QString::fromUtf8(action_id),
        QString::fromUtf8(message),
        QString::fromUtf8(icon_name ? icon_name : ""),
        QString::fromUtf8(cookie),
        identities,
        cancellable,
        reinterpret_cast<void *>(callback),
        user_data);
}

static gboolean slm_listener_initiate_authentication_finish(
    PolkitAgentListener  *listener,
    GAsyncResult         *res,
    GError              **error)
{
    Q_UNUSED(listener)
    // res is a GTask created by AuthSession or AuthSessionManager.
    return g_task_propagate_boolean(G_TASK(res), error);
}

static void slm_listener_finalize(GObject *object)
{
    SlmListener *self = SLM_LISTENER(object);
    delete self->priv;
    self->priv = nullptr;
    G_OBJECT_CLASS(slm_listener_parent_class)->finalize(object);
}

static void slm_listener_class_init(SlmListenerClass *klass)
{
    G_OBJECT_CLASS(klass)->finalize = slm_listener_finalize;

    PolkitAgentListenerClass *lc = POLKIT_AGENT_LISTENER_CLASS(klass);
    lc->initiate_authentication        = slm_listener_initiate_authentication;
    lc->initiate_authentication_finish = slm_listener_initiate_authentication_finish;
}

static void slm_listener_init(SlmListener *self)
{
    self->priv = new SlmListenerPrivate;
}

// ── SlmPolkitAgent (Qt side) ──────────────────────────────────────────────

SlmPolkitAgent::SlmPolkitAgent(AuthSessionManager *manager, QObject *parent)
    : QObject(parent), m_manager(manager)
{
}

SlmPolkitAgent::~SlmPolkitAgent()
{
    unregisterAgent();
}

bool SlmPolkitAgent::registerAgent()
{
    SlmListener *listener = SLM_LISTENER(g_object_new(SLM_TYPE_LISTENER, nullptr));
    listener->priv->manager = m_manager;
    m_listener = POLKIT_AGENT_LISTENER(listener);

    // Prefer XDG_SESSION_ID (systemd/logind) — faster and more reliable than
    // the _sync variant which talks to logind over D-Bus.
    GError *error = nullptr;
    PolkitSubject *subject = nullptr;

    const QByteArray sessionId = qgetenv("XDG_SESSION_ID");
    if (!sessionId.isEmpty()) {
        subject = polkit_unix_session_new(sessionId.constData());
    } else {
        subject = polkit_unix_session_new_for_process_sync(getpid(), nullptr, &error);
        if (!subject) {
            qCritical() << "Failed to determine session subject:"
                        << (error ? error->message : "unknown");
            if (error) g_error_free(error);
            g_object_unref(m_listener);
            m_listener = nullptr;
            return false;
        }
    }

    m_registeredAgent = polkit_agent_listener_register(
        m_listener,
        POLKIT_AGENT_REGISTER_FLAGS_NONE,
        subject,
        nullptr, // object_path — use polkit default
        nullptr, // cancellable
        &error);
    g_object_unref(subject);

    if (!m_registeredAgent) {
        qCritical() << "Failed to register polkit agent:"
                    << (error ? error->message : "unknown");
        if (error) g_error_free(error);
        g_object_unref(m_listener);
        m_listener = nullptr;
        return false;
    }

    qInfo("slm-polkit-agent registered successfully.");
    return true;
}

void SlmPolkitAgent::unregisterAgent()
{
    if (m_registeredAgent) {
        polkit_agent_listener_unregister(m_registeredAgent);
        m_registeredAgent = nullptr;
    }
    if (m_listener) {
        g_object_unref(m_listener);
        m_listener = nullptr;
    }
}
