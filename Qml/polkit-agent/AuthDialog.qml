import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// AuthDialog — root window for the polkit authentication agent.
//
// Driven by AuthDialogController (C++ context property "AuthDialog").
// Appears as a floating system dialog above all other windows.
// Invisible when AuthDialog.visible == false.

Window {
    id: root
    flags: Qt.WindowStaysOnTopHint | Qt.FramelessWindowHint | Qt.Tool
    color: "transparent"
    width: 360
    height: dialogSurface.implicitHeight + 40 // shadow margin
    visible: AuthDialog.visible

    // Center on screen
    Component.onCompleted: {
        x = Screen.width  / 2 - width  / 2
        y = Screen.height / 2 - height / 2
    }

    // Dim the background (full-screen overlay on primary screen)
    Window {
        id: dimOverlay
        flags: Qt.WindowStaysOnTopHint | Qt.FramelessWindowHint | Qt.Tool
        color: Qt.rgba(0, 0, 0, 0.45)
        x: 0; y: 0
        width: Screen.width; height: Screen.height
        visible: AuthDialog.visible

        Behavior on opacity { NumberAnimation { duration: 180 } }

        MouseArea {
            anchors.fill: parent
            // Absorb clicks on the overlay (do not dismiss dialog)
        }
    }

    // Entrance animation
    NumberAnimation on opacity {
        from: 0; to: 1
        duration: 200
        running: AuthDialog.visible
    }

    // ── Dialog surface ──────────────────────────────────────────────────
    Rectangle {
        id: dialogSurface
        anchors.centerIn: parent
        width: 320
        implicitHeight: content.implicitHeight + 48
        radius: 16
        color: Qt.rgba(0.13, 0.13, 0.15, 0.96)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.10)

        // Drop shadow
        layer.enabled: true
        layer.effect: null // replace with MultiEffect shadow when Qt 6.5+ available

        ColumnLayout {
            id: content
            anchors {
                left: parent.left; right: parent.right
                top: parent.top
                margins: 24
            }
            spacing: 16

            // Identity + message header
            AuthIdentityView {
                Layout.fillWidth: true
                iconName: AuthDialog.iconName
                message:  AuthDialog.message
                identity: AuthDialog.identity
            }

            // Error / info text
            Text {
                Layout.fillWidth: true
                text: AuthDialog.errorText || AuthDialog.infoText
                color: AuthDialog.errorText ? "#e07070" : Qt.rgba(1, 1, 1, 0.55)
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                visible: text.length > 0
            }

            // Password field
            PasswordField {
                id: passwordField
                Layout.fillWidth: true
                hasError: AuthDialog.errorText.length > 0
                enabled: !AuthDialog.loading

                onSubmitted: function(password) {
                    AuthDialog.submitPassword(password)
                }

                // Shake on error
                onHasErrorChanged: {
                    if (hasError) shake()
                }
            }

            // Loading indicator (replaces password field when authenticating)
            Item {
                Layout.fillWidth: true
                height: 20
                visible: AuthDialog.loading

                BusyIndicator {
                    anchors.centerIn: parent
                    running: AuthDialog.loading
                    palette.dark: Qt.rgba(1, 1, 1, 0.7)
                    width: 20; height: 20
                }
            }

            // Action buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                // Cancel
                Button {
                    Layout.fillWidth: true
                    text: qsTr("Cancel")
                    enabled: !AuthDialog.loading
                    onClicked: AuthDialog.requestCancel()

                    background: Rectangle {
                        radius: 8
                        color: parent.hovered ? Qt.rgba(1,1,1,0.12) : Qt.rgba(1,1,1,0.07)
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // Authenticate
                Button {
                    Layout.fillWidth: true
                    text: qsTr("Authenticate")
                    enabled: !AuthDialog.loading && passwordField.text.length > 0
                    onClicked: AuthDialog.submitPassword(passwordField.text)

                    background: Rectangle {
                        radius: 8
                        color: parent.enabled
                            ? (parent.hovered ? Qt.rgba(0.3, 0.55, 0.9, 1)
                                              : Qt.rgba(0.25, 0.48, 0.85, 1))
                            : Qt.rgba(1,1,1,0.08)
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "white" : Qt.rgba(1,1,1,0.3)
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            // Bottom spacer
            Item { height: 0 }
        }
    }
}
