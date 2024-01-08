#include <QDebug>
#include "httpanswer.h"

HTTPAnswer::HTTPAnswer(uint16_t Code)
{
    AnswerStatus = "HTTP/1.1 " + QString::number(Code);
    if (AnswerCode.find(Code) != AnswerCode.end())
        AnswerStatus += ' ' + AnswerCode[Code];
}

HTTPAnswer::~HTTPAnswer()
{
}

QByteArray HTTPAnswer::GetAnswer()
{
    QByteArray Answer = AnswerStatus.toUtf8() + "\r\n";
    for (const auto &[Key, Value] : AnswerHeader.toStdMap()) {
        Answer += Key.toUtf8() + ": " + Value.toUtf8() + "\r\n";
       // qDebug() << Key.toUtf8() << ": " << Value.toUtf8();
    }
    //добавляем тег Content-Length если его нет
    if (AnswerHeader.find("Content-Length") == AnswerHeader.end()) {
        Answer += QByteArray("Content-Length: ") + QString::number(AnswerBody.size()).toUtf8() + QByteArray("\r\n");
    }
    if (AnswerHeader.find("Content-Type") == AnswerHeader.end()) {
        Answer += QByteArray("Content-Type: application/xml\r\n");
    }

    Answer += "\r\n";
    Answer += AnswerBody;
    SizeSent = Answer.size();
    return  QByteArray(Answer);
}

void HTTPAnswer::AddHeader(const QString& Key, const QString& Value)
{
    AnswerHeader.insert(Key, Value);
}

void HTTPAnswer::AddBody(const QByteArray& Body)
{
    AnswerBody += Body;
}

quint64 HTTPAnswer::Size() const
{
    return SizeSent;
}
