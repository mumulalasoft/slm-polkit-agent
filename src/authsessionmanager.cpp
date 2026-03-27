#include "authsessionmanager.h"
#include "authsession.h"
#include "authdialogcontroller.h"
#include "dbus/agentnotifier.h"

#include <QDebug>

#include <polkit/polkit.h>
#include <gio/gio.h>
#include <pwd.h>
#include <grp.h>

// ── Helpers ────────────────────────────────────────────────────────────────

static QString resolveIdentityName(void *identity)
{
    if (POLKIT_IS_UNIX_USER(identity)) {
        uid_t uid = polkit_unix_user_get_uid(POLKIT_UNIX_USER(identity));
        if (struct passwd *pw = getpwuid(uid))
            return QString::fromLocal8Bit(pw->pw_name);
        return QStringLiteral("uid:%1").arg(uid);
    }
    if (POLKIT_IS_UNIX_GROUP(identity)) {
        gid_t gid = polkit_unix_group_get_gid(POLKIT_UNIX_GROUP(identity));
        if (struct group *gr = getgrgid(gid))
            return QString::fromLocal8Bit(gr->gr_name);
        return QStringLiteral("gid:%1").arg(gid);
    }
    return QStringLiteral("Unknown");
}

// ── AuthSessionManager ─────────────────────────────────────────────────────

AuthSessionManager::AuthSessionManager(AuthDialogController *controller,
                                       AgentNotifier        *notifier,
                                       QObject              *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_notifier(notifier)
    , m_timeout(new QTimer(this))
{
    m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout, this, &AuthSessionManager::onTimeout);

    connect(m_controller, &AuthDialogController::authenticate,
            this, &AuthSessionManager::onUserAuthenticate);
    connect(m_controller, &AuthDialogController::cancel,
            this, &AuthSessionManager::onUserCancel);
}

AuthSessionManager::~AuthSessionManager()
{
    // Unref GObjects for any queued requests that were never started.
    while (!m_queue.isEmpty()) {
        PendingRequest req = m_queue.dequeue();
        if (req.identity)    g_object_unref(req.identity);
        if (req.cancellable) g_object_unref(req.cancellable);
    }
    delete m_current;
}

void AuthSessionManager::initiateAuthentication(const QString &actionId,
                                                const QString &message,
                                                const QString &iconName,
                                                const QString &cookie,
                                                GList         *identities,
                                                GCancellable  *cancellable,
                                                void          *asyncCallback,
                                                void          *asyncUserData)
{
    // Validate identities immediately — GList is only valid during this call.
    if (!identities || !identities->data) {
        qWarning("Auth request with no identities — failing immediately.");
        failRequest(asyncCallback, asyncUserData, cancellable,
                    "No identities provided by polkitd");
        return;
    }

    // Extract the first identity. g_object_ref so it survives queuing.
    void *identity = g_object_ref(G_OBJECT(identities->data));
    const QString identityName = resolveIdentityName(identity);

    PendingRequest req{
        actionId, message, iconName, cookie,
        identityName,
        identity,
        cancellable ? G_CANCELLABLE(g_object_ref(cancellable)) : nullptr,
        asyncCallback,
        asyncUserData
    };

    if (m_current) {
        m_queue.enqueue(req);
        qDebug() << "Auth request queued for" << identityName
                 << "— queue size:" << m_queue.size();
    } else {
        startRequest(req);
    }
}

void AuthSessionManager::startRequest(PendingRequest &req)
{
    m_current = new AuthSession(req.cookie, req.identity, req.cancellable,
                                req.asyncCallback, req.asyncUserData, this);

    // AuthSession now has its own refs (polkit_agent_session_new refs identity;
    // AuthSession constructor refs cancellable). Release our copies.
    g_object_unref(req.identity);
    req.identity = nullptr;
    if (req.cancellable) {
        g_object_unref(req.cancellable);
        req.cancellable = nullptr;
    }

    connect(m_current, &AuthSession::completed,
            this, &AuthSessionManager::onSessionCompleted);
    connect(m_current, &AuthSession::showError,
            m_controller, &AuthDialogController::setError);
    connect(m_current, &AuthSession::showInfo,
            m_controller, &AuthDialogController::setInfo);
    connect(m_current, &AuthSession::requestPassword,
            m_controller, &AuthDialogController::showPasswordPrompt);

    m_controller->presentRequest(req.actionId, req.message,
                                 req.iconName, req.identityName);
    m_notifier->setAuthDialogActive(true);
    m_timeout->start(kTimeoutSeconds * 1000);
    m_current->start();
}

void AuthSessionManager::failRequest(void *asyncCallback, void *asyncUserData,
                                     GCancellable *cancellable, const char *reason)
{
    GTask *task = g_task_new(nullptr, cancellable,
                             reinterpret_cast<GAsyncReadyCallback>(asyncCallback),
                             asyncUserData);
    g_task_return_error(task,
        g_error_new(POLKIT_ERROR, POLKIT_ERROR_FAILED, "%s", reason));
    g_object_unref(task);
}

// ── Slots ──────────────────────────────────────────────────────────────────

void AuthSessionManager::onSessionCompleted(bool /*gainedAuth*/)
{
    finishCurrent();
}

void AuthSessionManager::onTimeout()
{
    qWarning("Auth session timed out after %d seconds.", kTimeoutSeconds);
    if (m_current)
        m_current->cancel();
}

void AuthSessionManager::onUserAuthenticate(const QString &password)
{
    if (m_current)
        m_current->respond(password);
}

void AuthSessionManager::onUserCancel()
{
    if (m_current)
        m_current->cancel();
}

void AuthSessionManager::finishCurrent()
{
    m_timeout->stop();

    delete m_current;
    m_current = nullptr;

    m_controller->hide();
    m_notifier->setAuthDialogActive(false);

    if (!m_queue.isEmpty()) {
        PendingRequest req = m_queue.dequeue();
        startRequest(req);
    }
}
