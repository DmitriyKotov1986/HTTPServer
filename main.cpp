#include <QCoreApplication>
#include <QtService/QtService>
#include <QScopedPointer>
#include "httpservice.h"

int main(int argc, char *argv[])
{
    setlocale(LC_CTYPE, ""); //настраиваем локаль

    auto service = QScopedPointer<HTTPService>(new HTTPService(argc, argv));

    return service->exec();
}
