#ifndef HTTPSERVICE_H
#define HTTPSERVICE_H

#include <QtService/QtService>
#include <QString>
#include <QSettings>
#include <memory>
#include <QSharedPointer>
#include "httpserver.h"

class HTTPService : public QtService<QCoreApplication>
{
public:
   explicit HTTPService(int argc, char **argv);
   virtual ~HTTPService();

protected:
    void start() override; //выполнятся при запуске службы
    void pause() override; //выполняется при остановки службы на паузу
    void resume() override;
    void stop() override; //выполняется при остановке службы

private:
    QSharedPointer<QSettings> Config; //Файл конфигурации службы
    std::unique_ptr<THTTPServer> HTTPServer; //класс HTTPServer
};


#endif // HTTPSERVICE_H
