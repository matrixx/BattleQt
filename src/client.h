#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTime>
#include <QAbstractSocket>

class QTcpSocket;

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = 0);
    void setPassword(QString password);
    void setPlayerName(QString playerName);
    void join(QString ip, QString port);
    void sendMessage(QString message, bool internal);
    void ping();
    void close();

signals:
    void messageRead(QString message);
    void messageSent();
    void messageError();
    void joinSuccess(QString otherPlayer);
    void joinError(QString error);
    void partSuccess();
    void pong(int msecs);

private slots:
    void readMessage();
    void handlerError(QAbstractSocket::SocketError error);
    void onConnected();
    void onDisconnected();

private:
    void parseMessage(QString message);
    void onWelcomeSuccess(QString otherPlayerName);
    void onWelcomeFail();

    QString m_ip;
    QString m_port;
    QString m_player;
    QString m_password;
    QTcpSocket* m_client;
    QString m_otherPlayerName;
    QTime* m_pingTime;
    bool m_joined;
    quint16 blockSize;
    bool m_welcome;
    bool m_closed;
};

#endif // CLIENT_H
