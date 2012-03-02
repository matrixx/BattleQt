#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
class ConnectionManagerPrivate;

class ConnectionManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ConnectionManager)

    // server related errors
    enum ServerError {
        ServerNotInMultiPlayerMode,
        ServerHasInvalidPlayerName,
        ServerAlreadyRunning,
        CannotListen,
        ServerGotUnknownError
    };

    // client related errors
    enum JoiningError {
        ClientNotInMultiPlayerMode,
        ClientHasInvalidPlayerName,
        InvalidServerIP,
        InvalidServerPort,
        ClientAlreadyConnected,
        ServerClosedConnection,
        ServerNotFound,
        ServerRefusedConnection,
        ServerNotTrusted,
        AlreadyJoinedInServerMode,
        ClientGotUnknownError
    };

    // message sending related errors
    enum GeneralError {
        NotConnected,
        MessageEmpty
    };

public:
    explicit ConnectionManager(QObject *parent = 0);

public slots:
    // enabling multiplayer mode, user is going to connect to network
    void enableMultiPlayerMode(bool enable);

    // starts game/chat server
    void startServer(QString playerName, QString password = QString());

    // closes server
    void closeServer();

    // joins to existing game
    void joinGame(QString playerName, QString serverIp, QString serverPort, QString serverPassword);

    // leaves from current game
    void leaveGame();

    // sends message to other player/chatter
    void sendMessage(QString message);

    // sends request for response time, emits pong when request received
    void ping();

signals:
    // multiplayer mode (client and server)
    void multiPlayerModeEnabled();
    void multiPlayerModeDisabled();
    void networkUnavailable();

    // server
    void serverStarted(QString ip, QString port);
    void serverError(ServerError error, QString errorString);
    void serverClosed();
    void playerConnected(QString playerName);
    void playerDisconnected(QString playerName);

    // client
    void joiningSucceeded(QString otherPlayer);
    void joiningError(JoiningError error, QString errorString);
    void leftFromGame();

    // messages (client and server)
    void messageSent();
    void incomingMessage(QString message);

    // general errors
    void generalError(GeneralError error, QString errorString);

    // timing between server and client
    void pong(int msecs);

protected:
    ConnectionManagerPrivate* const d_ptr;
private:

};

#endif // CONNECTIONMANAGER_H
