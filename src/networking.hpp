#pragma once

#include <memory>
#include <string_view>
#include <functional>
#include "logging.hpp"

namespace hnoker 
{
    struct network_context;

    using read_write_op_t = std::function<void(char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size)>;

    network_context* allocate_network_context();
    void deallocate_network_context(network_context* network_context);
    void run_impl(network_context* network_context);

    void async_create_server_impl(network_context* ctx, uint16_t port, char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size, const read_write_op_t& read_write_op);
    void async_connect_server_impl(network_context* ctx, std::string_view address, uint16_t port, char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size, const read_write_op_t& read_write_op);

    struct network
    {
        network() :
            ctx(allocate_network_context())
        {}
        
        ~network()
        {
            deallocate_network_context(ctx);
        }

        void async_create_server(uint16_t port, char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size, const read_write_op_t& read_write_op)
        {
            async_create_server_impl(ctx, port, read_buf, read_buf_size, write_buf, write_buf_size, read_write_op);
        }

        void async_connect_server(std::string_view address, uint16_t port, char* read_buf, std::size_t read_buf_size, char* write_buf, std::size_t write_buf_size, const read_write_op_t& read_write_op)
        {
            async_connect_server_impl(ctx, address, port, read_buf, read_buf_size, write_buf, write_buf_size, read_write_op);
        }

        void run()
        {
            run_impl(ctx);
        }

    private:
        network_context* ctx;
    };
}

