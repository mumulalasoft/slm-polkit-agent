#include "authsessionmanager.h"
#include "authsession.h"
#include "authdialogcontroller.h"
#include "dbus/agentnotifier.h"

#include <QDebug>
#include <polkit/polkit.h>

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
    PendingRequest req{ actionId, message, iconName, cookie,
                        identities, cancellable, asyncCallback, asyncUserData };

    if (m_current) {
        m_queue.enqueue(req);
        qDebug() << "Auth request queued. Queue size:" << m_queue.size();
    } else {
        startRequest(req);
    }
}

void AuthSessionManager::startRequest(const PendingRequest &req)
{
    // Pick the first identity (typically the local user or admin)
    void *identity = req.identities ? req.identities->data : nullptr;
    if (!identity) {
        qWarning() << "Auth request with no identities — aborting";
        return;
    }

    // Resolve display name for the identity
    QString identityName;
    if (polkit_identity_get_user_name(POLKIT_IDENTITY(identity))) {
        identityName = QString::fromUtf8(
            polkit_identity_get_user_name(POLKIT_IDENTITY(identity)));
    }

    m_current = new AuthSession(req.cookie, identity, req.cancellable,
                                req.asyncCallback, req.asyncUserData, this);
    connect(m_current, &AuthSession::completed,
            this, &AuthSessionManager::onSessionCompleted);
    connect(m_current, &AuthSession::showError, m_controller,
            &AuthDialogController::setError);
    connect(m_current, &AuthSession::showInfo,  m_controller,
            &AuthDialogController::setInfo);
    connect(m_current, &AuthSession::requestPassword, m_controller,
            &AuthDialogController::showPasswordPrompt);

    m_controller->presentRequest(req.actionId, req.message, req.iconName, identityName);
    m_notifier->setAuthDialogActive(true);
    m_timeout->start(kTimeoutSeconds * 1000);
    m_current->start();
}

void AuthSessionManager::onSessionCompleted(bool gainedAuth)
{
    Q_UNUSED(gainedAuth)
    finishCurrent();
}

void AuthSessionManager::onTimeout()
{
    qWarning() << "Auth session timed out";
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

    if (!m_queue.isEmpty())
        startRequest(m_queue.dequeue());
}
