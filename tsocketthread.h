#ifndef TSOCKETTHREAD_H
#define TSOCKETTHREAD_H

#include <QThread>
#include <QRunnable>
#include <QObject>
#include <QDebug>
#include <QtNetwork/QTcpSocket>
#include <QMap>
#include <QTimer>
#include <QTime>
#include <QProcess>
#include <QSharedPointer>
#include "httprequest.h"

class TSocketThread : public QObject
{
    Q_OBJECT
public:
    typedef QMap<QString, QString> TUsers; //ключ имя пользователя, значение - пароль

    typedef struct {
        QString ExecuteFileName;
        QMap<QString, QString> AddonsHeaders;
        bool CodeToUTF8 = true;
    } TCGIParametr;

    typedef QMap<QString, TCGIParametr> TCGI; //ключь - название, значение - имя исполняемого файла

public:
    explicit TSocketThread(qintptr Discriptor, const TUsers &Users, const TCGI &CGI, QObject *parent = nullptr);
    virtual ~TSocketThread();

private:
    const qintptr socketDiscriptor;
    const TCGI& CGIList; //список CGI модулей
    const TUsers& UsersList; //список пользователей

    //нужны для логов
    QString CurrentUID = "n/a";
    QString CurrentCGI = "HTTPServerCGI";
    quint64 CurrentSpeed = 0; //в КВ/сек

    QTcpSocket* TcpSocket = nullptr;
    HTTPRequest* Request = nullptr;
    QProcess* Process = nullptr;

    bool FirstPacket = true;

    quint64 IDSession = 0;

    QTimer* WatchDog = nullptr;
    QTime* Time  = nullptr;

private:
    void ParseRequest();
    bool CheckUID(const QString &UID, const QString &PWD); //Проверяет имя пользователя и пароль
    qint64 SendAnswer(quint16 Code, const QByteArray& Msg = ""); //отправляет ответ
    void Stop();  //завершает работу потока

private slots:
    void onReadyRead();
    void onDisconected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

    void onTimeout();

    void onCGIFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCGIErrorOccurred(QProcess::ProcessError error);

public slots:
    void onFinished();
    void onStart();

signals:
    void WriteLog(uint16_t Category, const QString &UID, const QString &Sender, const QString &Msg);
    void Finished();
};



#endif // TSOCKETTHREAD_H
