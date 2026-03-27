import QtQuick
import QtQuick.Controls

// PasswordField — password input with shake-on-error animation.
// Exposes text property and submitPassword() signal.

Item {
    id: root
    implicitHeight: field.implicitHeight + 8
    implicitWidth: 300

    property alias text: field.text
    property bool  hasError: false

    signal submitted(string password)

    function shake() {
        shakeAnim.restart()
    }

    function clear() {
        field.text = ""
    }

    // Shake animation on wrong password
    SequentialAnimation {
        id: shakeAnim
        loops: 1
        PropertyAnimation { target: root; property: "x"; to: root.x + 6;  duration: 40 }
        PropertyAnimation { target: root; property: "x"; to: root.x - 6;  duration: 40 }
        PropertyAnimation { target: root; property: "x"; to: root.x + 4;  duration: 40 }
        PropertyAnimation { target: root; property: "x"; to: root.x - 4;  duration: 40 }
        PropertyAnimation { target: root; property: "x"; to: root.x;      duration: 40 }
    }

    TextField {
        id: field
        anchors.fill: parent
        echoMode: TextInput.Password
        placeholderText: qsTr("Password")
        selectByMouse: true
        focus: true

        background: Rectangle {
            radius: 6
            color: Qt.rgba(1, 1, 1, 0.06)
            border.width: 1
            border.color: root.hasError
                ? "#e05555"
                : (field.activeFocus ? "#6ea4d8" : Qt.rgba(1, 1, 1, 0.18))
            Behavior on border.color { ColorAnimation { duration: 120 } }
        }

        color: "white"
        placeholderTextColor: Qt.rgba(1, 1, 1, 0.4)
        font.pixelSize: 14

        Keys.onReturnPressed: root.submitted(field.text)
        Keys.onEnterPressed:  root.submitted(field.text)
    }
}
