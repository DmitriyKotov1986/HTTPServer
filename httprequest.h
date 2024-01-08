#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QByteArray>
#include <QString>
#include <QMap>

enum TRequestType {UNDEFINE, GET, POST, PUT};

class HTTPRequest
{
private:
    TRequestType RequestType  = TRequestType::UNDEFINE;
    QString RequestResurce;
    QString RequestProtocol;
    QByteArray RequestBody; //тело запроса
    QMap<QString, QString> RequestHeader;
    uint16_t ErrorCode = 204;
    QString ErrorMsg;
    qint64 LengthBody = 0;
    qint64 TotalGetSize = 0;
    bool HeaderParsed = false;
public:
    HTTPRequest();

    void Add(const QByteArray& Data);

    TRequestType GetType(); //возвращает тип запроса
    QByteArray GetBody(); //возвращает тело запроса
    QString GetHeader(const QString& Key); //возвращает заголовок
    QString GetResurce(); //возввращает запрашиваемый ресурс
    QString GetProtocol(); //название протокола (Обычно HTTP/1.1)
    uint GetErrorCode(); //код ошибки
    QString GetErrorMsg(); //текстовое сообщение об ошибке
    bool isComplite(); // возвращает истину если пришли все данные
    bool isGetHeader(); //возвращает истину если пришел заголовок и он разобран
    qint64 Size(); //общий обем пришедших данных
};

#endif // HTTPREQUEST_H
