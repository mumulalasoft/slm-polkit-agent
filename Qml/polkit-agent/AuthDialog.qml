import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// AuthDialog — system authentication dialog for the polkit agent.
//
// Uses a fullscreen transparent window on Wayland (no nested Window objects —
// Wayland does not support absolute screen positioning for child windows).
// The dim layer fills the whole screen; the dialog card is centered on it.
//
// Driven by AuthDialogController (C++ context property "AuthDialog").

Window {
    id: root

    // Fullscreen transparent window — works on both Wayland and X11.
    // On Wayland, Qt maps this to xdg_toplevel with fullscreen state.
    flags:      Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color:      "transparent"
    visibility: AuthDialog.visible ? Window.FullScreen : Window.Hidden
    visible:    AuthDialog.visible

    // Clear password field whenever the dialog is hidden.
    // This prevents a stale password from being visible if the dialog re-opens.
    Connections {
        target: AuthDialog
        function onVisibleChanged() {
            if (!AuthDialog.visible)
                passwordField.clear()
        }
    }

    // Fade in on show
    opacity: 0
    Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
    onVisibleChanged: opacity = visible ? 1 : 0

    // ── Full-screen content ─────────────────────────────────────────────
    Item {
        anchors.fill: parent

        // Dim layer — absorbs all pointer input outside the dialog card
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0, 0, 0, 0.48)

            MouseArea {
                anchors.fill: parent
                // Intentionally absorb clicks — auth dialogs must not be
                // dismissed by clicking outside. User must Cancel explicitly.
            }
        }

        // ── Dialog card ────────────────────────────────────────────────
        Rectangle {
            id: dialogCard
            anchors.centerIn: parent
            width: 320
            height: content.implicitHeight + 48
            radius: 16
            color: Qt.rgba(0.12, 0.12, 0.14, 0.97)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.09)

            // Subtle inner glow on top edge
            Rectangle {
                anchors {
                    top: parent.top; left: parent.left; right: parent.right
                    topMargin: 1; leftMargin: 1; rightMargin: 1
                }
                height: 1
                color: Qt.rgba(1, 1, 1, 0.08)
                radius: parent.radius
            }

            ColumnLayout {
                id: content
                anchors {
                    left: parent.left; right: parent.right
                    top: parent.top; margins: 24
                }
                spacing: 16

                // ── Header: icon + message + identity ──────────────────
                AuthIdentityView {
                    Layout.fillWidth: true
                    iconName: AuthDialog.iconName
                    message:  AuthDialog.message
                    identity: AuthDialog.identity
                }

                // ── Error / info banner ────────────────────────────────
                Text {
                    Layout.fillWidth: true
                    text: AuthDialog.errorText || AuthDialog.infoText
                    color: AuthDialog.errorText.length > 0
                        ? "#e07070"
                        : Qt.rgba(1, 1, 1, 0.5)
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    visible: text.length > 0
                }

                // ── Password input ─────────────────────────────────────
                PasswordField {
                    id: passwordField
                    Layout.fillWidth: true
                    enabled: !AuthDialog.loading
                    hasError: AuthDialog.errorText.length > 0

                    onSubmitted: function(password) {
                        AuthDialog.submitPassword(password)
                    }

                    onHasErrorChanged: {
                        if (hasError) shake()
                    }
                }

                // ── Loading state ──────────────────────────────────────
                Item {
                    Layout.fillWidth: true
                    implicitHeight: 24
                    visible: AuthDialog.loading

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: AuthDialog.loading
                        width: 22; height: 22
                        palette.dark: Qt.rgba(1, 1, 1, 0.65)
                    }
                }

                // ── Action buttons ─────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Cancel")
                        enabled: !AuthDialog.loading
                        onClicked: AuthDialog.requestCancel()

                        background: Rectangle {
                            radius: 8
                            color: parent.hovered
                                ? Qt.rgba(1, 1, 1, 0.12)
                                : Qt.rgba(1, 1, 1, 0.07)
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Authenticate")
                        enabled: !AuthDialog.loading && passwordField.text.length > 0
                        onClicked: AuthDialog.submitPassword(passwordField.text)

                        background: Rectangle {
                            radius: 8
                            color: parent.enabled
                                ? (parent.hovered
                                    ? Qt.rgba(0.30, 0.55, 0.92, 1)
                                    : Qt.rgba(0.24, 0.48, 0.86, 1))
                                : Qt.rgba(1, 1, 1, 0.07)
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.enabled ? "white" : Qt.rgba(1, 1, 1, 0.25)
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Item { implicitHeight: 0 } // bottom padding inside margins
            }
        }
    }
}
