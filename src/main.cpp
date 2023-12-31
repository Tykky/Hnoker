#include "connector.hpp"
#include "logging.hpp"
#include "message_types.hpp"
#include "networking.hpp"
#include "player.hpp"
#include "listener.hpp"

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <chrono>
#include <fstream>
#include <functional>
#include <span>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

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

    // Tää omaan threadiin, mul meni joku puol tuntii tajuta et toi run on blokkaava calli xd
    std::jthread xd([&]() mutable { network.async_create_server(LISTENER_SERVER_PORT, server_rbuf, server_wbuf, read_write_op, hnoker::default_timeout_handler); network.run(); });

    // Uuus viesti queueen joka kerta
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        INFO("Sending message");

        hnoker::network client_network;
        client_network.async_connect_server("127.0.0.1", LISTENER_SERVER_PORT, client_rbuf, client_wbuf, write_archive, hnoker::default_timeout_handler);
        //network.async_connect_server("127.0.0.1", port, client_rbuf, client_wbuf, write_archive);
        client_network.run();
    }
}

void test_connector()
{
    std::jthread xd([]() { start_connector(); });
    xd.detach();

    std::array<char, 1024> client_rbuf;
    std::array<char, 1024> client_wbuf;

    hnoker::read_write_op_t client_callback = [](std::span<char> rbuf, std::span<char> wbuf, const std::string& ip, std::uint16_t port) -> bool
    {
        INFO("Sending message in 3")
        std::this_thread::sleep_for(std::chrono::seconds(3));
        Message connect{ MessageType::CONNECT };
        hnoker::write_message_to_buffer(wbuf, connect);
        INFO("Sending message")
        return true;
    };

    hnoker::read_write_op_t server_callback = [](std::span<char> rbuf, std::span<char> wbuf, const std::string& ip, std::uint16_t port) -> bool
    {
        INFO("Trying to read message from buffer")
        Message message = hnoker::read_message_from_buffer(rbuf);
        INFO("First client ip: {}", message.cu.clients[0].ip);
        INFO("First client bully id: {}", message.cu.clients[0].bully_id);
        INFO("First client port: {}", message.cu.clients[0].port);
        return false;
    };

    hnoker::network server_network;

    std::jthread xd2blyat([&]() { server_network.async_create_server(LISTENER_SERVER_PORT, client_rbuf, client_wbuf, server_callback, hnoker::default_timeout_handler); server_network.run(); });
    xd2blyat.detach();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        INFO("Sending message");
        hnoker::network client_network;
        client_network.async_connect_server("127.0.0.1", CONNECTOR_SERVER_PORT, client_rbuf, client_wbuf, client_callback, hnoker::default_timeout_handler);
        client_network.run();
        INFO("Message sent")
    }
}

void test_listener()
{
    std::jthread xdc{ []() { start_connector();} };
    xdc.detach();
    std::string conn_ip = "127.0.0.1";
    hnoker::start_listener(conn_ip, CONNECTOR_SERVER_PORT);
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

using string_map = std::unordered_map<std::string, std::vector<std::string>>;
template <class Keyword>
string_map parse_cmd_arg(std::vector<std::string> as, Keyword keyword)
{
    string_map result;

    std::string flag;
    for (auto&& x : as)
    {
        auto f = keyword(x);
        if (f.empty())
        {
            result[flag].push_back(x);
        }
        else
        {
            flag = f.front();
            result[flag];
            flag = f.back();
        }
    }
    return result;
}

int main(int argc, const char* argv[])
{
    std::vector<std::string> args(argv + 1, argv + argc);
    std::unordered_map<std::string, std::string> keys = 
    {
        { "--mode", "mode" },
        { "--test", "test" }
    };

    auto arg_map = parse_cmd_arg(args, [&](auto&& s) -> std::vector<std::string> {
        if (keys.count(s) > 0)
            return { keys[s] };
        else
            return {};
    });

    std::string mode = arg_map.find("mode") != arg_map.end() ? arg_map["mode"].size() > 0 ? arg_map["mode"][0] : "" : "";
    std::string test = arg_map.find("test") != arg_map.end() ? arg_map["test"].size() > 0 ? arg_map["test"][0] : "" : "";

    if (mode == "listener")
    {
        const char* conn_ip = "127.0.0.1";
        hnoker::start_listener(conn_ip, CONNECTOR_SERVER_PORT);
    }
    else if (mode == "connector")
    {
        start_connector();
    }
    else if (test == "singlemessage")
    {
        test_networking_archives_singlemessage();
    }


}
