#pragma once

#include<vector>
#include<string>

class Buffer
{
private:
    size_t readerIndex_;
    size_t writerIndex_;
    std::vector<char> buffer_;

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

    ssize_t readFd(int fd, int *saveErrno);
    ssize_t writeFd(int fd, int *saveErrno);
};