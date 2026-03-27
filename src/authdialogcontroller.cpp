#include "authdialogcontroller.h"

AuthDialogController::AuthDialogController(QObject *parent)
    : QObject(parent)
{
}

void AuthDialogController::presentRequest(const QString &actionId,
                                          const QString &message,
                                          const QString &iconName,
                                          const QString &identity)
{
    m_actionId   = actionId;
    m_message    = message;
    m_iconName   = iconName;
    m_identity   = identity;
    m_errorText  = {};
    m_infoText   = {};
    m_loading    = false;
    m_visible    = true;
    emit requestChanged();
    emit errorTextChanged();
    emit infoTextChanged();
    emit loadingChanged();
    emit visibleChanged();
}

void AuthDialogController::hide()
{
    m_visible  = false;
    m_loading  = false;
    m_errorText = {};
    m_infoText  = {};
    emit visibleChanged();
    emit loadingChanged();
    emit errorTextChanged();
    emit infoTextChanged();
}

void AuthDialogController::showPasswordPrompt()
{
    m_loading = false;
    emit loadingChanged();
}

void AuthDialogController::setError(const QString &text)
{
    m_errorText = text;
    m_loading   = false;
    emit errorTextChanged();
    emit loadingChanged();
}

void AuthDialogController::setInfo(const QString &text)
{
    m_infoText = text;
    emit infoTextChanged();
}

void AuthDialogController::submitPassword(const QString &password)
{
    m_loading   = true;
    m_errorText = {};
    emit loadingChanged();
    emit errorTextChanged();
    emit authenticate(password);
    // Note: password string will be zeroed in AuthSession::respond()
}

void AuthDialogController::requestCancel()
{
    emit cancel();
}
