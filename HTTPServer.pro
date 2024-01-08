QT -= gui
QT += network
QT += sql
# QT += core5compat

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        QtService/qtservice.cpp \
        QtService/qtservice_win.cpp \
        httpanswer.cpp \
        httprequest.cpp \
        httpserver.cpp \
        httpservice.cpp \
        main.cpp \
        tsocketthread.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    QtService/QtService \
    QtService/QtServiceBase \
    QtService/QtServiceController \
    QtService/qtservice.h \
    QtService/qtservice_p.h \
    httpanswer.h \
    httprequest.h \
    httpserver.h \
    httpservice.h \
    tsocketthread.h

SUBDIRS += \
    QtService/service.pro
