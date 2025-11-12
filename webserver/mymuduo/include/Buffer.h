#pragma once

#include <cstring>
#include<vector>
#include<string>
#include<string.h>

class Buffer
{
private:
    size_t readerIndex_;
    size_t writerIndex_;
    std::vector<char> buffer_;
    const char* CRLF = "\r\n";

    char *begin(){ return &*buffer_.begin(); }
    const char *begin() const{ return &*buffer_.begin(); }

    void makeSpace(size_t len){
        if(writableBytes() + prependerBytes() < kCheapPrepender + len){
            buffer_.resize(writerIndex_ + len);
        }else{
            size_t readable = readableBytes();
            std::copy(peek(), peek() + readableBytes(), begin() + kCheapPrepender);
            readerIndex_ = kCheapPrepender;
            writerIndex_ = kCheapPrepender + readable; 
        }
    }
public:
    static const size_t kCheapPrepender = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        :readerIndex_(kCheapPrepender)
        ,writerIndex_(kCheapPrepender)
        ,buffer_(kCheapPrepender + initialSize)
    {}

    size_t readableBytes() const{ return writerIndex_ - readerIndex_; }
    size_t writableBytes() const{ return buffer_.size() - writerIndex_; }
    size_t prependerBytes() const{ return readerIndex_; }
    const char *peek(){ return begin() + readerIndex_; }

    std::string retrieveAllAsString(){ return retrieveAsString(readableBytes()); }
    std::string retrieveAsString(size_t len){
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    void retrieve(size_t len){
        if(len >= readableBytes()){
            retrieveAll();
        }else{
            readerIndex_ += len;
        }
    }
    void retrieveAll(){
        readerIndex_ = kCheapPrepender;
        writerIndex_ = kCheapPrepender;
    }

    void ensureWritableBytes(size_t len){
        if(writableBytes() < len){
            makeSpace(len);
        }
    }
    char *beginWrite(){ return begin() + writerIndex_; }
    const char *beginWrite() const{ return begin() + writerIndex_; }
    
    void append(const char *data, int len){
        makeSpace(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    void append(const char *data){
        int len = std::strlen(data);
        append(data, len);
    }

    const char* findCRLF() const {
        const char* start = begin() + readerIndex_;
        const char* end = begin() + writerIndex_;
        // 确保有至少两个字符的空间
        while (start < end - 1) {
            if (start[0] == CRLF[0] && start[1] == CRLF[1]) {
                return start;
            }
            start++;
        }
        return nullptr; // 没有找到
    }

    void retrieveUntil(const char *readEnd){
        char* start = begin() + readerIndex_;
        char* end = begin() + writerIndex_;
        if(readEnd > end){
            readEnd = end;
        }
        //start = readerIndex_ + readEnd - begin() - readerIndex_ + begin() = readEnd
        readerIndex_ += (readEnd - start);
    }

    ssize_t readFd(int fd, int *saveErrno);
    ssize_t writeFd(int fd, int *saveErrno);
};