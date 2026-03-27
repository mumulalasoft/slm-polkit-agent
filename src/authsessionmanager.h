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

// AuthSessionManager — serializes polkit authentication requests.
//
// polkitd can issue multiple concurrent InitiateAuthentication calls.
// This manager ensures only one dialog is shown at a time; extras are queued.
//
// Request lifecycle:
//   initiateAuthentication() → [queue or start immediately]
//   → startRequest() → AuthSession → dialog shown
//   → user responds / cancels / timeout
//   → finishCurrent() → next in queue (if any)
//
// GLib ownership rules:
//   - Each PendingRequest holds a g_object_ref'd identity and cancellable.
//   - These are unref'd in startRequest() (after passing to AuthSession) or
//     in the destructor for queued-but-never-started requests.

class AuthSessionManager : public QObject
{
    Q_OBJECT

public:
    static constexpr int kTimeoutSeconds = 120;

    explicit AuthSessionManager(AuthDialogController *controller,
                                AgentNotifier        *notifier,
                                QObject              *parent = nullptr);
    ~AuthSessionManager() override;

    // Entry point from SlmPolkitAgent GObject callback.
    // identities: GList of PolkitIdentity* — valid only for the duration of this call.
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
        QString identityName;    // resolved at enqueue time
        void   *identity;        // g_object_ref'd PolkitIdentity — unref after use
        GCancellable *cancellable; // g_object_ref'd — unref after use
        void   *asyncCallback;
        void   *asyncUserData;
    };

    void startRequest(PendingRequest &req);
    void finishCurrent();
    void failRequest(void *asyncCallback, void *asyncUserData,
                     GCancellable *cancellable, const char *reason);

    AuthDialogController *m_controller;
    AgentNotifier        *m_notifier;
    AuthSession          *m_current = nullptr;
    QQueue<PendingRequest> m_queue;
    QTimer               *m_timeout;
};
