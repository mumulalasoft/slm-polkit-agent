#pragma once
#include <QObject>
#include <QString>

typedef struct _PolkitAgentSession PolkitAgentSession;
typedef struct _GCancellable       GCancellable;

// AuthSession — wraps one PolkitAgentSession (a single authentication attempt).
//
// Lifecycle:
//   1. Construct with cookie + identity (GObject, caller retains ownership)
//   2. Call start() — polkit initiates PAM conversation
//   3. On requestPassword(): call respond(password) — password zeroed internally
//   4. On completed(gainedAuth): session is done, delete this object
//
// Cancel path:
//   Call cancel() → emits completed(false) → delete
//
// Thread safety: single-threaded Qt+GLib event loop only.

class AuthSession : public QObject
{
    Q_OBJECT

public:
    enum class State { Idle, Authenticating, Completed, Failed, Cancelled };

    // identity: PolkitIdentity* — AuthSession does NOT take ownership (no extra ref).
    //           polkit_agent_session_new() will ref it internally.
    // cancellable: g_object_ref'd internally.
    // asyncCallback / asyncUserData: passed back to polkitd via GTask on completion.
    explicit AuthSession(const QString &cookie,
                         void          *identity,
                         GCancellable  *cancellable,
                         void          *asyncCallback,
                         void          *asyncUserData,
                         QObject       *parent = nullptr);
    ~AuthSession() override;

    QString cookie() const { return m_cookie; }
    State   state()  const { return m_state; }

    void start();
    void respond(const QString &password); // zeros QByteArray buffer after PAM response
    void cancel();

signals:
    void showError(const QString &text);
    void showInfo(const QString &text);
    void requestPassword();          // polkit PAM is asking for a password
    void completed(bool gainedAuth); // session finished (success, failure, or cancel)

private:
    // GLib signal callbacks — fire on Qt main thread (via QEventDispatcherGlib)
    static void onRequest  (PolkitAgentSession *, const char *req, gboolean echo, void *ud);
    static void onShowError(PolkitAgentSession *, const char *text, void *ud);
    static void onShowInfo (PolkitAgentSession *, const char *text, void *ud);
    static void onCompleted(PolkitAgentSession *, gboolean gainedAuth, void *ud);

    void completeTask(bool success, const char *errorMsg = nullptr);

    QString             m_cookie;
    State               m_state      = State::Idle;
    PolkitAgentSession *m_session    = nullptr;
    GCancellable       *m_cancellable = nullptr;
    void               *m_asyncCallback = nullptr;
    void               *m_asyncUserData = nullptr;
};
