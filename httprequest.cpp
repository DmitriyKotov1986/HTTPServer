#include "httprequest.h"
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>


HTTPRequest::HTTPRequest()
{
}

void HTTPRequest::Add(const QByteArray& Request)
{
    TotalGetSize += Request.size(); //суммируем объем принятых данных

    RequestBody += Request; //сохраняем пришедшие данные
    //проверяем пришел ли весь заголовок (должна содержаться пустая строчка)
    if (!HeaderParsed) {
        if (RequestBody.contains("\r\n\r\n")) { //найден заголовок - парсим его
            QTextStream RequestStr(RequestBody);

            RequestStr.setAutoDetectUnicode(true); //преобразуем в UNICODE

            QString tmp;
            RequestStr >> tmp; //читаем тип запроса
            if (tmp == "GET")
            {
                RequestType = TRequestType::GET;
            }
            else if (tmp == "POST")
            {
                RequestType = TRequestType::POST;
            }
            else if (tmp == "PUT")
            {
                RequestType = TRequestType::PUT;
            }
            else {
                ErrorCode = 501; //Bad request
                ErrorMsg = "Metod must be GET, PUT or POST";
                return;
            }
            RequestStr >> tmp; //читаем запрашиваемый ресурс
            tmp.remove(0,1);
            RequestResurce = tmp; //main.http

            RequestStr >> tmp; //читаем версию протокола
            RequestProtocol = tmp;      //HTTP/1.1
            if (RequestProtocol != "HTTP/1.1") {
                ErrorCode = 505; //Bad request
                ErrorMsg = "Protocol must be HTTP/1.1";
                return;
            }

            RequestStr.readLine(); //пропучскаем строчку
            //считываем заголовок
            while (!RequestStr.atEnd()) {
                QString Key = RequestStr.readLine();
                if (Key == "") break; //дальше идет тело запроса
                qsizetype Pos = Key.indexOf(":");
                QString Value = Key.mid(Pos + 2); //читаем строку до конца
                Key.remove(Pos, Key.length() - Pos);
                //Value.remove(0,1);
                RequestHeader.insert(Key, Value);
                //qDebug() << Key << "=====" << Value;
            }
            //остальное записываем в тело запроса
            RequestBody.clear();
            RequestBody = RequestStr.readAll().toUtf8(); //все остальное - тело запроса
            //qDebug() << "OK";
            ErrorCode = 200;
            ErrorMsg = "OK";

            if (GetHeader("Content-Length") != "") {
                LengthBody = GetHeader("Content-Length").toLongLong(); //0 если нет такого тега
            }
            else {
                LengthBody = 0;
            }

            HeaderParsed = true;
        }
    }
}

TRequestType HTTPRequest::GetType()
{
    return RequestType;
}

QByteArray HTTPRequest::GetBody()
{
    return RequestBody;
}

QString HTTPRequest::GetHeader(const QString& Key)
{
    if (RequestHeader.find(Key) != RequestHeader.end()) return RequestHeader[Key];
    else return "";
}

QString HTTPRequest::GetResurce()
{
    return RequestResurce;
}

QString HTTPRequest::GetProtocol()
{
    return RequestProtocol;
}

uint HTTPRequest::GetErrorCode()
{
    return ErrorCode;
}

QString HTTPRequest::GetErrorMsg()
{
    return ErrorMsg;
}

bool HTTPRequest::isComplite()
{
  //  qDebug() << (RequestBody.size() >= LengthBody);
    return (RequestBody.size() >= LengthBody);
}

bool HTTPRequest::isGetHeader()
{
    return HeaderParsed;
}

qint64 HTTPRequest::Size()
{
        return TotalGetSize;
}

