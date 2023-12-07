#pragma once

#include "logging.hpp"
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <strstream>

namespace hnoker 
{
    struct network_context;

    using read_write_op_t = std::function<void(std::span<char> read_buf, std::span<char> write_buf)>;

    network_context* allocate_network_context();
    void deallocate_network_context(network_context* network_context);
    void run_impl(network_context* network_context);

    void async_create_server_impl(network_context* ctx, uint16_t port, std::span<char> read_buf, std::span<char> write_buf,  const read_write_op_t& read_write_op);
    void async_connect_server_impl(network_context* ctx, std::string_view address, uint16_t port, std::span<char> read_buf, std::span<char> write_buf,const read_write_op_t& read_write_op);

    template<class Serializable>
    void write_archive_to_buffer(std::span<char>& buffer, const Serializable& s)
    {
        std::ostrstream output_stream(buffer.data(), (int) buffer.size());
        boost::archive::text_oarchive oa{output_stream};
        oa << s;
    }

    template<class Serializable>
    Serializable read_archive_from_buffer(const std::span<char>& buffer)
    {
        std::istrstream input_stream(buffer.data(), (int) buffer.size());
        boost::archive::text_iarchive ia{input_stream};
        Serializable s;
        ia >> s;
        return s;
    }

    struct network
    {
        network() :
            ctx(allocate_network_context())
        {}
        
        ~network()
        {
            deallocate_network_context(ctx);
        }

        void async_create_server(uint16_t port, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op)
        {
            async_create_server_impl(ctx, port, read_buf, write_buf, read_write_op);
        }

        void async_connect_server(std::string_view address, uint16_t port, std::span<char> read_buf, std::span<char> write_buf, const read_write_op_t& read_write_op)
        {
            async_connect_server_impl(ctx, address, port, read_buf, write_buf, read_write_op);
        }

        void run()
        {
            run_impl(ctx);
        }

    private:
        network_context* ctx;
    };
}

