#include "client.h"
#include <QTcpSocket>
#include <QStringList>

Client::Client(QObject *parent) :
    QObject(parent),
    m_client(NULL),
    m_pingTime(NULL),
    m_joined(false),
    blockSize(0),
    m_welcome(false),
    m_closed(false)
{
}

void Client::setPassword(QString password)
{
    m_password = password;
}

void Client::setPlayerName(QString playerName)
{
    m_player = playerName;
}

void Client::join(QString ip, QString port)
{
    qDebug("joining");
    m_client = new QTcpSocket(this);
    connect(m_client, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(m_client, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(m_client, SIGNAL(readyRead()), this, SLOT(readMessage()));
    connect(m_client, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handlerError(QAbstractSocket::SocketError)));
    m_client->connectToHost(ip, port.toUInt());
    qDebug("starting to connect host");
}

void Client::sendMessage(QString message, bool internal)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;
    out << message;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    qDebug("client sending message: %s", qPrintable(QString(block)));
    if ((m_welcome && m_client) || internal) {
        m_client->write(block);
        if (!internal) {
            emit messageSent();
        }
    } else if (!m_welcome || m_client) {
        emit messageError();
    }
}

void Client::ping()
{
    m_pingTime = new QTime;
    m_pingTime->start();
    sendMessage("ping", true);
}

void Client::close()
{
    if (m_client) {
        m_closed = true;
        m_client->disconnectFromHost();
        qDebug("closing connection to server");
    }
}

void Client::readMessage()
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

void Client::parseMessage(QString message)
{
    qDebug("parsing message from server: %s", qPrintable(message));
    QStringList list = message.split(" ");
    QString command = list.first();
    if (command != "username" && !m_welcome) {
        onWelcomeFail();
        return;
    } else if (command == "username" && m_welcome) {
        return;
    } else if (command == "username" && !m_welcome) {
        if (list.size() == 2) {
            onWelcomeSuccess(list.at(1));
        } else {
            onWelcomeFail();
        }
        return;
    }

    if (m_welcome && command == "servermessage") {
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

void Client::onWelcomeSuccess(QString otherPlayerName)
{
    m_otherPlayerName = otherPlayerName;
    sendMessage("username " + m_player, true);
    m_welcome = true;
    emit joinSuccess(m_otherPlayerName);
}

void Client::onWelcomeFail()
{
    m_client->disconnectFromHost();
    emit joinError("untrusted");
}

void Client::handlerError(QAbstractSocket::SocketError error)
{
    switch (error) {
        case QAbstractSocket::RemoteHostClosedError:
            qDebug("Server closed connection");
            emit joinError("closed");
            break;
        case QAbstractSocket::HostNotFoundError:
            qDebug("Server not found");
            emit joinError("notfound");
            break;
        case QAbstractSocket::ConnectionRefusedError:
            qDebug("Server refused connection");
            emit joinError("refused");
            break;
        default:
            emit joinError("unknown");
            qDebug("Error: %s", qPrintable(m_client->errorString()));
            break;
    }
    m_joined = false;
}

void Client::onConnected()
{
    m_joined = true;
    if (!m_password.isEmpty())
        sendMessage("password " + m_password, true);
}

void Client::onDisconnected()
{
    if (m_closed) {
        emit partSuccess();
    }
}


