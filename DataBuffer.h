#ifndef DATABUFFER_H
#define DATABUFFER_H
#include <QMutex>

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

class DataBuffer{
private:
    char * buffer;
    int	bufsize;
    volatile int write_ptr;
    volatile int read_ptr;
    QMutex read_mutex;
    QMutex write_mutex;
public:
    DataBuffer(int size);
    ~DataBuffer();
    int getReadSpace();
    int getWriteSpace();
    void setWritePos(int pos);
    int setReadPos(int pos);
    int Read( char *dest, int cnt);
    int Write( char *stc, int cnt);
    void Reset();
    int64_t seek(int offset,int whence);
};

#ifdef __cplusplus
}
#endif

#endif // DATABUFFER_H
