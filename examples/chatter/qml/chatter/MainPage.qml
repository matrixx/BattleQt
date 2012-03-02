import QtQuick 1.1
import com.nokia.meego 1.0

Page {
    id: maini
    tools: commonTools
    property string otherPlayerNick: ""
    property bool server: false
    Column {
        anchors.margins: 10;
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        spacing: 5
        Button {
            id: multiplayerButton
            property string notEnabled: "Enable multiplayer mode";
            property string enabled: "Enable single player mode";
            property string enabling: "Cancel";
            text: notEnabled
            onClicked: {
                if (text == notEnabled) {
                    text = enabling;
                    multiplayerState.text = multiplayerState.enabling
                    manager.enableMultiPlayerMode(true);
                } else if (text == enabling || text == enabled) {
                    text = notEnabled;
                    multiplayerState.text = multiplayerState.notEnabled
                    manager.enableMultiPlayerMode(false)
                }
            }
        }
        Label {
            id: multiplayerState
            property string enabled: "Multiplayer state enabled";
            property string enabling: "Enabling multiplayer state";
            property string notEnabled: "Single player state enabled";
            property string networkUnavailable: "Network unavailable";
            text: notEnabled
        }

        Row {
            Label {
                text: "Username:"
                width: 150
            }
            TextField {
                id: userName;
                width: 300;
            }
        }
        Row {
            Label {
                text: "Server password:"
                width: 150
            }
            TextField {
                id: password;
                width: 300;
            }
        }

        Button {
            text: "start server";
            onClicked: { server = true; manager.startServer(userName.text, password.text); disconnectButton.visible = true }
        }
        Label {
            id: serverStatusLabel
            text: "Server not started";
        }

        Row {
            Label {
                text: "Username:"
                width: 150
            }
            TextField {
                id: playerName
                width: 300;
            }
        }
        Row {
            Label {
                text: "IP:"
                width: 150
            }
            TextField {
                id: ip;
                width: 300;
            }
        }
        Row {
            Label {
                text: "Port:"
                width: 150
            }
            TextField {
                id: port
                width: 300
            }
        }
        Row {
            Label {
                text: "Server password:"
                width: 150
            }
            TextField {
                id: passWd
                width: 300
            }
        }
        Row {
            spacing: 5
            Button {
                text: "join server";
                width: 180
                onClicked: { manager.joinGame(playerName.text, ip.text, port.text, passWd.text); disconnectButton.visible = true }
            }
            Button {
                id: disconnectButton
                text: "disconnect"
                width: 180
                visible: false
                onClicked: server ? manager.closeServer() : manager.leaveGame();
            }
            Button {
                id: pingButton
                text: "Ping"
                width: 90
                onClicked: manager.ping()
            }
        }
        Label {
            id: joiningStatusLabel
            text: "Not joined to game"
        }
        Row {
            Label {
                id: nickLabel
                text: "";
            }
            TextField {
                id: chatField
                width: 200
            }
            Button {
                id: sendButton
                width: 150
                text: "Send"
                onClicked: {
                    console.debug("sending message:" + chatField.text);
                    manager.sendMessage(chatField.text)
                    chatField.text = ""
                }
            }
        }
        Label {
            id: latestReceived
            text: ""
        }
    }
    Rectangle {
        id: container

        property string text: ""

        signal clicked()

        height: infoText.height + 40;
        width: parent.width - 40;
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        border.width: 2
        border.color: 'black'
        visible: false
        z: 100
        color: 'white'
        radius: 10
        Text {
            id: infoText
            width: parent.width - 40;
            anchors.centerIn: parent
            font.pixelSize: 14
            color: 'black'
            wrapMode: Text.WordWrap
            text: container.text
        }
        MouseArea {
            anchors.fill: parent
            onClicked: { container.visible = false; container.clicked() }
        }
    }
    Connections {
        target: manager
        onMultiPlayerModeEnabled: {
            multiplayerButton.text = multiplayerButton.enabled
            multiplayerState.text = multiplayerState.enabled
        }
        onNetworkUnavailable: {
            multiplayerButton.text = multiplayerButton.notEnabled
            multiplayerState.text = multiplayerState.networkUnavailable
        }
        onServerStarted: {
            serverStatusLabel.text = "Server started at: " + ip + ":" + port;
            joiningStatusLabel.text = "Started server using nick " + userName.text
            nickLabel.text = userName.text + ":";
        }
        onJoiningSucceeded: {
            joiningStatusLabel.text = "Joined to game with " + otherPlayer;
            otherPlayerNick = otherPlayer
            nickLabel.text = playerName.text + ":";
        }
        onIncomingMessage: {
            latestReceived.text = otherPlayerNick + ": " + message;
        }
        onPlayerConnected: {
            otherPlayerNick = playerName
            joiningStatusLabel.text = otherPlayerNick + " joined the server";
        }
        onPlayerDisconnected: {
            joiningStatusLabel.text = otherPlayerNick + " left game";
        }
        onServerClosed: {
            serverStatusLabel.text = "Server closed";
            joiningStatusLabel.text = "disconnected other player";
        }
        onLeftFromGame: {
            joiningStatusLabel.text = "Left from game"
        }
        onServerError: {
            container.text = errorString;
            container.visible = true;
        }
        onJoiningError: {
            container.text = errorString;
            container.visible = true;
        }
        onGeneralError: {
            container.text = errorString;
            container.visible = true;
        }
        onPong: {
            container.text = "Ping: " + msecs;
            container.visible = true;
        }
    }
}
