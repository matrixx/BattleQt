#include "connectionmanager_p.h"
#include "include/connectionmanager.h"
#include "server.h"
#include "client.h"

#include <QNetworkAccessManager>

ConnectionManagerPrivate::ConnectionManagerPrivate(ConnectionManager *parent) :
    QObject(parent),
    q_ptr(parent),
    m_session(NULL),
    m_server(NULL),
    m_client(NULL),
    m_multiPlayerModeEnabled(false),
    m_host(false),
    m_closed(false)
{
}

ConnectionManagerPrivate::~ConnectionManagerPrivate()
{
    m_session->close();
}

void ConnectionManagerPrivate::startConnecting()
{
    m_closed = false;
    connect(&m_configManager, SIGNAL(updateCompleted()), this, SLOT(connectToNetwork()));
    connect(&m_networkUpdateTimer, SIGNAL(timeout()), this, SLOT(connectToNetwork()));
    qDebug("getting default access point");
    m_accessPoint = m_configManager.defaultConfiguration();

    if (m_accessPoint.isValid()) {
        qDebug("ap valid, connecting");
        connectToNetwork();
    } else {
        qDebug("updating configurations");
        m_networkUpdateTimer.setSingleShot(true);
        m_networkUpdateTimer.setInterval(5000);
        m_networkUpdateTimer.start();
        m_configManager.updateConfigurations();
    }
}

void ConnectionManagerPrivate::connectToNetwork()
{
    Q_Q(ConnectionManager);
    qDebug("ap found, trying to connect");
    disconnect(&m_configManager, SIGNAL(updateCompleted()), this, SLOT(connectToNetwork()));
    disconnect(&m_networkUpdateTimer, SIGNAL(timeout()), this, SLOT(connectToNetwork()));
    if (m_networkUpdateTimer.isActive()) {
        m_networkUpdateTimer.stop();
    }
    m_accessPoint = m_configManager.defaultConfiguration();
    const bool canStartIAP = (m_configManager.capabilities() & QNetworkConfigurationManager::CanStartAndStopInterfaces);

    if (!m_accessPoint.isValid() || !canStartIAP) {
        qDebug("no network available");
        m_multiPlayerModeEnabled = false;
        emit q->networkUnavailable();
    } else {
        qDebug("creating network session");
        m_session = new QNetworkSession(m_accessPoint, this);
        connect(m_session, SIGNAL(opened()), this, SLOT(finishConnecting()));
        connect(m_session, SIGNAL(error(QNetworkSession::SessionError)), this,
                SLOT(handleNetworkError(QNetworkSession::SessionError)));
        connect(m_session, SIGNAL(stateChanged(QNetworkSession::State)), this,
                SLOT(handleSessionStateChanged(QNetworkSession::State)));
        if (m_session->isOpen()) {
            finishConnecting();
        } else {
            qDebug("opening session");
            m_session->open();
        }
    }
}

void ConnectionManagerPrivate::finishConnecting()
{
    Q_Q(ConnectionManager);
    qDebug("session opened, connected");
    m_multiPlayerModeEnabled = true;
    emit q->multiPlayerModeEnabled();
}

void ConnectionManagerPrivate::handleNetworkError(QNetworkSession::SessionError error)
{
    Q_Q(ConnectionManager);
    m_multiPlayerModeEnabled = false;
    if (!m_closed) {
        emit q->networkUnavailable();
        qDebug() << "ap non valid, error:" << error;
    } else if (error == QNetworkSession::SessionAbortedError) {
        emit q->multiPlayerModeDisabled();
    }
}

void ConnectionManagerPrivate::handleServerError(QString error)
{
    Q_Q(ConnectionManager);
    m_host = false;
    if (error == "exists") {
        emit q->serverError(ConnectionManager::ServerAlreadyRunning, "Server is already running");
    } else if (error == "listen") {
        emit q->serverError(ConnectionManager::CannotListen, "Cannot start listening to network socket");
    } else {
        emit q->serverError(ConnectionManager::ServerGotUnknownError, "Server got unknown error");
    }
    delete m_server;
    m_server = 0;
    qDebug("server error");
}

