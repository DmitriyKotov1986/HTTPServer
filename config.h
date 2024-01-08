#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QSettings>


struct THTTPServerConfig {
    uint16_t Port = 80;
};

struct TSendFileConfig {
   QString Path = "";
};

struct TCGIConfig {
   QString Path = "";
};


class TConfig
{
public:
    explicit TConfig(QString ConfigFileName = "Config.ini");
    ~TConfig();

    THTTPServerConfig HTTPServerConfig;
    TSendFileConfig SendFileConfig;
    TCGIConfig CGIConfig;
private:
    QString FileName;
};

#endif // CONFIG_H
