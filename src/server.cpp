#include "server.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QStringList>

Server::Server(QObject *parent) :
    QObject(parent),
    m_server(NULL),
    m_client(NULL),
    m_pingTime(NULL),
    blockSize(0),
    m_authenticated(false),
    m_created(false),
    m_closed(false),
    m_otherPlayerConnected(false)
{
}

void Server::setPassword(QString password)
{
    m_password = password;
}

void Server::setPlayerName(QString playerName)
{
    m_player = playerName;
}

void Server::create()
{
    if (m_created && m_server) {
        emit createFailure("exists");
        return;
    }
    m_closed = false;
    m_server = new QTcpServer(this);
    connect(m_server, SIGNAL(newConnection()), this, SLOT(connectPlayer()));
    if (!m_server->listen()) {
        qDebug("could not start server, reason: %s", qPrintable(m_server->errorString()));
        m_server->deleteLater();
        m_server = 0;
        emit createFailure("listen");
    } else {
        QString ipAddress;
        QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

        // use the first non-localhost IPv4 address
        for (int i = 0; i < ipAddressesList.size(); ++i) {
            if (ipAddressesList.at(i) != QHostAddress::LocalHost && ipAddressesList.at(i).toIPv4Address()) {
                ipAddress = ipAddressesList.at(i).toString();
                break;
            }
        }

        // if we did not find one, use IPv4 localhost
        if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
        m_ip = ipAddress;
        m_port = QString::number(m_server->serverPort());
        m_created = true;
        emit createSuccess(m_ip, m_port);
    }
}

void Server::connectPlayer()
{
    if (m_client) {
        m_client->deleteLater();
        m_client = 0;
    }
    m_client = m_server->nextPendingConnection();
    connect(m_client, SIGNAL(readyRead()), this, SLOT(readMessage()));
    connect(m_client, SIGNAL(disconnected()),
            this, SLOT(onDisconnected()));
}

void Server::onDisconnected()
{
    qDebug("client disconnected on server side");
    m_client->deleteLater();
    m_client = 0;
    m_authenticated = false;
    if (m_closed) {
        m_server->close();
        m_server->deleteLater();
        m_server = 0;
        m_closed = false;
    }
    m_otherPlayerConnected = false;
    emit playerDisconnected(m_player);
}

void Server::readMessage()
{
    QDataStream in(m_client);
    in.setVersion(QDataStream::Qt_4_0);

    if (blockSize == 0) {
        if (m_client->bytesAvailable() < (int)sizeof(quint16))
            return;

        in >> blockSize;
    }

    if (m_client->bytesAvailable() < blockSize)
        return;

    QString message;
    in >> message;
    blockSize = 0;
    parseMessage(message);
}

void Server::parseMessage(QString message)
{
    qDebug("parsing message from client: %s", qPrintable(message));
    QStringList list = message.split(" ");
    QString command = list.first();
    if (!m_authenticated && command != "password") {
        onAuthFail();
        return;
    } else if (m_authenticated && command == "password") {
        return;
    }

    if (command == "password") {
        if ((list.size() == 2 && !m_password.isEmpty() && list.at(1) == m_password)
                || (list.size() == 1 && m_password.isEmpty())) {
            onAuthSuccess();
        } else {
            onAuthFail();
        }
        return;
    }

    if (m_authenticated) {
        if (command == "username") {
            if (list.size() == 2) {
                m_otherPlayerName = list.at(1);
                qDebug("user %s joined server", qPrintable(m_otherPlayerName));
                m_otherPlayerConnected = true;
                emit playerConnected(m_otherPlayerName);
            } else {
                return;
            }
        } else if (command == "clientmessage") {
            qDebug("got client message: %s", qPrintable(message));
            message.remove(0, 14);
            emit messageRead(message);
        } else if (command == "ping") {
            sendMessage("pong", true);
        } else if (command == "pong") {
            if (m_pingTime) {
                emit pong(m_pingTime->elapsed());
                delete m_pingTime;
                m_pingTime = 0;
            } else {
                emit pong(-1);
            }
        }
        // discard message if none of those
    }
}

void Server::onAuthSuccess()
{
    qDebug("client successfully authenticated, sending username");
    m_authenticated = true;
    sendMessage("username " + m_player, true);
}

void Server::onAuthFail()
{
    qDebug("authentication failure, disconnecting");
    m_client->disconnectFromHost();
}

void Server::sendMessage(QString message, bool internal)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;
    out << message;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    qDebug("server sending message: %s", qPrintable(QString(block)));
    if ((m_otherPlayerConnected && m_client) || internal) {
        m_client->write(block);
        if (!internal) {
            emit messageSent();
        }
    } else if (!m_otherPlayerConnected || m_client) {
        emit messageError();
    }
}

void Server::ping()
{
    m_pingTime = new QTime;
    m_pingTime->start();
    sendMessage("ping", true);
}

void Server::close()
{
    if (m_server && m_client) {
        m_closed = true;
        m_client->disconnectFromHost();
    }
}
