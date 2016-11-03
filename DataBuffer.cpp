#define LOG_TAG "NativeBuffer"
#define DGB 1
#include "DataBuffer.h"
#include <QDebug>

DataBuffer::DataBuffer(int size){
    //printf("create DataBuffer");
    buffer=(char*)malloc(size);
    bufsize=size;
    write_ptr=0;
    read_ptr=0;
}
DataBuffer::~DataBuffer(){
    if(buffer!=NULL)
        free(buffer);
    //printf("delete DataBuffer");
}
int DataBuffer::getReadSpace(){
    return write_ptr-read_ptr;
}

int DataBuffer::getWriteSpace(){
    read_mutex.lock();
    write_mutex.lock();
    int res = bufsize-(write_ptr-read_ptr);
//    qDebug()<<"getWriteSpace, read_ptr = "<<read_ptr<<", write_ptr = "<<write_ptr<<", res = "<<res;
    write_mutex.unlock();
    read_mutex.unlock();
    return res;
}

void DataBuffer::setWritePos(int pos){
    write_ptr=pos;
}

int DataBuffer::setReadPos(int pos){
    read_mutex.lock();
    read_ptr=pos;
    read_mutex.unlock();
    return read_ptr;
}

int DataBuffer::Read( char *dest, int cnt){
    read_mutex.lock();
    int curptr=read_ptr%bufsize;
    if(bufsize-curptr>=cnt){
        memcpy(dest,buffer+curptr,cnt);
        read_ptr+=cnt;
        read_mutex.unlock();
        return cnt;
    }else{
        int n1=bufsize-curptr;
        memcpy(dest,buffer+curptr,n1);
        int n2=cnt-n1;
        memcpy(dest+n1,buffer,n2);
        read_ptr+=cnt;
        read_mutex.unlock();
        return cnt;
    }

}

int DataBuffer::Write(char *src, int cnt){
    int capacity = getWriteSpace();
    if(capacity>=cnt){
        write_mutex.lock();
        int curptr=write_ptr%bufsize;
        if(bufsize-curptr>=cnt){
            memcpy(buffer+curptr,src,cnt);
            write_ptr+=cnt;
            write_mutex.unlock();
            return cnt;
        }else{
            int n1=bufsize-curptr;
            memcpy(buffer+curptr,src,n1);
            int n2=cnt-n1;
            memcpy(buffer,src+n1,n2);
            write_ptr+=cnt;
            write_mutex.unlock();
            return cnt;
        }
    }else return 0;

}

void DataBuffer::Reset(){
    read_ptr=0;
    write_ptr=0;
    memset(buffer,0,bufsize);
}

int64_t DataBuffer::seek(int offset, int whence){
    if(write_ptr>bufsize) {
        return -1;
    }
    return setReadPos(whence);
}
