#include "SslConnection.h"
#include "Logger.h"
#include <openssl/err.h>


// 自定义 BIO 方法
static BIO_METHOD* createCustomBioMethod() 
{
    BIO_METHOD* method = BIO_meth_new(BIO_TYPE_MEM, "custom");
    BIO_meth_set_write(method, SslConnection::bioWrite);
    BIO_meth_set_read(method, SslConnection::bioRead);
    BIO_meth_set_ctrl(method, SslConnection::bioCtrl);
    return method;
}


SslConnection::SslConnection(const TcpConnectionPtr& conn, SslContext* ctx)
: ssl_(nullptr)
, ctx_(ctx)
, conn_(conn)
, state_(SSLState::HANDSHAKE)
, readBio_(nullptr)
, writeBio_(nullptr)
, messageCallback_(nullptr)
{
    // 接收消息（网络 -> 应用层）：
    // 当从网络接收到加密数据时，我们将这些数据写入到readBio_（使用BIO_write）。
    // 然后调用SSL_read（在SSL连接已建立后）从SSL对象中读取解密后的数据。SSL对象会从readBio_中读取加密数据进行解密。
    // 发送消息（应用层 -> 网络）：
    // 当应用层要发送数据时，我们调用SSL_write将数据写入SSL对象。
    // SSL对象会加密数据，并将加密后的数据写入writeBio_（我们可以通过BIO_read从writeBio_中读取加密数据）。
    // 然后我们从writeBio_中读取加密数据（使用BIO_read）并发送到网络。
    // 创建 SSL 对象
    //核心：写入ssl加密再从write读出，写入read解密再从ssl读出
    ssl_ = SSL_new(ctx_->getNativeHandle());
    if (!ssl_) {
        LOG_ERROR("Failed to create SSL object: %s\n", ERR_error_string(ERR_get_error(), nullptr));
        return;
    }

    // 创建 BIO
    readBio_ = BIO_new(BIO_s_mem());
    writeBio_ = BIO_new(BIO_s_mem());
    
    if (!readBio_ || !writeBio_) {
        LOG_ERROR("Failed to create BIO objects\n");
        SSL_free(ssl_);
        ssl_ = nullptr;
        return;
    }

    SSL_set_bio(ssl_, readBio_, writeBio_);
    //SSL_set_accept_state(ssl_);  // 设置为服务器模式
    
    // 设置 SSL 选项
    SSL_set_mode(ssl_, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    SSL_set_mode(ssl_, SSL_MODE_ENABLE_PARTIAL_WRITE);
    
    // 设置连接回调
    conn_->setMessageCallback(
        std::bind(&SslConnection::onRead, this, std::placeholders::_1,
                 std::placeholders::_2, std::placeholders::_3));
}

SslConnection::~SslConnection() 
{
    if (ssl_) 
    {
        SSL_free(ssl_);  // 这会同时释放 BIO
    }
}

void SslConnection::startHandshake() 
{
    LOG_DEBUG("SslConnection::startHandshake\n");
    SSL_set_accept_state(ssl_);
    handleHandshake();
}

void SslConnection::send(const char* data, size_t len) 
{
    if (state_ != SSLState::ESTABLISHED) {
        LOG_ERROR("Cannot send data before SSL handshake is complete\n");
        return;
    }
    
    int written = SSL_write(ssl_, data, len);
    //写入进行加密并把加密数据放到writeBio_，因为从SSL输出加密数据所以是write
    if (written <= 0) {
        int err = SSL_get_error(ssl_, written);
        LOG_ERROR("SSL_write failed: %s\n", ERR_error_string(err, nullptr));
        return;
    }
    
    char buf[4096];
    int pending;
    while ((pending = BIO_pending(writeBio_)) > 0) {
        int bytes = BIO_read(writeBio_, buf, 
                           std::min(pending, static_cast<int>(sizeof(buf))));
        LOG_DEBUG("send %d bytes\n", bytes);
        if (bytes > 0) {
            std::string sendMsg(buf, bytes);
            conn_->send(sendMsg);
        }
    }
    if(written < len){
        send(data + written, len - written);
    }
}

void SslConnection::onRead(const TcpConnectionPtr& conn, BufferPtr buf, 
                         TimeStamp time) 
{
    LOG_DEBUG("SslConnection::onRead\n");
    if (state_ == SSLState::HANDSHAKE) {
        // 将数据写入 BIO，SSL读BIO将解密数据放到SSL缓冲区
        BIO_write(readBio_, buf->peek(), buf->readableBytes());
        buf->retrieve(buf->readableBytes());
        handleHandshake();
    }
    if (state_ == SSLState::ESTABLISHED) {
        // 将数据写入 BIO，SSL读BIO将解密数据放到SSL缓冲区
        int written = BIO_write(readBio_, buf->peek(), buf->readableBytes());
        bool isFull = false;
        if (written > 0) {
            buf->retrieve(written);  // 只清除成功写入的数据
        }else if (BIO_should_retry(readBio_)) {
            LOG_DEBUG("BIO已满，需要等待可写\n");
            isFull = true;
            // BIO已满，需要等待可写
            // 保持buf中的数据，稍后重试
        }
        // 解密数据
        char decryptedData[4096];
        //从ssl？中读取解密文件，因为是BIO读外部数据所以是read
        int ret = SSL_read(ssl_, decryptedData, sizeof(decryptedData));
        while (ret > 0) {
            // 调用上层回调处理解密后的数据
            LOG_DEBUG("ret:%d  SslConnection::onRead %.900s\n", ret, decryptedData);
            decryptedBuffer_.append(decryptedData, ret);
            ret = SSL_read(ssl_, decryptedData, sizeof(decryptedData));
            // 创建新的 Buffer 存储解密后的数据
        }
        if(isFull){
            onRead(conn, buf, time);
        }
        if(messageCallback_ && decryptedBuffer_.readableBytes() > 0){
            messageCallback_(conn, &decryptedBuffer_, time);
        }
    }
}

void SslConnection::handleHandshake() 
{
    LOG_DEBUG("SslConnection::handleHandshake\n");
    int ret = SSL_do_handshake(ssl_);
    
    if (ret == 1) {
        state_ = SSLState::ESTABLISHED;
        LOG_INFO("SSL handshake completed successfully\n");
        LOG_INFO("Using cipher: %s\n", SSL_get_cipher(ssl_));
        LOG_INFO("Protocol version: %s\n", SSL_get_version(ssl_));
        
        // 握手完成后，确保设置了正确的回调
        if (!messageCallback_) {
            LOG_ERROR("No message callback set after SSL handshake\n");
        }
        sendPendingData();
        return;
    }
    
    int err = SSL_get_error(ssl_, ret);
    switch (err) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // 正常的握手过程，需要继续
            sendPendingData();
            break;
            
        default: {
            // 获取详细的错误信息
            char errBuf[256];
            unsigned long errCode = ERR_get_error();
            ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
            LOG_ERROR("SSL handshake failed: %s\n", errBuf);
            conn_->shutdown();  // 关闭连接
            break;
        }
    }
}

