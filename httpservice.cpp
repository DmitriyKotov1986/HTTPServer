#include <QFileInfo>
#include <QSettings>
#include <QDir>
#include "httpservice.h"

HTTPService::HTTPService(int argc, char **argv)
    : QtService<QCoreApplication>(argc, argv, "HTTP Service")
{
    try {
        setServiceDescription("HTTPServer by iK");
        setServiceFlags(QtServiceBase::CanBeSuspended);

     }  catch (const std::exception &e) {
        qCritical() << "Criticall error on costruction service. Message:" << e.what();
        exit(-1);
    }
}

HTTPService::~HTTPService()
{
}

void HTTPService::start()
{
    try {
        qDebug() << "Start service";

        QString ConfigFileName = application()->applicationDirPath() + "/HTTPServer.ini";
        QDir::setCurrent(application()->applicationDirPath()); //меняем текущую директорию

        qDebug() << "Reading configuration from :" + ConfigFileName;

        QFileInfo FileInfo(ConfigFileName);
        if (!FileInfo.exists()) {
            throw std::runtime_error("Configuration file not found.");
        }

        Config = QSharedPointer<QSettings>(new QSettings(ConfigFileName, QSettings::IniFormat));

        HTTPServer = std::make_unique<THTTPServer>(Config);

        HTTPServer->Listing();
    }  catch (const std::exception &e) {
        qCritical() << "Critical error on start service. Message:" << e.what();
        exit(-1);
    }
}

void HTTPService::pause()
{
    try {
        qDebug() << "Pause service";
        if (HTTPServer) HTTPServer->DisconnectAll();
    }  catch (const std::exception &e) {
        qCritical() << "Critical error on pause service. Message:" << e.what();
        exit(-1);
    }
}

void HTTPService::resume()
{
    try {
       qDebug() << "Resume service";
       if (HTTPServer) HTTPServer->Listing();
    }  catch (const std::exception &e) {
        qCritical() << "Critical error on resume service. Message:" << e.what();
        exit(-1);
    }
}

void HTTPService::stop()
{
    try {
        qDebug() << "Stop service";
        if (HTTPServer) HTTPServer->DisconnectAll();
    }  catch (const std::exception &e) {
        qCritical() << "Critical error on stop service. Message:" << e.what();
        exit(-1);
    }
}


