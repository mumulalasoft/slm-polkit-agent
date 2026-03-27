#pragma once
#include <QObject>

// GLib/Polkit forward declarations (avoid pulling GLib headers into Qt code)
typedef struct _PolkitAgentListener PolkitAgentListener;

class AuthSessionManager;

// SlmPolkitAgent — thin bridge between libpolkit-agent-1 (GLib/GObject) and Qt.
//
// Subclasses PolkitAgentListener via a GObject wrapper (see slmpolkitagent.cpp).
// Receives InitiateAuthentication callbacks from polkitd and delegates to
// AuthSessionManager.
//
// Responsibilities:
//   - Register/unregister this process as the session authentication agent
//   - Forward authentication requests to AuthSessionManager
//   - Forward cancellations to AuthSessionManager
//
// Does NOT make authorization decisions — that stays with polkitd.

class SlmPolkitAgent : public QObject
{
    Q_OBJECT

public:
    explicit SlmPolkitAgent(AuthSessionManager *manager, QObject *parent = nullptr);
    ~SlmPolkitAgent() override;

    // Register with polkit for the current session subject.
    // Returns false if registration fails (no active session, polkitd not running, etc.)
    bool registerAgent();
    void unregisterAgent();

private:
    AuthSessionManager *m_manager;
    PolkitAgentListener *m_listener = nullptr; // GObject, managed manually
    void *m_registeredAgent = nullptr;         // opaque polkit registration handle
};
