#include <QFile>
#include <QProcess>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QByteArray>
#include <QTime>
#include <QRandomGenerator>
#include <QTextCodec>
#include "httpserver.h"
#include "tsocketthread.h"
#include "httpanswer.h"

TSocketThread::TSocketThread(qintptr Discriptor, const TUsers & Users, const TCGI &CGI, QObject *parent)
    : QObject(parent)
    , socketDiscriptor(Discriptor)
    , CGIList(CGI)
    , UsersList(Users)
{
    //Создаем уникальный ИД
    QRandomGenerator *rg = QRandomGenerator::global();
    IDSession = rg->generate64();
}

TSocketThread::~TSocketThread()
{   
    if (TcpSocket != nullptr) delete TcpSocket;
    if (Request != nullptr) delete Request;
    if (Process != nullptr) delete Process;
    if (WatchDog != nullptr) delete WatchDog;
    if (Time != nullptr) delete Time;
}

void TSocketThread::onStart()
{
    //Создаем сокет
    TcpSocket = new QTcpSocket();
    TcpSocket->setSocketDescriptor(socketDiscriptor);
    TcpSocket->

    QObject::connect(TcpSocket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));  //пришли новые данные
    QObject::connect(TcpSocket, SIGNAL(disconnected()), this, SLOT(onDisconected())); //дисконнект
    QObject::connect(TcpSocket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
                     this, SLOT(onErrorOccurred(QAbstractSocket::SocketError)));  //ошибка соединения

    //Создаем ватчдог
    WatchDog = new QTimer();
    WatchDog->setInterval(600000);
    WatchDog->setSingleShot(true);
    QObject::connect(WatchDog, SIGNAL(timeout()), this, SLOT(onTimeout()));  //таймаут соединения
    WatchDog->start(); //запускаем WatchDog

    Time = new QTime(QTime::currentTime()); //Счетчик времени выполенния потока

    Request = new HTTPRequest(); //Класс приемник данных;

    Process = new QProcess(); //класс процесса обработки CGI
    QObject::connect(Process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onCGIFinished(int, QProcess::ExitStatus))); //процесс завершился
    QObject::connect(Process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(onCGIErrorOccurred(QProcess::ProcessError))); //произошла ошибка

    emit WriteLog(MSG_CODE::CODE_OK, CurrentUID, CurrentCGI, "ID: " + QString::number(IDSession) + " " +
                                                             "Incomming connect. Client: " + TcpSocket->peerAddress().toString() +  ":" + QString::number(TcpSocket->peerPort()));
}

void TSocketThread::ParseRequest()
{
    QString Resource = Request->GetResurce();
    //Запрашиваемый ресурс - CGI
    if (Resource.left(4) == "CGI/") {
        Resource.remove(0,4);
        CurrentCGI = Resource.left(Resource.indexOf('&'));
        if (CGIList.find(CurrentCGI) != CGIList.end()) {
            Resource.remove(0, CurrentCGI.length() + 1); //удаляем лишнее
            //получаем логин и пароль пользователя
            CurrentUID = Resource.left(Resource.indexOf('&'));
            Resource.remove(0, Resource.indexOf('&') + 1);
            QString PWD = Resource.left(Resource.indexOf('/'));
            QStringList Params;
            if (Resource.indexOf('/') != -1) {
                Resource.remove(0, Resource.indexOf('/') + 1);
                Params << Resource;
            }
            //qDebug() << Request.GetBody();
            //qDebug() << "ID:" << IDSession << "User:" << CurrentCGI << "PWD:" << PWD << "Start CGI module:" << CurrentCGI << Params;
            if (CheckUID(CurrentUID, PWD)) { //проверяем правильность логина и пароля
                //Если все ок - Запускаем CGI модуль
                qDebug() << "ID:" << IDSession << "User:" << CurrentUID << "PWD:" << PWD << "Start CGI module:" << CurrentCGI << "Params" << Params.join(",");
                Process->start(CGIList[CurrentCGI].ExecuteFileName, Params);
            }
            else {
                SendAnswer(401); // 401 Unauthorized («не авторизован (не представился)»
                emit WriteLog(MSG_CODE::CODE_HTTP_AUTHORISATION, CurrentUID, CurrentCGI, "ID: " + QString::number(IDSession) + " User undefine. Pwd: " + PWD);
            }
        }
        else  {
            SendAnswer(523); //523 Origin Is Unreachable («источник недоступен»
            emit WriteLog(MSG_CODE::CODE_HTTP_UNDEFINE_CGI, CurrentUID, CurrentCGI,  "ID: " + QString::number(IDSession) + " CGI module not found. Module:" + CurrentCGI);
        }
    }
    else {
        //503 - Service Unavailable («сервис недоступен»)
        SendAnswer(503);
        emit WriteLog(MSG_CODE::CODE_HTTP_ERROR, CurrentUID, CurrentCGI,  "ID: " + QString::number(IDSession) + " Requested non-CGI module. Resource: " + Resource);
    }

}


