#include "logging.hpp"
#include "message_types.hpp"
#include "networking.hpp"
#include "player.hpp"

#include <chrono>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include <vector>

void test_archival()
{
    {
        std::ofstream test_output("test_filename420.virus");
        const ControlMusic cm{.op = ControlOperation::SKIP};
        boost::archive::text_oarchive oa(test_output);
        oa << cm;
    }
    {
        std::ifstream test_input("test_filename420.virus");
        boost::archive::text_iarchive ia(test_input);
        ControlMusic read_cm;
        ia >> read_cm;
        std::cout << "op: " << read_cm.op << "\n";
    }
}

void test_networking()
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

void test_player()
{
    player::MusicPlayer player(2);

    std::this_thread::sleep_for(std::chrono::seconds(5));
    player.toggle_pause();
    //std::cout << "\nPaused\n";

    std::this_thread::sleep_for(std::chrono::seconds(5));
    player.toggle_pause();
    //std::cout << "Unpaused\n\n\n";

    std::this_thread::sleep_for(std::chrono::seconds(20));
    player.add_to_queue(1);
    //std::cout << "\nAdded new song to queue\n\n\n";

    std::this_thread::sleep_for(std::chrono::seconds(10));
    player.set_elapsed(3);
    //std::cout << "\nSet timer to 3 seconds\n\n\n";

    std::this_thread::sleep_for(std::chrono::seconds(5));
    player.stopper.request_stop();
}

int main()
{

}
