import QtQuick
import QtQuick.Layouts

// AuthIdentityView — shows the lock icon + identity (user/admin being authenticated as).

Item {
    id: root
    implicitHeight: col.implicitHeight
    implicitWidth: col.implicitWidth

    property string iconName: "dialog-password"
    property string identity: ""
    property string message:  ""

    ColumnLayout {
        id: col
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 12

        // Lock / action icon
        Image {
            Layout.alignment: Qt.AlignHCenter
            source: root.iconName ? ("image://icon/" + root.iconName) : "image://icon/dialog-password"
            width: 52; height: 52
            smooth: true
            fillMode: Image.PreserveAspectFit

            // Fallback rectangle if icon not found
            Rectangle {
                anchors.fill: parent
                radius: 12
                color: Qt.rgba(1, 1, 1, 0.08)
                visible: parent.status !== Image.Ready
            }
        }

        // Authentication message
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 280
            text: root.message || qsTr("Authentication Required")
            color: "white"
            font.pixelSize: 15
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        // Identity label (who is authenticating as)
        Text {
            Layout.alignment: Qt.AlignHCenter
            visible: root.identity.length > 0
            text: root.identity
            color: Qt.rgba(1, 1, 1, 0.55)
            font.pixelSize: 12
        }
    }
}