void ConnectionManagerPrivate::handleServerSuccess(QString ip, QString port)
{
    Q_Q(ConnectionManager);
    m_host = true;
    emit q->serverStarted(ip, port);
    qDebug("server started on ip: %s and port %s", qPrintable(ip), qPrintable(port));
}

void ConnectionManagerPrivate::handleJoiningError(QString error)
{
    Q_Q(ConnectionManager);
    if (error == "untrusted") {
        emit q->joiningError(ConnectionManager::ServerNotTrusted, "Server is not trusted, not connecting");
    } else if (error == "closed") {
        emit q->joiningError(ConnectionManager::ServerClosedConnection, "Server closed the connection");
    } else if (error == "notfound") {
        emit q->joiningError(ConnectionManager::ServerNotFound, "Server not found");
    } else if (error == "refused") {
        emit q->joiningError(ConnectionManager::ServerRefusedConnection, "Server not able to authenticate client");
    } else if (error == "unknown") {
        emit q->joiningError(ConnectionManager::ClientGotUnknownError, "Client got unknown error");
    }
    m_client->deleteLater();
    m_client = 0;
    qDebug("unable to join game");
}

void ConnectionManagerPrivate::handleJoiningSuccess(QString otherPlayer)
{
    Q_Q(ConnectionManager);
    emit q->joiningSucceeded(otherPlayer);
    qDebug("successfully joined a game with %s", qPrintable(otherPlayer));
}

void ConnectionManagerPrivate::handleLeavingFromServer()
{
    Q_Q(ConnectionManager);
    m_client->deleteLater();
    m_client = 0;
    emit q->leftFromGame();
}

void ConnectionManagerPrivate::handleMessageError()
{
    Q_Q(ConnectionManager);
    emit q->generalError(ConnectionManager::NotConnected, "Cannot send message, not connected");
}

void ConnectionManagerPrivate::startServer(QString player, QString password)
{
    Q_Q(ConnectionManager);
    if (!m_multiPlayerModeEnabled) {
        emit q->serverError(ConnectionManager::ServerNotInMultiPlayerMode, "Multiplayer mode not enabled");
    } else if (player.isEmpty()) {
        emit q->serverError(ConnectionManager::ServerHasInvalidPlayerName, "Player name not valid");
    } else {
        m_server = new Server(this);
        m_server->setPassword(password);
        m_server->setPlayerName(player);
        connect(m_server, SIGNAL(createSuccess(QString,QString)), this, SLOT(handleServerSuccess(QString,QString)));
        connect(m_server, SIGNAL(createFailure(QString)), this, SLOT(handleServerError(QString)));
        connect(m_server, SIGNAL(playerConnected(QString)), q, SIGNAL(playerConnected(QString)));
        connect(m_server, SIGNAL(playerDisconnected(QString)), q, SIGNAL(playerDisconnected(QString)));
        connect(m_server, SIGNAL(messageRead(QString)), q, SIGNAL(incomingMessage(QString)));
        connect(m_server, SIGNAL(messageSent()), q, SIGNAL(messageSent()));
        connect(m_server, SIGNAL(messageError()), this, SLOT(handleMessageError()));
        connect(m_server, SIGNAL(pong(int)), q, SIGNAL(pong(int)));
        m_server->create();
    }
}

void ConnectionManagerPrivate::closeServer()
{
    Q_Q(ConnectionManager);
    m_server->close();
    m_server->deleteLater();
    m_server = 0;
    emit q->serverClosed();
}

