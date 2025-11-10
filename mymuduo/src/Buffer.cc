#include<sys/uio.h>
#include<unistd.h>
#include<errno.h>

#include "Buffer.h"

ssize_t Buffer::readFd(int fd, int *saveErrno){
    size_t writable = writableBytes();
    char extrabuff[65536] = {0};
    iovec vec[2];
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writableBytes();
    vec[1].iov_base = extrabuff;
    vec[1].iov_len = sizeof extrabuff;
    int cnt = writable < sizeof extrabuff ? 2 : 1;
    int n = ::readv(fd, vec, cnt);
    if(n < 0){
        *saveErrno = errno;
    }else if(n <= writable){
        writerIndex_ += n;
    }else{
        writerIndex_ = buffer_.size();
        append(extrabuff, n - writable);
    }
    return n;
}
    
ssize_t Buffer::writeFd(int fd, int *saveErrno){
    int n = ::write(fd, peek(), readableBytes());
    if(n < 0){
        *saveErrno = errno;
    }
    return n;
}