// 添加一个辅助函数，用于发送 writeBio_ 中的待发送数据
void SslConnection::sendPendingData()
{
    char buf[4096];
    int pending;
    while ((pending = BIO_pending(writeBio_)) > 0) {
        int bytes = BIO_read(writeBio_, buf, 
                           std::min(pending, static_cast<int>(sizeof(buf))));
        if (bytes > 0) {
            std::string sendMsg(buf, bytes);
            conn_->send(sendMsg);
        }
    }
}

//修改过
void SslConnection::onEncrypted(const char* data, size_t len) 
{
    writeBuffer_.append(data, len);
    std::string writeBytes = writeBuffer_.retrieveAllAsString();
    conn_->send(writeBytes);
}

void SslConnection::onDecrypted(const char* data, size_t len) 
{
    decryptedBuffer_.append(data, len);
}

SSLError SslConnection::getLastError(int ret) 
{
    int err = SSL_get_error(ssl_, ret);
    switch (err) 
    {
        case SSL_ERROR_NONE:
            return SSLError::NONE;
        case SSL_ERROR_WANT_READ:
            return SSLError::WANT_READ;
        case SSL_ERROR_WANT_WRITE:
            return SSLError::WANT_WRITE;
        case SSL_ERROR_SYSCALL:
            return SSLError::SYSCALL;
        case SSL_ERROR_SSL:
            return SSLError::SSL;
        default:
            return SSLError::UNKNOWN;
    }
}

void SslConnection::handleError(SSLError error) 
{
    switch (error) 
    {
        case SSLError::WANT_READ:
        case SSLError::WANT_WRITE:
            // 需要等待更多数据或写入缓冲区可用
            break;
        case SSLError::SSL:
        case SSLError::SYSCALL:
        case SSLError::UNKNOWN:
            LOG_ERROR("SSL error occurred: %s\n", ERR_error_string(ERR_get_error(), nullptr));
            state_ = SSLState::ERROR;
            conn_->shutdown();
            break;
        default:
            break;
    }
}
//将data数据写入并且发送
int SslConnection::bioWrite(BIO* bio, const char* data, int len) 
{
    SslConnection* conn = static_cast<SslConnection*>(BIO_get_data(bio));
    if (!conn) return -1;
    std::string sendMsg(data, len);
    conn->conn_->send(sendMsg);
    return len;
}
//读入data
int SslConnection::bioRead(BIO* bio, char* data, int len) 
{
    SslConnection* conn = static_cast<SslConnection*>(BIO_get_data(bio));
    if (!conn) return -1;

    size_t readable = conn->readBuffer_.readableBytes();
    if (readable == 0) 
    {
        return -1;  // 无数据可读
    }

    size_t toRead = std::min(static_cast<size_t>(len), readable);
    memcpy(data, conn->readBuffer_.peek(), toRead);
    conn->readBuffer_.retrieve(toRead);
    return toRead;
}

long SslConnection::bioCtrl(BIO* bio, int cmd, long num, void* ptr) 
{
    switch (cmd) 
    {
        case BIO_CTRL_FLUSH:
            return 1;
        default:
            return 0;
    }
}
