/* Copyright (c) 2017-2019 Rolf Timmermans */
#pragma once

#include "prefix.h"

#include "outgoing_msg.h"
#include "poller.h"

namespace zmq {
class Module;

class Socket : public Napi::ObjectWrap<Socket>, public Closable {
public:
    static void Initialize(Module& module, Napi::Object& exports);

    explicit Socket(const Napi::CallbackInfo& info);
    virtual ~Socket();

    void Close() override;

protected:
    enum class State : uint8_t {
        Open, /* Socket is open. */
        Closed, /* Socket is closed. */
        Blocked, /* Async operation in progress that disallows socket access. */
    };

    inline void Close(const Napi::CallbackInfo& info);

    inline Napi::Value Bind(const Napi::CallbackInfo& info);
    inline Napi::Value Unbind(const Napi::CallbackInfo& info);

    inline void Connect(const Napi::CallbackInfo& info);
    inline void Disconnect(const Napi::CallbackInfo& info);

    inline Napi::Value Send(const Napi::CallbackInfo& info);
    inline Napi::Value Receive(const Napi::CallbackInfo& info);

    inline void Join(const Napi::CallbackInfo& info);
    inline void Leave(const Napi::CallbackInfo& info);

    template <typename T>
    inline Napi::Value GetSockOpt(const Napi::CallbackInfo& info);

    template <typename T>
    inline void SetSockOpt(const Napi::CallbackInfo& info);

    inline Napi::Value GetEvents(const Napi::CallbackInfo& info);
    inline Napi::Value GetContext(const Napi::CallbackInfo& info);

    inline Napi::Value GetClosed(const Napi::CallbackInfo& info);
    inline Napi::Value GetReadable(const Napi::CallbackInfo& info);
    inline Napi::Value GetWritable(const Napi::CallbackInfo& info);

private:
    inline void WarnUnlessImmediateOption(int32_t option) const;
    inline bool ValidateOpen() const;
    bool HasEvents(int32_t events) const;

    /* Send/receive are usually in a hot path and will benefit slightly
       from being inlined. They are used in more than one location and are
       not necessarily automatically inlined by all compilers. */
    force_inline void Send(const Napi::Promise::Deferred& res, OutgoingMsg::Parts& parts);
    force_inline void Receive(const Napi::Promise::Deferred& res);

    class Poller : public zmq::Poller<Poller> {
        Socket& socket;
        Napi::Promise::Deferred read_deferred;
        Napi::Promise::Deferred write_deferred;
        OutgoingMsg::Parts write_value;

    public:
        explicit Poller(Socket& socket)
            : socket(socket), read_deferred(socket.Env()), write_deferred(socket.Env()) {}

        Napi::Value ReadPromise(int64_t timeout);
        Napi::Value WritePromise(int64_t timeout, OutgoingMsg::Parts&& parts);

        inline bool ValidateReadable() const {
            return socket.HasEvents(ZMQ_POLLIN);
        }

        inline bool ValidateWritable() const {
            return socket.HasEvents(ZMQ_POLLOUT);
        }

        void ReadableCallback();
        void WritableCallback();
    };

    Napi::AsyncContext async_context;
    Napi::ObjectReference context_ref;
    Napi::ObjectReference observer_ref;
    Socket::Poller poller;

    Module& module;
    void* socket = nullptr;

    int64_t send_timeout = -1;
    int64_t receive_timeout = -1;
    uint32_t sync_operations = 0;
    uint32_t endpoints = 0;

    State state = State::Open;
    bool request_close = false;
    bool thread_safe = false;
    uint8_t type = 0;

    friend class Observer;
    friend class Proxy;
};
}

static_assert(!std::is_copy_constructible<zmq::Socket>::value, "not copyable");
static_assert(!std::is_move_constructible<zmq::Socket>::value, "not movable");