bool TSocketThread::CheckUID(const QString &UID, const QString &PWD)
{
    if (UsersList.find(UID) != UsersList.end()) {
        return (UsersList[UID] == PWD);
    }
    return false;
}

qint64 TSocketThread::SendAnswer(quint16 Code, const QByteArray& Msg)
{
    HTTPAnswer Answer(Code);
    if (Msg != "") {
        Answer.AddBody(Msg);
        for (const auto& KeyItem : CGIList[CurrentCGI].AddonsHeaders.keys()) {
            Answer.AddHeader(KeyItem, CGIList[CurrentCGI].AddonsHeaders[KeyItem]);
        }
    }
    qint64 SendBytes = TcpSocket->write(Answer.GetAnswer());
    TcpSocket->waitForBytesWritten();  //ждем записи данных в буфер

    Stop();

    return SendBytes;
}

void TSocketThread::Stop()
{
    WatchDog->stop();

    //вызываем дисконнект
    if (TcpSocket->isOpen()){
        TcpSocket->disconnectFromHost();
    }

    //убиваем процесс если он еще не завершился
    if (Process->state() != QProcess::NotRunning) {
        emit WriteLog(MSG_CODE::CODE_CGI_ERROR, CurrentUID, CurrentCGI, "ID: " + QString::number(IDSession) + " CGI process does not complete on time");
        Process->terminate();
        Process->waitForFinished(2000);
        Process->kill();
        Process->waitForFinished(2000);
    }

    //тормозим WathDog
    WatchDog->stop();

    //Сообщаем о завершении
    emit Finished();
}


void TSocketThread::onReadyRead()
{
    //считываем пришедшие данные
    Request->Add(TcpSocket->readAll());
    //если заголовок успешно разобран - запускаем CGI обработчик
    if ((FirstPacket)&&(Request->isGetHeader())) {
        FirstPacket = false;
        if (Request->GetErrorCode() == 200) { //Заголовок успешно разобран
              ParseRequest();//Парсим заголовок и запускаем CGI процесс
        }
        else { //запрос содержит ошибки
            SendAnswer(Request->GetErrorCode()); //Отправляем сообщение об ошибке
            //Сохраняем данные в лог
            emit WriteLog(MSG_CODE::CODE_HTTP_ERROR, CurrentUID, CurrentCGI, "ID: " + QString::number(IDSession) + " Incorrect request. Code: " +
                          QString::number(Request->GetErrorCode()) + " Msg: " + Request->GetErrorMsg() +
                          " IP: " + TcpSocket->peerAddress().toString() + " Port: " + QString::number(TcpSocket->peerPort()));
        }
    }

    //если пришли все данные
    if (Request->isComplite()) {
        //qDebug() << "Complite" << Process->arguments() << " " << Process->program();
        CurrentSpeed = Request->Size() / (Time->msecsTo(QTime::currentTime()) + 1); //расчитываем скорость загрузки в КВ/сек
        //ждем завершения запуска CGI процесса
        if (Process->waitForStarted(10000)) {
            //Пишем данные в стандартный ввод процесса. В случае ошиибки ссгернерируется onCGIErrorOccurred
            qDebug() << "ID:" << IDSession << "All data received. Start processing";
            Process->write(Request->GetBody());
            Process->write("\nEOF\n"); //добавляем конец посылки
            //Ждем завершения выполения процесса. onCGIFinished
        }
        else {
         qDebug() << "No started";
        }
    }

}

