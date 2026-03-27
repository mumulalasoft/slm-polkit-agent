#pragma once
#include <QObject>
#include <QString>

// AuthDialogController — QObject bridge between C++ session manager and QML dialog.
//
// Exposed to QML as context property "AuthDialog".
// QML reads properties and calls invokable methods.
// C++ connects to signals (authenticate, cancel) to respond to user actions.

class AuthDialogController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool    visible     READ visible     NOTIFY visibleChanged)
    Q_PROPERTY(QString actionId    READ actionId    NOTIFY requestChanged)
    Q_PROPERTY(QString message     READ message     NOTIFY requestChanged)
    Q_PROPERTY(QString iconName    READ iconName    NOTIFY requestChanged)
    Q_PROPERTY(QString identity    READ identity    NOTIFY requestChanged)
    Q_PROPERTY(QString errorText   READ errorText   NOTIFY errorTextChanged)
    Q_PROPERTY(QString infoText    READ infoText    NOTIFY infoTextChanged)
    Q_PROPERTY(bool    loading     READ loading     NOTIFY loadingChanged)

public:
    explicit AuthDialogController(QObject *parent = nullptr);

    bool    visible()   const { return m_visible; }
    QString actionId()  const { return m_actionId; }
    QString message()   const { return m_message; }
    QString iconName()  const { return m_iconName; }
    QString identity()  const { return m_identity; }
    QString errorText() const { return m_errorText; }
    QString infoText()  const { return m_infoText; }
    bool    loading()   const { return m_loading; }

    // Called by AuthSessionManager
    void presentRequest(const QString &actionId,
                        const QString &message,
                        const QString &iconName,
                        const QString &identity);
    void hide();

public slots:
    void showPasswordPrompt();
    void setError(const QString &text);
    void setInfo(const QString  &text);

signals:
    void visibleChanged();
    void requestChanged();
    void errorTextChanged();
    void infoTextChanged();
    void loadingChanged();

    // Emitted by QML — consumed by AuthSessionManager
    void authenticate(const QString &password);
    void cancel();

public:
    // Invokable from QML
    Q_INVOKABLE void submitPassword(const QString &password);
    Q_INVOKABLE void requestCancel();

private:
    bool    m_visible  = false;
    QString m_actionId;
    QString m_message;
    QString m_iconName;
    QString m_identity;
    QString m_errorText;
    QString m_infoText;
    bool    m_loading  = false;
};
