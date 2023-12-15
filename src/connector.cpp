#include "connector.hpp"
#include "logging.hpp"
#include "message_types.hpp"
#include "networking.hpp"

#include <algorithm>
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
static std::mutex clients_mutex;

class RNG {
private:
    std::random_device dev_{};
    std::mt19937 gen_{dev_()};
    std::uniform_int_distribution<std::uint16_t> dist_{1, std::numeric_limits<std::uint16_t>::max()};
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

bool operator==(const ClientInfo& lhs, const Client& rhs)
{
    return lhs.client.ip == rhs.ip;
}

std::vector<ClientInfo> clients;

std::jthread create_knocker(const std::string ip, const std::uint16_t port)
{
    return std::jthread{[=]()
        {
            std::array<char, 1024> read_buffer;
            std::array<char, 1024> write_buffer;

            std::span<char> read_span{read_buffer};
            std::span<char> write_span{write_buffer};

            Message qs_out{ MessageType::QUERY_STATUS };

            INFO("Knocker created for {}:{}", ip, LISTENER_SERVER_PORT);
            hnoker::read_write_op_t knocker_callback = [&qs_out](std::span<char> xd1, std::span<char> xd2, const std::string& ip, std::uint16_t port) -> bool
            {
                hnoker::write_message_to_buffer(xd2, qs_out);
                return true;
            };

            hnoker::network knocker_network;

            const hnoker::timeout_handler th = []() {
            };

            while (true)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                INFO("Knocking {}:{}", ip, LISTENER_SERVER_PORT);
                knocker_network.async_connect_server(ip, LISTENER_SERVER_PORT, read_span, write_span, knocker_callback, hnoker::default_timeout_handler);
                knocker_network.run();
            }
        }
    };
}

void remove_client(Client to_remove)
{
    INFO("Removing {}:{}", to_remove.ip, to_remove.port);
    const auto client_equals = [&](const ClientInfo& client) -> bool
    {
        return client == to_remove;
    };
    std::erase_if(clients, client_equals);
}

void refresh_timeout(Client to_refresh)
{
    for (ClientInfo& c : clients)
    {
        if (c == to_refresh)
        {
            c.timeout = clocker.now();
        }
    }
}

void send_list_to_all()
{
    INFO("Starting list send")
    std::array<char, 1024> write_buffer;
    std::span<char> write_span{write_buffer.begin(), write_buffer.end()};

    Message cu_out{ MessageType::CONNECTOR_LIST_UPDATE };
    std::vector<Client> client_list;
    INFO("Current clients: {}", clients.size());

    {
        // Copy these so we have a stable iterator
        std::lock_guard<std::mutex> clients_lock(clients_mutex);
        for (const ClientInfo& c : clients)
        {
            client_list.emplace_back(c.client);
        }
    }

    cu_out.cu.clients = client_list;

    INFO("Writing client list to buffer")
    hnoker::write_message_to_buffer(write_span, cu_out);
    INFO("Client list written to buffer");
    hnoker::read_write_op_t op = [](std::span<char> xd, std::span<char> xd2, const std::string& xd3, std::uint16_t xd4)
    {
        return true;
    };

    hnoker::network bombardment;
    for (const Client& c : client_list)
    {
        INFO("Client: {}", c.ip)
        bombardment.async_connect_server(c.ip, LISTENER_SERVER_PORT, std::span<char>(), write_buffer, op, hnoker::default_timeout_handler);
    }
    bombardment.run();
}

void start_connector()
{
    INFO("Starting connector")
    std::array<char, 1024> read_buffer;
    std::array<char, 1024> write_buffer;

    RNG rng{};
    clients = std::vector<ClientInfo>();

    hnoker::read_write_op_t handle_message = [&rng](std::span<char> read_buffer, std::span<char> write_buffer, const std::string& ip, const std::uint16_t& port) -> bool
    {
        INFO("Connector received message from {}:{}", ip, port)
        MessageType message_type = static_cast<MessageType>(read_buffer[0]);

        std::uint16_t bully_id = rng.get();
        Client client{ip, port, rng.get()};

        hnoker::network net;

        if (message_type == MessageType::CONNECT)
        {
            INFO("Message type was CONNECT")

            bool added = false;
            {
                std::lock_guard<std::mutex> clients_lock(clients_mutex);
                if (std::find(clients.begin(), clients.end(), client) == clients.end())
                {
                    clients.emplace_back(std::move(client), create_knocker(ip, port));
                    added = true;
                }
            }

            added = true;

            if (added)
            {
                INFO("New client {} was added to list, sending new list to all", ip)

                Message cl = { MessageType::CONNECTOR_LIST };
                cl.cl.bully_id = bully_id;

                std::function msg = [&cl](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
                {
                    hnoker::write_message_to_buffer(write_buf, cl);
                    return false;
                };

                hnoker::timeout_handler th = []()
                {
                    INFO("Timed out while sending CONNECTOR_LIST");
                };

                net.async_connect_server(ip, LISTENER_SERVER_PORT, read_buffer, write_buffer, msg, th);
                net.run();

                send_list_to_all();
            }
        }
        else if (message_type == MessageType::DISCONNECT)
        {
            INFO("Message type was DISCONNECT")
            {
                std::lock_guard<std::mutex> clients_lock(clients_mutex);
                remove_client(client);
            }
            send_list_to_all();
        }
        else if (message_type == MessageType::SEND_STATUS)
        {
            INFO("Message type was SEND_STATUS")
            refresh_timeout(client);
        }
        return false;
    };

    hnoker::network network;
    std::jthread xd{[&]() { network.async_create_server(CONNECTOR_SERVER_PORT, read_buffer, write_buffer, handle_message, hnoker::default_timeout_handler); INFO("Connector receiving messages"); network.run(); }};
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

        size_t number_erased;
        {
            std::lock_guard<std::mutex> clients_lock(clients_mutex);
            number_erased = std::erase_if(clients, past_timeout);
        }
        if (number_erased > 0)
        {
            INFO("Some timed out clients were removed, sending new list to remaining clients");
            send_list_to_all();
        }
    }
}
