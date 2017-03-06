#include "CommandHandler.h"

CommandHaldler::CommandHaldler(QObject* parent):
    QThread(parent){
    qDebug()<<"CommandHaldler comstructor";
}

void CommandHaldler::run(){
    qDebug()<<"CommandHaldler run";

    command = new QUdpSocket();
    checkstopflag = false;
    int command_port = 40001;
    bool result = command->bind(command_port, QUdpSocket::ShareAddress);
    if(!result) {
        qDebug()<<"udp bind error\n";
        return ;
    }
    connect(command, SIGNAL(readyRead()),this, SLOT(command_come()), Qt::DirectConnection);
    exec();
}

void CommandHaldler::command_come(){
    if(command->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress *hostaddr = new QHostAddress;
        quint16 * hostport = new  quint16;
        datagram.resize(command->pendingDatagramSize());

        command->readDatagram(datagram.data(), datagram.size(),hostaddr,hostport);
        QString host_ip = hostaddr->toString();
        host_ip = host_ip.mid(host_ip.lastIndexOf(":")+1);
        qDebug()<<"device scan, receive:"<<datagram.data()<<", ip:"<<host_ip<<", thread id:"<<currentThreadId();
        if(strcmp(datagram.data(),"f")==0){
            checkstopflag = true;
        }

        handlemsg(datagram.data(),host_ip);

        delete hostaddr;
        delete hostport;
    }
}
