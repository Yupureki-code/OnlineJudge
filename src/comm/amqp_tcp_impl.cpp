// src/comm/amqp_tcp_impl.cpp
// Minimal AMQP::TcpConnection implementation — self-contained TCP transport
//
// IMPORTANT: AMQP-CPP headers must be included in the correct order.
// tcpconnection.h does NOT include its own dependencies — they must be
// included before it, exactly as the original `includes.h` does.

#include <amqpcpp.h>          // core protocol library
#include <algorithm>
#include <sys/types.h>         // for connectToHost()
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <stdexcept>

// TCP module headers — MUST be included in this order
#include <amqpcpp/linux_tcp/tcpparent.h>
#include <amqpcpp/linux_tcp/tcphandler.h>
#include <amqpcpp/linux_tcp/tcpconnection.h>

#include "amqp_tcpstate.h"     // local copy — not installed by AMQP-CPP

namespace AMQP {

// ==================== Minimal TcpState subclass ====================

/// SimpleTcpState — a single TcpState that handles both connecting and connected phases
class SimpleTcpState : public TcpState
{
public:
    SimpleTcpState(TcpParent* parent, int sockfd)
        : TcpState(parent), _sockfd(sockfd) {}
    ~SimpleTcpState() override { if (_sockfd >= 0) ::close(_sockfd); }

    int fileno() const override { return _sockfd; }
    bool closed() const override { return _closed; }
    std::size_t queued() const override { return _outbuf.size(); }

    TcpState* process(const Monitor& monitor, int fd, int flags) override
    {
        (void)monitor;
        (void)fd;

        // Write queued output
        if ((flags & AMQP::writable) && !_outbuf.empty()) {
            ssize_t n = ::send(_sockfd, _outbuf.data(), _outbuf.size(), MSG_NOSIGNAL);
            if (n > 0) _outbuf.erase(0, n);
            else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                _closed = true;
                _parent->onLost(this);
                return this;
            }
        }

        // Read incoming data
        if (flags & AMQP::readable) {
            char buf[65536];
            ssize_t n = ::recv(_sockfd, buf, sizeof(buf), 0);
            if (n > 0) {
                _inbuf.append(buf, static_cast<size_t>(n));
                ByteBuffer bb(_inbuf.data(), _inbuf.size());
                const size_t consumed = _parent->onReceived(this, bb);
                if (consumed > 0) _inbuf.erase(0, std::min(consumed, _inbuf.size()));
            } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                _closed = true;
                _parent->onLost(this);
            }
        }
        return this;
    }

    /// Buffer outgoing data (called by onData via the parent)
    void sendData(const char* data, size_t size) {
        _outbuf.append(data, size);
    }

private:
    int _sockfd;
    bool _closed = false;
    std::string _outbuf;
    std::string _inbuf;
};

// ==================== Helper: connect to RabbitMQ ====================

static int connectToHost(const Address& address)
{
    const char* host = address.hostname().c_str();
    int port = address.port();
    if (port == 0) port = 5672;

    struct addrinfo hints{}, *result;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int ret = getaddrinfo(host, port_str, &hints, &result);
    if (ret != 0) return -1;

    int sockfd = -1;
    for (struct addrinfo* rp = result; rp; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd < 0) continue;

        // Set non-blocking
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        // Set close-on-exec
        fcntl(sockfd, F_SETFD, FD_CLOEXEC);

        if (::connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        if (errno == EINPROGRESS) break; // non-blocking connect in progress

        ::close(sockfd);
        sockfd = -1;
    }
    freeaddrinfo(result);
    return sockfd;
}

// ==================== TcpConnection Methods ====================

TcpConnection::TcpConnection(TcpHandler* handler, const Address& address)
    : _handler(handler)
    , _state(new SimpleTcpState(this, connectToHost(address)))
    , _connection(this, address.login(), address.vhost())
{
    const int sockfd = fileno();
    if (sockfd >= 0 && _handler) {
        _handler->onAttached(this);
        _handler->monitor(this, sockfd, AMQP::readable | AMQP::writable);
    } else if (_handler) {
        _handler->onDetached(this);
    }
}

TcpConnection::~TcpConnection() noexcept
{
    int fd = fileno();
    if (fd >= 0 && _handler) {
        _handler->monitor(this, fd, 0); // unregister from event loop
    }
    _handler = nullptr;
    // _state and _connection are automatically destroyed
}

void TcpConnection::process(int fd, int flags)
{
    if (!_state) return;

    Monitor monitor(this);
    auto* s = static_cast<SimpleTcpState*>(_state.get());
    s->process(monitor, fd, flags);
}

bool TcpConnection::close(bool immediate)
{
    (void)immediate;
    if (_handler) {
        _handler->onDetached(this);
    }
    return true;
}

bool TcpConnection::closed() const
{
    return _state ? _state->closed() : true;
}

// ==================== ConnectionHandler overrides ====================

/// Called by AMQP protocol layer when it wants to send data to the network
void TcpConnection::onData(Connection* connection, const char* buffer, size_t size)
{
    (void)connection;
    auto* s = static_cast<SimpleTcpState*>(_state.get());
    if (s) {
        s->sendData(buffer, size);
    }
    int fd = fileno();
    if (fd >= 0 && _handler) {
        _handler->monitor(this, fd, AMQP::readable | AMQP::writable);
    }
}

void TcpConnection::onClosed(Connection* connection)
{
    (void)connection;
}

void TcpConnection::onProperties(Connection* connection, const Table& server, Table& client)
{
    (void)connection; (void)server; (void)client;
}

uint16_t TcpConnection::onNegotiate(Connection* connection, uint16_t interval)
{
    (void)connection;
    return interval;
}

int TcpConnection::fileno() const
{
    return _state ? _state->fileno() : -1;
}

std::size_t TcpConnection::queued() const
{
    return _state ? _state->queued() : 0;
}

void TcpConnection::onError(Connection* connection, const char* message)
{
    (void)connection;
    if (_handler) _handler->onError(this, message);
}

// ==================== TcpParent overrides ====================

void TcpConnection::onError(TcpState* state, const char* message, bool connected)
{
    (void)state;
    (void)connected;
    _connection.fail(message == nullptr ? "TCP connection error" : message);
    if (_handler) _handler->onError(this, message);
}

void TcpConnection::onLost(TcpState* state)
{
    (void)state;
    _connection.fail("TCP connection lost");
    if (_handler) _handler->onDetached(this);
}

} // namespace AMQP
