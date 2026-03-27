#include "slmpolkitagent.h"
#include "authsessionmanager.h"

#include <QDebug>

// Pull in GLib/Polkit — isolated to this translation unit only
#include <polkit/polkit.h>
#include <polkitagent/polkitagent.h>

// ── GObject subclass of PolkitAgentListener ───────────────────────────────
// We need a real GObject subtype because PolkitAgentListener is abstract GObject.
// Keep all Qt logic in SlmPolkitAgent; this struct is the minimal GLib glue.

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
    SlmListener *self = SLM_LISTENER(listener);
    self->priv->manager->initiateAuthentication(
        QString::fromUtf8(action_id),
        QString::fromUtf8(message),
        QString::fromUtf8(icon_name ? icon_name : ""),
        QString::fromUtf8(cookie),
        identities,
        cancellable,
        callback,
        user_data);
}

static gboolean slm_listener_initiate_authentication_finish(
    PolkitAgentListener  *listener,
    GAsyncResult         *res,
    GError              **error)
{
    Q_UNUSED(listener)
    return g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(res), error) == FALSE;
}

static void slm_listener_class_init(SlmListenerClass *klass)
{
    PolkitAgentListenerClass *listener_class = POLKIT_AGENT_LISTENER_CLASS(klass);
    listener_class->initiate_authentication        = slm_listener_initiate_authentication;
    listener_class->initiate_authentication_finish = slm_listener_initiate_authentication_finish;
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

    // Register for the current Unix session
    GError *error = nullptr;
    PolkitSubject *subject = polkit_unix_session_new_for_process_sync(
        getpid(), nullptr, &error);
    if (!subject) {
        qCritical() << "Failed to get session subject:" << (error ? error->message : "unknown");
        if (error) g_error_free(error);
        g_object_unref(m_listener);
        m_listener = nullptr;
        return false;
    }

    m_registeredAgent = polkit_agent_listener_register(
        m_listener,
        POLKIT_AGENT_REGISTER_FLAGS_NONE,
        subject,
        nullptr, // object_path — use default
        nullptr, // cancellable
        &error);
    g_object_unref(subject);

    if (!m_registeredAgent) {
        qCritical() << "Failed to register polkit agent:" << (error ? error->message : "unknown");
        if (error) g_error_free(error);
        g_object_unref(m_listener);
        m_listener = nullptr;
        return false;
    }

    qDebug() << "slm-polkit-agent registered successfully";
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
