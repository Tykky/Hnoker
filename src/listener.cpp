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
    static bool cm_handler(ControlMusic& cm, player::MusicPlayer& player)
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

    static bool qs_handler(QueryStatus& qs, player::MusicPlayer& player, std::span<char> wbuf)
    {
        INFO("Listener recieved QUERY_STATUS");
        Message msg{ MessageType::SEND_STATUS };
        msg.ss = player.get_status();
        write_message_to_buffer(wbuf, msg);
        return false;
    }

    static bool ss_handler(SendStatus& ss)
    {
        INFO("Listener recieved SEND_STATUS");
        return false;
    }

    static bool bl_handler(Bully& bl)
    {
        INFO("Listenere recieved BULLY");
        return false;
    }

    static bool cl_handler(ClientList& cl, ClientList& listener_state)
    {
        INFO("Listener recieved CONNECTOR_LIST");
        listener_state = cl;
        return false;
    }

    static bool clu_handler(ClientListUpdate& clu, ClientList& listener_state)
    {
        INFO("Listener recieved CONNECTOR_LIST_UPDATE");
        listener_state.clients = clu.clients;
        return false;
    }

    static bool message_handler(Message& msg, player::MusicPlayer& player, std::span<char> wbuf, ClientList& listener_state)
    {
        switch (msg.type)
        {
            case MessageType::CONTROL_MUSIC:
                return cm_handler(msg.cm, player);
            case MessageType::CHANGE_SONG:
                return cs_handler(msg.cs, player);
            case MessageType::DISCONNECT:
                return dc_handler(msg.dc, player);
            case MessageType::CONNECT:
                return cn_handler(msg.cn);
            case MessageType::QUERY_STATUS:
                return qs_handler(msg.qs, player, wbuf);
            case MessageType::SEND_STATUS:
                return ss_handler(msg.ss);
            case MessageType::BULLY:
                return bl_handler(msg.bl);
            case MessageType::CONNECTOR_LIST:
                return cl_handler(msg.cl, listener_state);
            case MessageType::CONNECTOR_LIST_UPDATE:
                return clu_handler(msg.cu, listener_state);
        }
        return false;
    }

    static void listener_send_control(ControlMusic& cm, network& net)
    {
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

        static std::function server = [&player, &listener_state_cl](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
        {
            Message msg = read_message_from_buffer(read_buf);
            return message_handler(msg, player, write_buf, listener_state_cl);
        };

        static std::function send_connect = [&connect_msg](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
        {
            write_message_to_buffer(write_buf, connect_msg);
            return true;
        };

        const timeout_handler th = []()
        {
        };

        listener_server_network.async_create_server(LISTENER_SERVER_PORT, server_rb, server_wb, server, [](){});
        listener_server_network.async_connect_server(connector_ip, connector_port, client_rb, client_wb, send_connect, th);

        std::jthread th{ [&]() {
            listener_server_network.run();
        }};

        th.detach();
           
        player.start_player(listener_state_cl);

    }

  }
