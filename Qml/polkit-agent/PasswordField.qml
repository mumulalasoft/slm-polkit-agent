import QtQuick
import QtQuick.Controls

// PasswordField — password input with shake-on-error animation.
//
// Uses a Translate transform for the shake so it doesn't conflict with
// parent layout managers (ColumnLayout, etc.) that control the x property.

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

    // Translate transform — layout-safe, does not conflict with managed x
    transform: Translate { id: shiftT; x: 0 }

    SequentialAnimation {
        id: shakeAnim
        loops: 1
        PropertyAnimation { target: shiftT; property: "x"; to:  7; duration: 45; easing.type: Easing.OutQuad }
        PropertyAnimation { target: shiftT; property: "x"; to: -7; duration: 45; easing.type: Easing.InOutQuad }
        PropertyAnimation { target: shiftT; property: "x"; to:  5; duration: 40; easing.type: Easing.InOutQuad }
        PropertyAnimation { target: shiftT; property: "x"; to: -5; duration: 40; easing.type: Easing.InOutQuad }
        PropertyAnimation { target: shiftT; property: "x"; to:  2; duration: 35; easing.type: Easing.InOutQuad }
        PropertyAnimation { target: shiftT; property: "x"; to:  0; duration: 35; easing.type: Easing.InQuad }
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
