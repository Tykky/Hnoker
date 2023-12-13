#include "message_types.hpp"
#include "networking.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <strstream>

namespace hnoker 
{
    using boost::asio::ip::tcp;
    using boost::asio::ip::address;
    using boost::asio::io_context;
    using boost::asio::signal_set;
    using boost::asio::deadline_timer;
    using boost::asio::io_service;
    using boost::asio::awaitable;
    using boost::system::system_error;
    using boost::asio::socket_base;
    using boost::asio::co_spawn;
    using boost::asio::detached;
    using boost::asio::use_awaitable;
    namespace this_coro = boost::asio::this_coro;

    awaitable<void> tcp_server_session(tcp::socket socket, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op);
    awaitable<void> tcp_client_session(tcp::socket socket, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op);
    awaitable<void> accept_tcp_connections(const uint16_t port, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op);
    awaitable<void> connect_to_tcp_server(const std::string_view host, const uint16_t port, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op);

    struct network_context 
    {
        network_context() :
            boost_ctx(1)
        {}

        io_context boost_ctx;
    };

    network_context* allocate_network_context()
    {
        return new network_context;
    }

    void deallocate_network_context(network_context* network_context)
    {
        delete network_context;
    }
    
    void run_impl(network_context* ctx)
    {
        ctx->boost_ctx.run();
    }

    static void handle_network_eptr(std::exception_ptr eptr)
    {
        try
        {
            if (eptr)
                std::rethrow_exception(eptr);
        }
        catch (const boost::exception& e)
        {
            std::string info = boost::diagnostic_information(e);
            INFO("NETWORK ERROR ({}:{}): {}", __FILE__, __LINE__, info);
            throw std::runtime_error("Network error");
        }
    }

    void async_create_server_impl(network_context* ctx, uint16_t port, std::span<char> read_buf, std::span<char> write_buf,  const read_write_op_t& read_write_op)
    {
        co_spawn(ctx->boost_ctx, accept_tcp_connections(port, read_buf, write_buf, read_write_op), [](std::exception_ptr ep) {
            handle_network_eptr(ep);
        });
    }

    void async_connect_server_impl(network_context* ctx, std::string_view address, uint16_t port, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op)
    {
        co_spawn(ctx->boost_ctx, connect_to_tcp_server(address, port, read_buf, write_buf, read_write_op), [](std::exception_ptr ep) {
            handle_network_eptr(ep);
        });
    }

    void write_message_to_buffer(std::span<char>& buffer, const Message& m)
    {
        INFO("Writing message from buffer, type as integer is {}", +(static_cast<char>(m.type)));
        buffer[0] = static_cast<char>(m.type);

        std::span<char> archive_buffer{buffer.begin() + 1, buffer.end()};
        std::ostrstream output_stream(archive_buffer.data(), (int) archive_buffer.size());

        boost::archive::text_oarchive oa{output_stream};
        try
        {
            // t�� ei toimi jos messagessa on C array esim ClientList, tolle
            // arraylle oli kai joku oma archive juttu boost::serialization::make_array
            // https://www.boost.org/doc/libs/1_78_0/libs/serialization/doc/wrappers.html#:~:text=boost%3A%3Aserialization%3A%3Amake_array(T*%20t%2C%20std%3A%3Asize_t%20size)%3B
            oa << m;
        }
        catch (std::exception& e)
        {
            INFO("Archiving busted, pls fix");
            // pit�� viel fixaa n�� exceptionit 
            throw std::runtime_error("archiving bad and busted");
        }
    }

    Message read_message_from_buffer(const std::span<char>& buffer)
    {
        Message m { static_cast<MessageType>(buffer[0]) };
        std::istrstream input_stream(buffer.data() + 1, (int) buffer.size() - 1);
        boost::archive::text_iarchive ia{input_stream};
        try
        {
            ia >> m;
        }
        catch (std::exception& e)
        {
            INFO("Read message busted, pls fix!");
            throw std::runtime_error("read message busted");
        }
        return m;
    }

    awaitable<void> tcp_server_session(tcp::socket socket, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op)
    {
        try
        {
            auto ip = socket.remote_endpoint().address();
            auto port = socket.remote_endpoint().port();

            for (;;)
            {
                INFO("Starting tcp server session for client connecting from {}:{}", ip.to_string(), port);
                std::size_t n = co_await socket.async_read_some(boost::asio::buffer(read_buf), use_awaitable);
                INFO("Server received {} bytes", n);
                bool send_response = read_write_op(read_buf, write_buf, ip.to_string(), (std::uint16_t) port);
                if (send_response)
                {
                    co_await async_write(socket, boost::asio::buffer(write_buf), use_awaitable);
                    INFO("Server responded with {} bytes", write_buf.size());
                }
            }
        }
        catch (boost::system::system_error& e)
        {
            auto ip = socket.remote_endpoint().address();

            if (e.code() == boost::asio::error::eof)
            {
                INFO("Client disconnecting from {}", ip.to_string())
            }
            else
            {
                // TODO: fix exceptions
                throw std::runtime_error("sever busted XD");
            }
               
        }
    }

    awaitable<void> tcp_client_session(tcp::socket socket, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op)
    {
        try
        {
            auto ip = socket.remote_endpoint().address();
            auto port = socket.remote_endpoint().port();
            INFO("Tcp client open to {}:{}", ip.to_string(), port);
            read_write_op(read_buf, write_buf, ip.to_string(), (std::uint16_t) port);
            co_await async_write(socket, boost::asio::buffer(write_buf), use_awaitable);
        }
        catch (boost::system::system_error& e)
        {
            auto ip = socket.remote_endpoint().address();

            if (e.code() == boost::asio::error::eof)
            {
                INFO("Client disconnecting from {}", ip.to_string())
            }
            else
            {
                // TODO: fix exceptions
                throw std::runtime_error("sever busted XD");
            }
        }
    }

    awaitable<void> accept_tcp_connections(const uint16_t port, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op)
    {
        auto executor = co_await this_coro::executor;
        tcp::acceptor acceptor(executor, {tcp::v4(), port});
        INFO("Accepting tcp connections from port {}", port);
        for (;;)
        {
            tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
            auto ip = socket.remote_endpoint().address();

            socket_base::keep_alive option(true);
            socket.set_option(option);

            INFO("Client connecting from {}", ip.to_string(), port);
            co_spawn(executor, tcp_server_session(std::move(socket), read_buf, write_buf, read_write_op), detached);
        }
    }

    awaitable<void> connect_to_tcp_server(const std::string_view host, const uint16_t port, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op)
    {
        auto executor = co_await this_coro::executor;
        tcp::socket socket(executor);
        tcp::endpoint endpoint(address::from_string(std::string(host)), port);

        auto timeout_timer = [](deadline_timer& timer, tcp::socket& socket, const std::string_view host, const uint16_t port) -> awaitable<void>
        {
            co_await timer.async_wait(use_awaitable);
            socket.cancel();
            socket.close();
            INFO("Tcp client timed out while trying to connect to {}:{}", host, port);
        };

        deadline_timer timer(executor);
        timer.expires_from_now(boost::posix_time::seconds(1));
        co_spawn(executor, timeout_timer(timer, socket, host, port), detached);

        INFO("Connecting to server {}:{}", host, port);

        co_await socket.async_connect(endpoint, use_awaitable);
        timer.cancel();

        co_await tcp_client_session(std::move(socket), read_buf, write_buf, read_write_op);
    }
}
