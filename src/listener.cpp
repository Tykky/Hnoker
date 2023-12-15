#include "listener.hpp"
#include "logging.hpp"
#include "functional"
#include "message_types.hpp"
#include "networking.hpp"
#include "player.hpp"

#include <functional>
#include <array>
#include <string_view>
#include <cstdint>
#include <span>

namespace hnoker
{
    static bool is_this_coordinator(ClientList& cl)
    {
        std::uint16_t my_id = cl.bully_id;
        Client coordinator;
        coordinator.bully_id = 0;
        for (auto& c : cl.clients)
        {
            if (c.bully_id > coordinator.bully_id)
                coordinator = c;
        }
        return coordinator.bully_id == my_id;
    }
    
    static void relay_cm_to_all_clients(ControlMusic& cm, ClientList& cl)
    {
        network net;
        Message msg { MessageType::CONTROL_MUSIC };

        std::array<char, 1024> rbuf;
        std::array<char, 1024> wbuf;

        std::function w = [&](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
        {
            write_message_to_buffer(write_buf, msg);
            return false;
        };

        std::function th = []()
        {
            INFO("Failed to relay CONTROL_MUSIC to listener");
        };

        for (auto& c : cl.clients)
        { 
            if (!is_this_coordinator(cl))
            {
                net.async_connect_server(c.ip, LISTENER_SERVER_PORT, rbuf, wbuf, w, th);
                net.run();
            }
        }
    }

    static void send_player_status(std::string_view ip, std::span<char> rbuf, std::span<char> wbuf, player::MusicPlayer& player)
    {
        INFO("Listener recieved coordinator relay");
        network net;
        
        std::function write = [&](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
        {
            Message msg{ MessageType::SEND_STATUS };
            msg.ss = player.get_status();
            write_message_to_buffer(wbuf, msg);
            return false;
        };

        hnoker::timeout_handler th = []()
        {
            INFO("Listener tried to respond to coordinator relay but timed out!");
        };

        net.async_connect_server(ip, LISTENER_SERVER_PORT, rbuf, wbuf, write, th);
        net.run();

    }

    static bool cm_handler(ControlMusic& cm, player::MusicPlayer& player, ClientList& cl, std::string_view ip, std::span<char> rbuf, std::span<char> wbuf)
    {
        INFO("Listener recieved CONTROL_MUSIC");
        SendStatus status = player.get_status();
        switch (cm.op)
        {
            case ControlOperation::START:
                if (status.paused)
                    player.toggle_pause();
                break;
            case ControlOperation::STOP:
                if (!status.paused)
                    player.toggle_pause();
                break;
            case ControlOperation::SKIP:
                player.skip();
                break;
        }

        if (is_this_coordinator(cl))
            relay_cm_to_all_clients(cm, cl);
        else
            send_player_status(ip, rbuf, wbuf, player);

        return false;
    }

    static bool cs_handler(ChangeSong& cs, player::MusicPlayer& player)
    {
        INFO("Listener recieved CHANGE_SONG");
        if (cs.add_to_queue)
            player.add_to_queue(cs.song_id);
        return false;
    }

    static bool dc_handler(Disconnect& dc, player::MusicPlayer& player)
    {
        INFO("Listener recieved DISCONNECT");
        return false;
    }

    static bool cn_handler(Connect& cn)
    {
        INFO("Listener recieved CONNECT");
        return false;
    }

    static bool qs_handler(QueryStatus& qs, player::MusicPlayer& player,std::span<char> rbuf, std::span<char> wbuf, const std::string_view ip, const std::uint16_t port)
    {
        INFO("Listener recieved QUERY_STATUS");
        network net;
        
        std::function write = [&](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
        {
            Message msg{ MessageType::SEND_STATUS };
            msg.ss = player.get_status();
            write_message_to_buffer(wbuf, msg);
            return false;
        };

        hnoker::timeout_handler th = []()
        {
            INFO("Listener tried to respond to QUERY_STATUS but timed out!");
        };

        net.async_connect_server(ip, port, rbuf, wbuf, write, th);
        net.run();

        return false;
    }

    static bool ss_handler(SendStatus& ss, player::MusicPlayer& player, ClientList& listener_state, std::string_view ip, std::span<char> rbuf, std::span<char> wbuf)
    {
        INFO("Listener recieved SEND_STATUS, checking for desync");
        const SendStatus& ps = player.get_status();

        if (is_this_coordinator(listener_state))
        {
            if (ps != ss)
            {
                INFO("Coordinator detected desync! sending state");
                for (auto& c : listener_state.clients)
                {
                    if (listener_state.bully_id != c.bully_id)
                        send_player_status(c.ip, rbuf, wbuf, player);
                }
            }
        }
        else
        {
            player.set_status(ps);
        }

        return false;
    }

    static bool bl_handler(Bully& bl)
    {
        INFO("Listenere recieved BULLY");
        return false;
    }

    static bool cl_handler(ClientList& cl, ClientList& listener_state)
    {
        INFO("Listener recieved CONNECTOR_LIST, bully_id : {}", cl.bully_id);
        listener_state.bully_id = cl.bully_id;
        return false;
    }

    static bool clu_handler(ClientListUpdate& clu, ClientList& listener_state)
    {
        INFO("Listener recieved CONNECTOR_LIST_UPDATE");
        listener_state.clients = clu.clients;
        return false;
    }

    static bool message_handler(Message& msg, player::MusicPlayer& player, std::span<char> rbuf, std::span<char> wbuf, ClientList& listener_state, const std::string_view ip, std::uint16_t port)
    {
        switch (msg.type)
        {
            case MessageType::CONTROL_MUSIC:
                return cm_handler(msg.cm, player, listener_state, ip, rbuf, wbuf);
            case MessageType::CHANGE_SONG:
                return cs_handler(msg.cs, player);
            case MessageType::DISCONNECT:
                return dc_handler(msg.dc, player);
            case MessageType::CONNECT:
                return cn_handler(msg.cn);
            case MessageType::QUERY_STATUS:
                return qs_handler(msg.qs, player, rbuf, wbuf, ip, port);
            case MessageType::SEND_STATUS:
                return ss_handler(msg.ss, player, listener_state, ip, rbuf, wbuf);
            case MessageType::BULLY:
                return bl_handler(msg.bl);
            case MessageType::CONNECTOR_LIST:
                return cl_handler(msg.cl, listener_state);
            case MessageType::CONNECTOR_LIST_UPDATE:
                return clu_handler(msg.cu, listener_state);
        }
        return false;
    }

    void start_listener(const std::string_view connector_ip, const uint16_t connector_port)
    {
        INFO("Starting listener");

        player::MusicPlayer player(1);
        ClientList listener_state_cl;

        std::array<char, 1024> client_rb;
        std::array<char, 1024> client_wb;
        std::array<char, 1024> server_rb;
        std::array<char, 1024> server_wb;

        network listener_server_network;

        Message connect_msg = { MessageType::CONNECT };

        static std::function server = [connector_ip, connector_port, &player, &listener_state_cl](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
        {
            Message msg = read_message_from_buffer(read_buf);
            return message_handler(msg, player,read_buf, write_buf, listener_state_cl, connector_ip, connector_port);
        };

        static std::function send_connect = [&connect_msg](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
        {
            write_message_to_buffer(write_buf, connect_msg);
            return true;
        };

        const timeout_handler thandler = [&]()
        {
            INFO("Failed to send CONNECT to connect node at {}:{}! Connection timed out.", connector_ip, connector_port);
        };

        listener_server_network.async_create_server(LISTENER_SERVER_PORT, server_rb, server_wb, server, [](){});
        listener_server_network.async_connect_server(connector_ip, connector_port, client_rb, client_wb, send_connect, thandler);

        std::jthread th { [&]() {
            listener_server_network.run();
        }};

        th.detach();
           
        player.start_player(listener_state_cl);

    }

  }
