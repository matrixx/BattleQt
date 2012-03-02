#ifndef CONNECTIONMANAGERPRIVATE_H
#define CONNECTIONMANAGERPRIVATE_H

#include <QObject>
#include "include/connectionmanager.h"

#include <QNetworkConfigurationManager>
#include <QNetworkConfiguration>
#include <QNetworkSession>
#include <QTimer>

class Server;
class Client;

class ConnectionManagerPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(ConnectionManager)
public:
    explicit ConnectionManagerPrivate(ConnectionManager *parent = 0);
    
    ~ConnectionManagerPrivate();
    void startConnecting();
    void startServer(QString player, QString password);
    void closeServer();
    void joinGame(QString player, QString ip, QString port, QString password);
    void leaveGame();
    void sendMessage(QString message);
    void ping();
    void closeConnection();

public slots:
    void connectToNetwork();
    void finishConnecting();
    void handleNetworkError(QNetworkSession::SessionError error);
    void handleServerError(QString error);
    void handleServerSuccess(QString ip, QString port);
    void handleJoiningError(QString error);
    void handleJoiningSuccess(QString otherPlayer);
    void handleLeavingFromServer();
    void handleMessageError();
protected:
    ConnectionManager* const q_ptr;
private:
    QNetworkConfigurationManager m_configManager;
    QNetworkConfiguration m_accessPoint;
    QNetworkSession* m_session;
    QTimer m_networkUpdateTimer;

    Server* m_server;
    Client* m_client;

    bool m_multiPlayerModeEnabled;
    bool m_host;
    int m_retryCount;
    QTimer m_retryTimer;
    bool m_closed;
};

#endif // CONNECTIONMANAGERPRIVATE_H
