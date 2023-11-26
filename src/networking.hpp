#pragma once

#include <memory>
#include <string_view>
#include <functional>
#include <thread>
#include "logging.hpp"

namespace hnoker 
{
    struct network_context;

    using read_write_op_t = std::function<void(char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size)>;

    network_context* allocate_network_context();
    void deallocate_network_context(network_context* network_context);

    std::jthread async_create_server_impl(network_context* ctx, uint16_t port, char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size, const read_write_op_t& read_write_op);
    std::jthread async_connect_server_impl(network_context* ctx, std::string_view address, uint16_t port, char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size, const read_write_op_t& read_write_op);
    std::jthread async_connect_server_range_impl(network_context* ctx, std::vector<std::string>& addresses, uint16_t port, char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size, const read_write_op_t& read_write_op);

    struct network
    {
        network() :
            ctx(allocate_network_context())
        {}
        
        ~network()
        {
            deallocate_network_context(ctx);
        }

        void join()
        {
            thread_handle.join();
        }

        void async_create_server(uint16_t port, char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size, const read_write_op_t& read_write_op)
        {
            if (!thread_handle_exist)
            {
                thread_handle = async_create_server_impl(ctx, port, read_buf, read_buf_size, write_buf, write_buf_size, read_write_op);
                thread_handle_exist = true;
            }
            else
            {
                WARN("trying to create server when thread handle exist");
            }
        }

        void async_connect_server(std::string_view address, uint16_t port, char* read_buf, std::size_t read_buf_size, char* write_buf, std::size_t write_buf_size, const read_write_op_t& read_write_op)
        {
            if (!thread_handle_exist)
            {
                thread_handle = async_connect_server_impl(ctx, address, port, read_buf, read_buf_size, write_buf, write_buf_size, read_write_op);
                thread_handle_exist = true;
            }
            else
            {
                WARN("trying connect to server when thread handle exist")
            }
        }

        void async_connect_server_range(network_context* ctx, std::vector<std::string>& addresses, uint16_t port, char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size, const read_write_op_t& read_write_op)
        {
            if (!thread_handle_exist)
            {
                thread_handle = async_connect_server_range_impl(ctx, addresses, port, read_buf, read_buf_size, write_buf, write_buf_size, read_write_op);
                thread_handle_exist = true;
            }
            else
            {
                WARN("trying to connect to server range when thread handle exist")
            }
        }

    private:
        bool thread_handle_exist = false;
        std::jthread thread_handle;
        network_context* ctx;
    };
}

