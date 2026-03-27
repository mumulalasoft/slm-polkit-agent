# slm-polkit-agent — Architecture

## Overview

`slm-polkit-agent` is the official Polkit authentication agent for SLM Desktop.
It replaces GNOME/KDE polkit agents with a native SLM implementation.

## Component diagram

```
polkitd
  │  InitiateAuthentication (GLib callback)
  ▼
SlmPolkitAgent          ← GObject subclass of PolkitAgentListener
  │  Qt signal bridged
  ▼
AuthSessionManager      ← queue, timeout, lifecycle
  │
  ├─► AuthSession       ← wraps PolkitAgentSession (one auth attempt)
  │     │  signals
  │     ▼
  └─► AuthDialogController  ← QObject context property for QML
            │  properties + invokables
            ▼
          AuthDialog.qml    ← visual dialog (Window)
```

## Data flow — password submission

1. User types password in `PasswordField.qml`
2. QML calls `AuthDialog.submitPassword(password)`
3. `AuthDialogController::submitPassword()` emits `authenticate(password)`
4. `AuthSessionManager::onUserAuthenticate()` calls `AuthSession::respond(password)`
5. `AuthSession::respond()` calls `polkit_agent_session_response()` then **zeros the buffer**
6. polkitd verifies → emits `completed` signal
7. `AuthSession::onCompleted()` resolves the GAsyncResult → polkitd receives result

## Security notes

- Core dumps disabled at startup (`RLIMIT_CORE = 0`)
- Password buffer zeroed immediately after `polkit_agent_session_response()`
- No password ever reaches `qDebug`, journal, or stderr
- Single-instance enforced via DBus service name ownership
- Dialog cannot be dismissed by clicking outside (no cancel-on-outside-click)
- Auto-cancel timeout: 120 seconds

## State machine

```
[Idle]
  │ InitiateAuthentication
  ▼
[Authenticating]
  │
  ├─ PasswordWrong → showError → back to [Authenticating] (retry)
  ├─ Cancelled     → [Idle] → next in queue
  ├─ Timeout       → [Idle] → next in queue
  └─ Success       → [Idle] → next in queue
```
