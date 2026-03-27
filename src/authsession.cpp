#include "authsession.h"

#include <QDebug>
#include <cstring>

#include <polkit/polkit.h>
#include <polkitagent/polkitagent.h>
#include <gio/gio.h>

AuthSession::AuthSession(const QString &cookie,
                         void          *polkitIdentity,
                         GCancellable  *cancellable,
                         void          *asyncCallback,
                         void          *asyncUserData,
                         QObject       *parent)
    : QObject(parent)
    , m_cookie(cookie)
    , m_cancellable(cancellable ? G_CANCELLABLE(g_object_ref(cancellable)) : nullptr)
    , m_asyncCallback(asyncCallback)
    , m_asyncUserData(asyncUserData)
{
    m_session = polkit_agent_session_new(
        POLKIT_IDENTITY(polkitIdentity), cookie.toUtf8().constData());

    g_signal_connect(m_session, "request",    G_CALLBACK(onRequest),    this);
    g_signal_connect(m_session, "show-error", G_CALLBACK(onShowError),  this);
    g_signal_connect(m_session, "show-info",  G_CALLBACK(onShowInfo),   this);
    g_signal_connect(m_session, "completed",  G_CALLBACK(onCompleted),  this);
}

AuthSession::~AuthSession()
{
    if (m_session) {
        g_object_unref(m_session);
        m_session = nullptr;
    }
    if (m_cancellable) {
        g_object_unref(m_cancellable);
        m_cancellable = nullptr;
    }
}

void AuthSession::start()
{
    if (m_state != State::Idle) return;
    m_state = State::Authenticating;
    polkit_agent_session_initiate(m_session);
}

void AuthSession::respond(const QString &password)
{
    if (m_state != State::Authenticating) return;

    QByteArray buf = password.toUtf8();
    polkit_agent_session_response(m_session, buf.constData());

    // Zero the buffer immediately — minimize time sensitive data is in memory
    buf.fill('\0');
}

void AuthSession::cancel()
{
    if (m_state == State::Completed || m_state == State::Cancelled) return;
    m_state = State::Cancelled;
    polkit_agent_session_cancel(m_session);

    // Complete the GAsync result so the polkit caller gets a response
    GSimpleAsyncResult *result = g_simple_async_result_new(
        nullptr, reinterpret_cast<GAsyncReadyCallback>(m_asyncCallback),
        m_asyncUserData, nullptr);
    g_simple_async_result_set_error(result, POLKIT_ERROR,
                                    POLKIT_ERROR_CANCELLED, "Cancelled by user");
    g_simple_async_result_complete(result);
    g_object_unref(result);

    emit completed(false);
}

// ── GLib signal callbacks ─────────────────────────────────────────────────

void AuthSession::onRequest(PolkitAgentSession *, const char *request, gboolean echoOn, void *userData)
{
    Q_UNUSED(request); Q_UNUSED(echoOn)
    // polkit is asking for the password — forward to UI
    emit reinterpret_cast<AuthSession *>(userData)->requestPassword();
}

void AuthSession::onShowError(PolkitAgentSession *, const char *text, void *userData)
{
    emit reinterpret_cast<AuthSession *>(userData)->showError(QString::fromUtf8(text));
}

void AuthSession::onShowInfo(PolkitAgentSession *, const char *text, void *userData)
{
    emit reinterpret_cast<AuthSession *>(userData)->showInfo(QString::fromUtf8(text));
}

void AuthSession::onCompleted(PolkitAgentSession *, gboolean gainedAuth, void *userData)
{
    auto *self = reinterpret_cast<AuthSession *>(userData);
    self->m_state = gainedAuth ? State::Completed : State::Failed;

    GSimpleAsyncResult *result = g_simple_async_result_new(
        nullptr,
        reinterpret_cast<GAsyncReadyCallback>(self->m_asyncCallback),
        self->m_asyncUserData,
        nullptr);
    if (!gainedAuth) {
        g_simple_async_result_set_error(result, POLKIT_ERROR,
                                        POLKIT_ERROR_FAILED, "Authentication failed");
    }
    g_simple_async_result_complete(result);
    g_object_unref(result);

    emit self->completed(gainedAuth);
}
