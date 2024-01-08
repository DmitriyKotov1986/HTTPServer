#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QThread>
#include "httpserver.h"
#include "tsocketthread.h"


THTTPServer::THTTPServer(QSharedPointer<QSettings> Config, QObject *parent)
    : QTcpServer(parent)
{
    this->Config = Config;

    Config->beginGroup("DATABASE");

    DB = QSqlDatabase::addDatabase(Config->value("Driver", "QODBC").toString(), "MainDB");
    DB.setDatabaseName(Config->value("DataBase", "SystemMonitorDB").toString());
    DB.setUserName(Config->value("UID", "").toString());
    DB.setPassword(Config->value("PWD", "").toString());
    DB.setConnectOptions(Config->value("ConnectionOprions", "").toString());
    DB.setPort(Config->value("Port", "").toUInt());
    DB.setHostName(Config->value("Host", "localhost").toString());

    Config->endGroup();

    if (!DB.open()) {
        throw std::runtime_error("Cannot connect to database. Error: " + DB.lastError().text().toStdString());
    };

    //Загружаем данные о пользователях
    QSqlQuery Query(DB);
    Query.setForwardOnly(true);
    DB.transaction();

    QString TextQuery = "SELECT [UID], [PWD] "
                        "FROM [Users] "
                        "WHERE [Enabled] <> 0 ";

    if (!Query.exec(TextQuery)) {
        qCritical() << "FAIL. Cannot load information about users from database. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        DB.rollback();
        SendLogMsg(MSG_CODE::CODE_ERROR, "", "HTTPServer", "Cannot load information about users from database. Error: " + Query.lastError().text() + " Query: " + Query.lastQuery());
        exit(-1);
    }
    while (Query.next()) {
        qDebug() << "User add. UID = " << Query.value("UID").toString() << " PWD = " << Query.value("PWD").toString();
        UsersList.insert(Query.value("UID").toString(), Query.value("PWD").toString());
    }

    if (!DB.commit()) {
        qCritical() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    };

    Config->beginGroup("CGI");
    uint32_t CountCGI = Config->value("Count", 0).toUInt();
    QString PathCGI = Config->value("Path", 0).toString();
    Config->endGroup();

    for (uint32_t i = 0; i < CountCGI; ++i) {
        Config->beginGroup("CGI" + QString::number(i));
        QString CGICMD = Config->value("CMD", "").toString();
        TSocketThread::TCGIParametr tmp;
        tmp.CodeToUTF8 = Config->value("CodeToUTF8", true).toBool();
        tmp.ExecuteFileName = PathCGI + "/" + Config->value("ExecuteFileName", 0).toString();
        qint16 HeadersCount = Config->value("HeadersCount", -1).toInt();
        for (qint16 j = 0; j < HeadersCount; ++j) {
            QString Key = Config->value("HeaderKey" + QString::number(j), "").toString();
            if (Key != "") {
                tmp.AddonsHeaders.insert(Key, Config->value("HeaderValue" + QString::number(j), "").toString());
            }
        }

        CGIList.insert(CGICMD, tmp);
        qDebug() << "CGI module add. CMD:" << CGICMD << " Execute file name:" << tmp.ExecuteFileName <<
                    "Addon headers count:" << tmp.AddonsHeaders.size() << "Code to UTF8:" << tmp.CodeToUTF8;
        Config->endGroup();
    }

    WriteLogTimer = new QTimer(this);

    QObject::connect(WriteLogTimer, SIGNAL(timeout()), this, SLOT(onWriteLogTimer()));
    WriteLogTimer->start(5000);

    SendLogMsg(MSG_CODE::CODE_OK, "", "HTTPServer", "Start is succesfull");
}

THTTPServer::~THTTPServer()
{
    SendLogMsg(MSG_CODE::CODE_OK, "", "HTTPServer", "Finished is succesfull");

    DB.close();

    onWriteLogTimer();

    WriteLogTimer->deleteLater();
}


