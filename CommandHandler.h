#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <QThread>
#include <QtNetwork>
#include <QDebug>

class CommandHaldler : public QThread{
    Q_OBJECT
public:
    explicit CommandHaldler(QObject * parent = 0);
    void run() Q_DECL_OVERRIDE;
signals:
    void handlemsg(char* msg, const QString & host_ip);
private slots:
    void command_come();
private:
    QUdpSocket* command;
public:
    volatile bool checkstopflag;
};

#endif // COMMANDHANDLER_H