void TSocketThread::onCGIFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (Process->exitCode() == 0) { //Если процесс завершился с кодом 0 - то обработка прошла успешно. Отправляем ответ
        QByteArray AnswerCGI;
        //если CGI модуль требует переклодировки - то  считываем данные с перекодировкой в UTF8 (нужно для текстовых ответов)
        if (CGIList[CurrentCGI].CodeToUTF8) {
            QTextCodec *Codec = QTextCodec::codecForName("Windows-1251");
            AnswerCGI = Codec->toUnicode(Process->readAllStandardOutput()).toUtf8();
        }
        else {
            AnswerCGI = QByteArray::fromBase64(Process->readAllStandardOutput());
        }

        qint64 SentByte = SendAnswer(200, AnswerCGI);
        emit WriteLog(MSG_CODE::CODE_OK, CurrentUID, CurrentCGI, "ID: " + QString::number(IDSession) + " Request processing completed. " +
                                                                 "Total received: " + QString::number(Request->Size()) + "B " +
                                                                 "Total sent: " +  QString::number(SentByte) + "B " +
                                                                 "Speed: " + QString::number(CurrentSpeed) + "KB/sec " +
                                                                 "Total time: " + QString::number(Time->msecsTo(QTime::currentTime())) + "ms");
    }
    else {
       SendAnswer(500);
       QTextCodec *Codec = QTextCodec::codecForName("Windows-1251");
       emit WriteLog(MSG_CODE::CODE_CGI_ERROR, CurrentUID, CurrentCGI, "ID: " + QString::number(IDSession) + " CGI module exited with an error. " +
                                                                       "Exit code: " + QString::number(exitCode) + " " +
                                                                       "Msg:" + Process->errorString() + " " +
                                                                       "Total received: " + QString::number(Request->Size()) + "B " +
                                                                       "Speed: " + QString::number(CurrentSpeed) + "KB/sec " +
                                                                       "Total time: " + QString::number(Time->msecsTo(QTime::currentTime())) + "ms " +
                                                                       "ErrorOut: " +  Codec->toUnicode(Process->readAllStandardError()));
    }
}

void TSocketThread::onCGIErrorOccurred(QProcess::ProcessError error)
{
   // SendAnswer(500); //Internal error
    QTextCodec *Codec = QTextCodec::codecForName("Windows-1251");
    emit WriteLog(MSG_CODE::CODE_CGI_ERROR, CurrentUID, CurrentCGI, "ID: " + QString::number(IDSession) + " CGI was crashed. " +
                                                                    "ErrorOut: " +  Codec->toUnicode(Process->readAllStandardError()));
    Stop();
}

void TSocketThread::onFinished()
{
    emit WriteLog(MSG_CODE::CODE_ERROR, CurrentUID, CurrentCGI, "ID: " + QString::number(IDSession) + " Stop signal received");
    Stop();
}



void TSocketThread::onDisconected()
{
    qDebug() <<  "ID:" << IDSession << "Disconnect";
    Stop();
}

void TSocketThread::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    emit WriteLog(MSG_CODE::CODE_ERROR, CurrentUID, CurrentCGI, "ID: " + QString::number(IDSession) + " Socket error: " + TcpSocket->errorString());
    Stop();
}

void TSocketThread::onTimeout()
{
    SendAnswer(524); //timeout
    emit WriteLog(MSG_CODE::CODE_ERROR, CurrentUID, CurrentCGI, "ID: " + QString::number(IDSession) + " Timeout data processing. " +
                                                                "Total received: " + QString::number(Request->Size()) + "B " +
                                                                "Total time: " + QString::number(Time->msecsTo(QTime::currentTime())));
    Stop();
}






