#include "config.h"

TConfig::TConfig(QString ConfigFileName)
    : FileName(ConfigFileName)
{
    QSettings IniFile(FileName, QSettings::IniFormat);
    IniFile.beginGroup("HTTPServer");
    HTTPServerConfig.Port = IniFile.value("Port", 80).toUInt();
    IniFile.endGroup();

    IniFile.beginGroup("SendFile");
    SendFileConfig.Path = IniFile.value("Path", "").toString();
    IniFile.endGroup();

    IniFile.beginGroup("CGI");
    CGIConfig.Path = IniFile.value("Path", "").toString();
    IniFile.endGroup();

    qDebug() << "Read configuration from " + FileName + " complite";

}

TConfig::~TConfig()
{
    QSettings IniFile(FileName, QSettings::IniFormat);
    IniFile.beginGroup("HTTPServer");
    IniFile.setValue("Port", HTTPServerConfig.Port);
    IniFile.endGroup();

    IniFile.beginGroup("SendFile");
    IniFile.setValue("Path", SendFileConfig.Path);
    IniFile.endGroup();

    IniFile.beginGroup("CGI");
    IniFile.setValue("Path", CGIConfig.Path);
    IniFile.endGroup();

    qDebug() << "Save configuration to " + FileName + " complite";
}
