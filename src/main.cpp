#include "networking.hpp"
#include "logging.hpp"
#include <vector>
#include <functional>
#include <string>

int main()
{
    hnoker::network neta;
    hnoker::network netb;

    std::vector<char> rbuf(1024);
    std::vector<char> wbuf(1024);

    char* rbuf_data = rbuf.data();
    char* wbuf_data = wbuf.data();

    std::size_t rbuf_data_size = rbuf.size();
    std::size_t wbuf_data_size = wbuf.size();

    std::function read_write_op = [i = 0](char*& read_buf, std::size_t& read_buf_size, char*& write_buf, std::size_t& write_buf_size) mutable
    {
        INFO("read data: {}", read_buf);
        std::string muted = "ok muted XD " + std::to_string(++i);
        memcpy(write_buf, muted.data(), muted.size());
        write_buf_size = muted.size();
    };

   const auto port = 55555;

    neta.async_create_server(port, rbuf_data, rbuf_data_size, wbuf_data, wbuf_data_size, read_write_op);
    netb.async_connect_server("127.0.0.1", port, rbuf_data, rbuf_data_size, wbuf_data, wbuf_data_size, read_write_op);

    std::jthread th {};

    auto id = th.get_id();

    neta.join();
    netb.join();

}