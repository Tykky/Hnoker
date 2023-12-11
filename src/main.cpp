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

    std::function write_archive = [](std::span<char> rbuf, std::span<char> wbuf, const std::string& ip, std::uint16_t port) mutable -> bool 
    {
        INFO("Writing one-byte header and ChangeSong struct to wbuf");
        Message cs_out { MessageType::CHANGE_SONG };
        cs_out.cs.song_id = 1;
        hnoker::write_message_to_buffer(wbuf, cs_out);
        return true;
    };

    std::function read_write_op = [&player](std::span<char> rbuf, std::span<char> wbuf, const std::string& ip, std::uint16_t port) -> bool
    {
        INFO("Header not used yet but its value is: {}", +rbuf[0]);

        INFO("Attempting deserialization...");
        std::span archive{rbuf.begin(), rbuf.end()};
        Message cs_in = hnoker::read_message_from_buffer(archive);
        INFO("song_id: {}", cs_in.cs.song_id);

        player.add_to_queue(cs_in.cs.song_id);
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
    test_networking_archives_singlemessage();
}