void THTTPServer::Listing()
{
    Config->beginGroup("SERVER");
    uint16_t Port = Config->value("Port", 8080).toUInt();
    Config->endGroup();

    if (listen(QHostAddress::AnyIPv4, Port))
    {
        SendLogMsg(MSG_CODE::CODE_INFORMATION, "", "HTTPServer", "The server was listening on port: " + QString::number(Port));
    }
    else {
        SendLogMsg(MSG_CODE::CODE_ERROR, "", "HTTPServer", "Error listening server on port: " + QString::number(Port) + " Msg: " + errorString());
        exit(-10);
    }
}

void THTTPServer::DisconnectAll()
{
    emit StopAll(); //тормозим все потоки
    close(); //Закрываем все соединения
}

void THTTPServer::incomingConnection(qintptr Handle)
{

   QThread* Thread = new QThread(); //создаем поток
   TSocketThread* TcpSocketThread = new TSocketThread(Handle, UsersList, CGIList); //создаем класс обработчика соединения

   TcpSocketThread->moveToThread(Thread); //перемещаем обработчик в отдельный поток

   QObject::connect(TcpSocketThread, SIGNAL(WriteLog(uint16_t, const QString, const QString, const QString )),
                    this, SLOT(onWriteLog(uint16_t, const QString, const QString, const QString)));

   //запускаем обработку стазу после создания потока
   QObject::connect(Thread, SIGNAL(started()), TcpSocketThread, SLOT(onStart()));

   //Ставим поток на удаление когда завершили работы
   QObject::connect(TcpSocketThread, SIGNAL(Finished()), Thread, SLOT(quit()));

   //тормозим поток при отключении сервера
   QObject::connect(this, SIGNAL(StopAll()), TcpSocketThread, SLOT(onFinished()));

   //ставим поток на удаление когда он сам завершился
   QObject::connect(Thread, SIGNAL(finished()), Thread, SLOT(deleteLater()));

   //ставим обработчик на удаление когда он завершился
   QObject::connect(TcpSocketThread, SIGNAL(Finished()), TcpSocketThread, SLOT(deleteLater()));

   //запускаем поток на выполнение
   Thread->start(QThread::NormalPriority);
}

void THTTPServer::SendLogMsg(uint16_t Category, const QString& UID, const QString& Sender, const QString& Msg)
{
    qDebug() << Msg;
    //сохраняем сообщение в очередь
    TLogMsg tmp;
    tmp.Category = Category;
    tmp.UID = UID;
    tmp.Sender = Sender;
    tmp.Msg = Msg;
    tmp.Msg.replace("'", "''");
    tmp.DateTime = QDateTime::currentDateTime();
    LogQueue.enqueue(tmp);
}

void THTTPServer::onWriteLogTimer()
{
 //   QTime time = QTime::currentTime();
    if (!LogQueue.isEmpty()) {

        QSqlQuery QueryLog(DB);
        DB.transaction();

        QString QueryText = "INSERT INTO LOG ([CATEGORY], [UID], [DateTime], [SENDER], [MSG]) VALUES ";

        while (!LogQueue.isEmpty()) {
            TLogMsg tmp = LogQueue.dequeue();
            QueryText += "(" +
                         QString::number(tmp.Category) + ", "
                         "'" + tmp.UID + "', "
                         "CAST('" + tmp.DateTime.toString("yyyy-MM-dd hh:mm:ss.zzz") + "' AS DATETIME2), "
                         "'" + tmp.Sender +"', "
                         "'" + tmp.Msg +"'), ";
        }
        QueryText = QueryText.left(QueryText.length() - 2); //удаляем лишнюю запятую в конце


        if (!QueryLog.exec(QueryText)) {
            qCritical() << "FAIL Cannot execute query. Error: " << QueryLog.lastError().text() << " Query: "<< QueryLog.lastQuery();
            DB.rollback();
            exit(-1);
        }
        if (!DB.commit()) {
            qCritical() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
            DB.rollback();
            exit(-2);
        };
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Log messages save successull";
    }
    else {
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "No log messages";
    }
 //   qDebug() << "WriteLogTime" << time.msecsTo(QTime::currentTime()) << "ms";
}

void THTTPServer::onWriteLog(uint16_t Category, const QString& UID, const QString& Sender, const QString& Msg)
{
    SendLogMsg(Category, UID, Sender, Msg);
}
