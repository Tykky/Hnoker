#include "logging.hpp"
#include "message_types.hpp"
#include "networking.hpp"
#include "player.hpp"

#include <chrono>
#include <fstream>
#include <functional>
#include <span>
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

    //neta.async_create_server(port, rbuf, wbuf, read_write_op);
    //neta.async_connect_server("127.0.0.1", port, rbuf, wbuf, read_write_op);

    neta.run();
}

void test_networking_archives()
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
        INFO("Attempting deserialization...");

        if (rbuf[0] != 0) {
            ChangeSong cs_in = hnoker::read_archive_from_buffer<ChangeSong>(rbuf);
            INFO("song_id: {}", cs_in.song_id);
        } else {
            INFO("First byte was null, skipping deserialization because this probably was the first piece of data");
        }
        ChangeSong cs_out {.song_id = ++i};
        hnoker::write_archive_to_buffer<ChangeSong>(wbuf, cs_out);
    };

    const auto port = 55555;

    //neta.async_create_server(port, rbuf, wbuf, read_write_op);
    //neta.async_connect_server("127.0.0.1", port, rbuf, wbuf, read_write_op);

    //neta.run();
}

void test_networking_archives_singlemessage()
{
    hnoker::network network;

    std::vector<char> server_rbuf(1024);
    std::vector<char> server_wbuf(1024);

    std::array<char, 1024> client_rbuf;
    std::array<char, 1024> client_wbuf;

    player::MusicPlayer player{1};
    player.add_to_queue(2);

    // Tää rw-funktion palautusarvo määrittää vastataanko wbuf:n arvolla takasin
    // Tässä testiversiossa serveri vaan vastaanottaa ja käyttää sitä arvoa playerin kontrolloimiseen,
    // ei kirjota mitään takas

    std::function write_archive = [](std::span<char> rbuf, std::span<char> wbuf) mutable -> bool 
    {
        INFO("Writing one-byte header and ChangeSong struct to wbuf");
        ChangeSong cs_out {.song_id = 1};
        wbuf[0] = 123;
        std::span<char> after_header{wbuf.begin() + 1, wbuf.end()};
        hnoker::write_archive_to_buffer(after_header, cs_out);
        return true;
    };

    std::function read_write_op = [&player](std::span<char> rbuf, std::span<char> wbuf) -> bool
    {
        INFO("Header not used yet but its value is: {}", +rbuf[0]);

        INFO("Attempting deserialization...");
        std::span archive{rbuf.begin() + 1, rbuf.end()};
        ChangeSong cs_in = hnoker::read_archive_from_buffer<ChangeSong>(archive);
        INFO("song_id: {}", cs_in.song_id);

        player.add_to_queue(cs_in.song_id);
        player.skip();

        return false;
    };

    const auto port = 55555;

    // Tää omaan threadiin, mul meni joku puol tuntii tajuta et toi run on blokkaava calli xd
    std::jthread xd([&]() mutable { network.async_create_server(port, server_rbuf, server_wbuf, read_write_op); network.run(); });

    // Uuus viesti queueen joka kerta
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        INFO("Sending message");
        
        hnoker::network client_network;
        client_network.async_connect_server("127.0.0.1", port, client_rbuf, client_wbuf, write_archive);
        //network.async_connect_server("127.0.0.1", port, client_rbuf, client_wbuf, write_archive);
        client_network.run();
    }
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