void ConnectionManagerPrivate::joinGame(QString player, QString ip, QString port, QString password)
{
    Q_Q(ConnectionManager);
    qDebug("trying to join a game");
    if (m_host) {
        emit q->joiningError(ConnectionManager::AlreadyJoinedInServerMode, "Cannot join as client, already connected as server");
    } else if (!m_multiPlayerModeEnabled) {
        emit q->joiningError(ConnectionManager::ClientNotInMultiPlayerMode, "Multiplayer mode not enabled");
    } else if (player.isEmpty()) {
        emit q->joiningError(ConnectionManager::ClientHasInvalidPlayerName, "Player name not valid");
    } else if (ip.isEmpty()) {
        emit q->joiningError(ConnectionManager::InvalidServerIP, "Server IP not valid");
    } else if (port.isEmpty()) {
        emit q->joiningError(ConnectionManager::InvalidServerPort, "Server port not valid");
    } else if (m_client) {
        emit q->joiningError(ConnectionManager::ClientAlreadyConnected, "Already connected");
    } else {
        m_client = new Client(this);
        m_client->setPassword(password);
        m_client->setPlayerName(player);
        connect(m_client, SIGNAL(joinSuccess(QString)), this, SLOT(handleJoiningSuccess(QString)));
        connect(m_client, SIGNAL(joinError(QString)), this, SLOT(handleJoiningError(QString)));
        connect(m_client, SIGNAL(partSuccess()), this, SLOT(handleLeavingFromServer()));
        connect(m_client, SIGNAL(messageRead(QString)), q, SIGNAL(incomingMessage(QString)));
        connect(m_client, SIGNAL(messageSent()), q, SIGNAL(messageSent()));
        connect(m_client, SIGNAL(messageError()), this, SLOT(handleMessageError()));
        connect(m_client, SIGNAL(pong(int)), q, SIGNAL(pong(int)));
        m_client->join(ip, port);
    }
}

void ConnectionManagerPrivate::leaveGame()
{
    Q_Q(ConnectionManager);
    if (m_client) {
        m_client->close();
    } else {
        emit q->generalError(ConnectionManager::NotConnected, "Cannot leave game, already left");
    }
}

void ConnectionManagerPrivate::sendMessage(QString message)
{
    Q_Q(ConnectionManager);
    if (message.isEmpty()) {
        emit q->generalError(ConnectionManager::MessageEmpty, "Cannot send empty message");
    } else if (m_host && m_server) {
        message.prepend("servermessage ");
        m_server->sendMessage(message, false);
    } else if (!m_host && m_client) {
        message.prepend("clientmessage ");
        m_client->sendMessage(message, false);
    }
}

void ConnectionManagerPrivate::ping()
{
    if (m_host && m_server) {
        m_server->ping();
    } else if (!m_host && m_client) {
        m_client->ping();
    }
}

void ConnectionManagerPrivate::closeConnection()
{
    m_closed = true;
    if (m_host && m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = 0;
    } else if (!m_host && m_client) {
        m_client->close();
        m_client->deleteLater();
        m_client = 0;
    }
    m_session->close();
}


ConnectionManager::ConnectionManager(QObject *parent) :
    QObject(parent), d_ptr(new ConnectionManagerPrivate(this))
{
}

void ConnectionManager::enableMultiPlayerMode(bool enable)
{
    Q_D(ConnectionManager);
    if (enable)
        d->startConnecting();
    else
        d->closeConnection();
}

void ConnectionManager::startServer(QString playerName, QString password)
{
    Q_D(ConnectionManager);
    d->startServer(playerName, password);
}

void ConnectionManager::closeServer()
{
    Q_D(ConnectionManager);
    d->closeServer();
}

void ConnectionManager::joinGame(QString playerName, QString serverIp, QString serverPort, QString serverPassword)
{
    Q_D(ConnectionManager);
    d->joinGame(playerName, serverIp, serverPort, serverPassword);
}

void ConnectionManager::leaveGame()
{
    Q_D(ConnectionManager);
    d->leaveGame();
}

void ConnectionManager::sendMessage(QString message)
{
    Q_D(ConnectionManager);
    d->sendMessage(message);
}

void ConnectionManager::ping()
{
    Q_D(ConnectionManager);
    d->ping();
}
