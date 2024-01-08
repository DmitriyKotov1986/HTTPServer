#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QSettings>
#include <QSqlDatabase>
#include <QTimer>
#include <QQueue>
#include <QSharedPointer>
#include <memory>
#include "tsocketthread.h"

typedef enum {CODE_OK, CODE_ERROR, CODE_INFORMATION, CODE_HTTP_ERROR, CODE_HTTP_AUTHORISATION, CODE_HTTP_UNDEFINE_CGI, CODE_CGI_ERROR, CODE_HTTP_CGI_NOT_STARTED} MSG_CODE;

class THTTPServer : public QTcpServer
{
    Q_OBJECT
private:
    typedef struct {
        uint16_t Category;
        QDateTime DateTime;
        QString UID;
        QString Sender;
        QString Msg;
    } TLogMsg;

public:
    explicit THTTPServer(QSharedPointer<QSettings> Config, QObject *parent = nullptr);
    virtual ~THTTPServer();

    void Listing();
    void DisconnectAll();
    void incomingConnection(qintptr Handle);

private:
    QSharedPointer<QSettings> Config;
    QSqlDatabase DB;

    TSocketThread::TUsers UsersList;
    TSocketThread::TCGI CGIList;

    QQueue <TLogMsg> LogQueue; //очередь сообщений лога
    QTimer* WriteLogTimer; //таймер записи лога

    void SendLogMsg(uint16_t Category, const QString& UID, const QString& Sender, const QString& Msg);

private slots:
    void onWriteLogTimer();

public slots:
    void onWriteLog(uint16_t Category, const QString& UID, const QString& Sender, const QString& Msg);

signals:
    void StopAll();

};

#endif // HTTPSERVER_H
