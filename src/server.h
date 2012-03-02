#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTime>

class QTcpServer;
class QTcpSocket;

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = 0);
    void setPassword(QString password);
    void setPlayerName(QString playerName);
    void create();
    void sendMessage(QString message, bool internal);
    void ping();
    void close();

signals:
    void createSuccess(QString ip, QString port);
    void createFailure(QString error);
    void playerConnected(QString playerName);
    void playerDisconnected(QString playerName);
    void messageRead(QString message);
    void messageSent();
    void messageError();
    void pong(int msecs);

private slots:
    void connectPlayer();
    void onDisconnected();
    void readMessage();

private:
    void parseMessage(QString message);
    void onAuthSuccess();
    void onAuthFail();

    QString m_ip;
    QString m_port;
    QString m_player;
    QString m_password;
    QTcpServer* m_server;
    QTcpSocket* m_client;
    QString m_otherPlayerName;
    QTime* m_pingTime;
    quint16 blockSize;
    bool m_authenticated;
    bool m_created;
    bool m_closed;
    bool m_otherPlayerConnected;
};

#endif // SERVER_H
