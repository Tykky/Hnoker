#include "networking.hpp"
#include "logging.hpp"
#include <vector>
#include <functional>
#include <string>

int main()
{
    hnoker::network neta;

    std::vector<char> rbuf(1024);
    std::vector<char> wbuf(1024);

    char* rbuf_data = rbuf.data();
    char* wbuf_data = wbuf.data();

    std::size_t rbuf_data_size = rbuf.size();
    std::size_t wbuf_data_size = wbuf.size();

    std::function read_write_op = [i = 0](std::span<char> rbuf, std::span<char> wbuf) mutable
    {
        INFO("read data: {}", rbuf.data());
        std::string muted = "ok muted XD " + std::to_string(++i);
        memcpy(wbuf.data(), muted.data(), muted.size());
    };

   const auto port = 55555;

    neta.async_create_server(port, rbuf, wbuf, read_write_op);
    neta.async_connect_server("127.0.0.1", port, rbuf, wbuf, read_write_op);

    neta.run();

}