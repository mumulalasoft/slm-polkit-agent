#pragma once
#include <QObject>
#include <QString>

// GLib forward declarations
typedef struct _PolkitAgentSession PolkitAgentSession;
typedef struct _GCancellable       GCancellable;
typedef struct _GAsyncReadyCallback GAsyncReadyCallback; // not a struct, but avoids include
typedef void (*GAsyncReadyCallback_t)(void*, void*, void*);

// AuthSession — wraps one PolkitAgentSession (a single authentication attempt).
//
// State machine:
//   Idle → Authenticating → Completed / Failed / Cancelled
//
// Password is passed in, used once, and must be zeroed by the caller after respond().

class AuthSession : public QObject
{
    Q_OBJECT

public:
    enum class State { Idle, Authenticating, Completed, Failed, Cancelled };

    explicit AuthSession(const QString &cookie,
                         void          *polkitIdentity, // PolkitIdentity*
                         GCancellable  *cancellable,
                         void          *asyncCallback,  // GAsyncReadyCallback
                         void          *asyncUserData,
                         QObject       *parent = nullptr);
    ~AuthSession() override;

    QString cookie() const { return m_cookie; }
    State   state()  const { return m_state; }

    void start();
    void respond(const QString &password); // password zeroed internally after use
    void cancel();

signals:
    void showError(const QString &text);
    void showInfo(const QString &text);
    void requestPassword();             // polkit wants a password
    void completed(bool gainedAuth);    // session done

private:
    static void onRequest(PolkitAgentSession *session,
                          const char         *request,
                          gboolean            echoOn,
                          void               *userData);
    static void onShowError(PolkitAgentSession *session,
                            const char         *text,
                            void               *userData);
    static void onShowInfo(PolkitAgentSession *session,
                           const char         *text,
                           void               *userData);
    static void onCompleted(PolkitAgentSession *session,
                            gboolean            gainedAuth,
                            void               *userData);

    QString          m_cookie;
    State            m_state = State::Idle;
    PolkitAgentSession *m_session = nullptr;
    GCancellable     *m_cancellable = nullptr;
    void             *m_asyncCallback = nullptr;
    void             *m_asyncUserData = nullptr;
};
