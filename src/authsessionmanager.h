#pragma once
#include <QObject>
#include <QQueue>
#include <QString>
#include <QTimer>

class AuthSession;
class AuthDialogController;
class AgentNotifier;

typedef struct _GList        GList;
typedef struct _GCancellable GCancellable;

// AuthSessionManager — owns the queue of authentication requests.
//
// Polkit may deliver multiple InitiateAuthentication calls concurrently.
// This manager ensures only one dialog is shown at a time; the rest wait.
//
// Flow:
//   polkitd → initiateAuthentication()
//             → if idle: start immediately
//             → if busy: enqueue, start when current finishes
//
// Each request has a per-session timeout. If the user does not respond
// within the timeout, the session is auto-cancelled.

class AuthSessionManager : public QObject
{
    Q_OBJECT

public:
    static constexpr int kTimeoutSeconds = 120;

    explicit AuthSessionManager(AuthDialogController *controller,
                                AgentNotifier        *notifier,
                                QObject              *parent = nullptr);
    ~AuthSessionManager() override;

    // Called from SlmPolkitAgent GObject callback — queues or starts immediately.
    void initiateAuthentication(const QString &actionId,
                                const QString &message,
                                const QString &iconName,
                                const QString &cookie,
                                GList         *identities,
                                GCancellable  *cancellable,
                                void          *asyncCallback,
                                void          *asyncUserData);

private slots:
    void onSessionCompleted(bool gainedAuth);
    void onTimeout();
    void onUserAuthenticate(const QString &password);
    void onUserCancel();

private:
    struct PendingRequest {
        QString actionId;
        QString message;
        QString iconName;
        QString cookie;
        GList   *identities;
        GCancellable *cancellable;
        void    *asyncCallback;
        void    *asyncUserData;
    };

    void startNext();
    void startRequest(const PendingRequest &req);
    void finishCurrent();

    AuthDialogController *m_controller;
    AgentNotifier        *m_notifier;
    AuthSession          *m_current = nullptr;
    QQueue<PendingRequest> m_queue;
    QTimer               *m_timeout;
};
