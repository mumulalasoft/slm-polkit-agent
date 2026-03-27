#include "authsession.h"

#include <QDebug>

#include <polkit/polkit.h>
#include <polkitagent/polkitagent.h>
#include <gio/gio.h>

AuthSession::AuthSession(const QString &cookie,
                         void          *identity,
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
    m_session = polkit_agent_session_new(POLKIT_IDENTITY(identity),
                                         cookie.toUtf8().constData());
    g_signal_connect(m_session, "request",    G_CALLBACK(onRequest),    this);
    g_signal_connect(m_session, "show-error", G_CALLBACK(onShowError),  this);
    g_signal_connect(m_session, "show-info",  G_CALLBACK(onShowInfo),   this);
    g_signal_connect(m_session, "completed",  G_CALLBACK(onCompleted),  this);
}

AuthSession::~AuthSession()
{
    if (m_session) {
        // Disconnect ALL GLib signals pointing to this object before unref.
        // Without this, a late-firing signal would dereference a dead pointer.
        g_signal_handlers_disconnect_by_data(m_session, this);
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

    // Copy to QByteArray, respond, then zero — minimize time password is in memory.
    QByteArray buf = password.toUtf8();
    polkit_agent_session_response(m_session, buf.constData());
    buf.fill('\0');
    // Note: the original QString arg and QML's TextField.text are not zeroable from C++.
    // This is a known limitation of Qt string handling in authentication flows.
}

void AuthSession::cancel()
{
    if (m_state == State::Completed || m_state == State::Cancelled) return;
    m_state = State::Cancelled;

    polkit_agent_session_cancel(m_session);
    // polkit_agent_session_cancel may asynchronously fire "completed" — the state
    // guard in onCompleted() ensures we don't process it twice.

    completeTask(false, "Cancelled by user");
    emit completed(false);
}

// ── Private: GTask completion ─────────────────────────────────────────────

void AuthSession::completeTask(bool success, const char *errorMsg)
{
    GTask *task = g_task_new(nullptr, m_cancellable,
                             reinterpret_cast<GAsyncReadyCallback>(m_asyncCallback),
                             m_asyncUserData);
    if (success) {
        g_task_return_boolean(task, TRUE);
    } else {
        g_task_return_error(task,
            g_error_new(POLKIT_ERROR, POLKIT_ERROR_FAILED,
                        "%s", errorMsg ? errorMsg : "Authentication failed"));
    }
    g_object_unref(task);
}

// ── GLib signal callbacks — run on Qt main thread via QEventDispatcherGlib ─

void AuthSession::onRequest(PolkitAgentSession *, const char * /*req*/, gboolean /*echo*/, void *ud)
{
    // polkit PAM helper is requesting a password (or other input).
    // We always treat it as a password prompt — echo=false is the normal case.
    emit reinterpret_cast<AuthSession *>(ud)->requestPassword();
}

void AuthSession::onShowError(PolkitAgentSession *, const char *text, void *ud)
{
    emit reinterpret_cast<AuthSession *>(ud)->showError(QString::fromUtf8(text));
}

void AuthSession::onShowInfo(PolkitAgentSession *, const char *text, void *ud)
{
    emit reinterpret_cast<AuthSession *>(ud)->showInfo(QString::fromUtf8(text));
}

void AuthSession::onCompleted(PolkitAgentSession *, gboolean gainedAuth, void *ud)
{
    auto *self = reinterpret_cast<AuthSession *>(ud);

    // Guard: cancel() already called completeTask + emitted completed(false).
    if (self->m_state == State::Cancelled) return;

    self->m_state = gainedAuth ? State::Completed : State::Failed;
    self->completeTask(gainedAuth);
    emit self->completed(gainedAuth);
}
