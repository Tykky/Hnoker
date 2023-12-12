#include "connector.hpp"
#include "logging.hpp"
#include "message_types.hpp"
#include "networking.hpp"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <random>
#include <span>
#include <thread>
#include <utility>
#include <vector>

static std::chrono::steady_clock clocker;

class RNG {
private:
    std::random_device dev_{};
    std::mt19937 gen_{dev_()};
    std::uniform_int_distribution<std::uint16_t> dist_{0, std::numeric_limits<std::uint16_t>::max()};
public:
    std::uint16_t get()
    {
        return dist_(gen_);
    }
};

struct ClientInfo {
    Client client;
    std::jthread thread;
    std::chrono::time_point<std::chrono::steady_clock> timeout = clocker.now();

    bool operator==(const ClientInfo& other) const
    {
        return client.ip == other.client.ip
            && client.port == other.client.port;
    }
};

std::jthread create_knocker(const std::string ip, const std::uint16_t port)
{
    return std::jthread{[=]()
        {
            std::array<char, 1024> read_buffer;
            std::array<char, 1024> write_buffer;

            std::span<char> read_span{read_buffer};
            std::span<char> write_span{write_buffer};

            Message qs_out{ MessageType::QUERY_STATUS };

            hnoker::write_message_to_buffer(read_span, qs_out);

            INFO("Knocker created for {}:{}", ip, 55555);
            hnoker::read_write_op_t knocker_callback = [](std::span<char> xd1, std::span<char> xd2, const std::string& ip, std::uint16_t port) -> bool
            {
                return true;
            };

            while (true)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                hnoker::network knocker_network;
                INFO("Knocking {}:{}", ip, 55555);
                knocker_network.async_connect_server(ip, 55555, read_span, write_span, knocker_callback);
            }
        }
    };
}

void remove_client(Client to_remove, std::vector<ClientInfo>& clients)
{
    INFO("Removing {}:{}", to_remove.ip, to_remove.port);
    const auto client_equals = [&](const ClientInfo& client) -> bool
    {
        return (client.client.ip == to_remove.ip)
            && (client.client.port == to_remove.port);
    };

    std::erase_if(clients, client_equals);
}

void refresh_timeout(Client to_refresh, std::vector<ClientInfo>& clients)
{
    for (ClientInfo& c : clients)
    {
        if (c.client.ip == to_refresh.ip
         && c.client.port == to_refresh.port)
        {
            c.timeout = clocker.now();
        }
    }
}

void send_list_to_all(const std::vector<ClientInfo>& clients)
{
    INFO("Starting list send")
    std::array<char, 1024> write_buffer;
    std::span<char> write_span{write_buffer.begin(), write_buffer.end()};

    Message clientlist{ MessageType::CONNECTOR_LIST_UPDATE };
    INFO("Current clients: {}", clients.size());

    for (const ClientInfo& c : clients)
    {
        clientlist.cl.clients.emplace_back(c.client);
    }

    hnoker::write_message_to_buffer(write_span, clientlist);
    INFO("Client list written to buffer");
    hnoker::read_write_op_t op = [](std::span<char> xd, std::span<char> xd2, const std::string& xd3, std::uint16_t xd4)
    {
        return true;
    };


    for (const ClientInfo& c : clients)
    {
        INFO("Client: {}", c.client.ip)
        hnoker::network bombardment;
        std::jthread xd {[&](){bombardment.async_connect_server(c.client.ip, 55555, std::span<char>(), write_buffer, op); bombardment.run(); }};
        xd.detach();
        INFO("Detached thread")
    }
}

void start_connector()
{
    INFO("Starting connector")
    std::array<char, 1024> read_buffer;
    std::array<char, 1024> write_buffer;

    std::vector<ClientInfo> clients;

    RNG rng{};

    hnoker::read_write_op_t handle_message = [&clients, &rng](std::span<char> read_buffer, std::span<char> write_buffer, const std::string& ip, const std::uint16_t& port) -> bool
    {
        INFO("Connector received message from {}:{}", ip, port)
        MessageType message_type = static_cast<MessageType>(read_buffer[0]);

        std::uint16_t bully_id = rng.get();
        Client client{ip, port, rng.get()};

        if (message_type == MessageType::CONNECT)
        {
            INFO("Message type was CONNECT")
            clients.emplace_back(std::move(client), create_knocker(ip, port));
            //send_list_to_all(clients);
        }
        else if (message_type == MessageType::DISCONNECT)
        {
            INFO("Message type was DISCONNECT")
            remove_client(client, clients);
            //send_list_to_all(clients);
        }
        else if (message_type == MessageType::SEND_STATUS)
        {
            INFO("Message type was SEND_STATUS")
            refresh_timeout(client, clients);
        }
        return false;
    };

    hnoker::network network;
    std::jthread xd{[&]() { network.async_create_server(CONNECTOR_SERVER_PORT, read_buffer, write_buffer, handle_message); INFO("Connector receiving messages"); network.run(); }};
    xd.detach();


    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        INFO("Cleaning up timed out clients")
        auto now = clocker.now();
        const auto past_timeout = [&](const ClientInfo& c)
        {
            return (c.timeout + std::chrono::seconds(3)) < now;
        };
        std::erase_if(clients, past_timeout);
    }
}
