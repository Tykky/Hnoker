#pragma once

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/string.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <variant>
#include <vector>

#define MAXIMUM_QUEUE_SIZE 128
#define MAXIMUM_CLIENTS 64
#define IP_LENGTH 16

enum struct MessageType : std::uint8_t {
    CONTROL_MUSIC = 0,
    CHANGE_SONG = 1,
    DISCONNECT = 2,
    CONNECT = 3,
    QUERY_STATUS = 4,
    SEND_STATUS = 5,
    BULLY = 6,
    CONNECTOR_LIST = 7,
    CONNECTOR_LIST_UPDATE = 8,
};

enum struct ControlOperation : std::uint8_t {
    START = 1,
    STOP = 2,
    SKIP = 3,
};

std::ostream& operator<< (std::ostream& stream, const ControlOperation& op);

// CM
struct ControlMusic {
    ControlOperation op;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & op;
    }
};

// CS
struct ChangeSong {
    int song_id;
    bool add_to_queue = true;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & song_id;
        ar & add_to_queue;
    }
};

// DC
struct Disconnect {
    std::uint8_t dummy{};

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & dummy;
    }
};

// CN
struct Connect {
    std::uint8_t dummy{};

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & dummy;
    }
};

// QS
struct QueryStatus {
    std::uint8_t dummy{};

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & dummy;
    }
};

// SS
struct SendStatus {
    int current_song_id;
    int elapsed_time;
    bool paused;
    std::deque<int> queue;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & current_song_id;
        ar & elapsed_time;
        ar & paused;
        ar & queue;
    }
};

enum struct BullyType : std::uint8_t {
    ELECTION = 1,
    ANSWER = 2,
    VICTORY = 3,
};

// BL
struct Bully {
    BullyType et;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & et;
    }
};

struct Client {
    std::string ip;
    std::uint16_t port;
    std::uint16_t bully_id;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & ip;
        ar & port;
        ar & bully_id;
    }

    bool operator==(const Client& rhs)
    {
        return ip == rhs.ip && port == rhs.port;
    }
};

struct ClientList {
    std::uint16_t bully_id;
    std::vector<Client> clients;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & bully_id;
        ar & clients;
    }
};

struct ClientListUpdate {
    std::vector<Client> clients;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & clients;
    }
};

struct Message
{
    Message(MessageType t) : 
        type(t)
    {
        switch (type)
        {
            case MessageType::CONTROL_MUSIC:
                new (&cm) ControlMusic;
                break;
            case MessageType::CHANGE_SONG:
                new (&cs) ChangeSong;
                break;
            case MessageType::DISCONNECT:
                new (&dc) Disconnect;
                break;
            case MessageType::CONNECT:
                new (&cn) Connect;
                break;
            case MessageType::QUERY_STATUS:
                new (&qs) QueryStatus;
                break;
            case MessageType::SEND_STATUS:
                new (&ss) SendStatus;
                break;
            case MessageType::BULLY:
                new (&bl) Bully;
                break;
            case MessageType::CONNECTOR_LIST:
                new (&cl) ClientList;
                break;
            case MessageType::CONNECTOR_LIST_UPDATE:
                new (&cu) ClientListUpdate;
                break;
        }
    }

    ~Message()
    {
        switch (type)
        {
            case MessageType::CONTROL_MUSIC:
                cm.~ControlMusic();
                break;
            case MessageType::CHANGE_SONG:
                cs.~ChangeSong();
                break;
            case MessageType::DISCONNECT:
                dc.~Disconnect();
                break;
            case MessageType::CONNECT:
                cn.~Connect();
                break;
            case MessageType::QUERY_STATUS:
                qs.~QueryStatus();
                break;
            case MessageType::SEND_STATUS:
                ss.~SendStatus();
                break;
            case MessageType::BULLY:
                bl.~Bully();
                break;
            case MessageType::CONNECTOR_LIST:
                cl.~ClientList();
                break;
            case MessageType::CONNECTOR_LIST_UPDATE:
                cu.~ClientListUpdate();
                break;
        }
        type.~MessageType();
    }

    Message(const Message& other) {
        type = other.type;
        switch(type)
        {
            case MessageType::CONTROL_MUSIC:
                cm = other.cm;
                break;
            case MessageType::CHANGE_SONG:
                cs = other.cs;
                break;
            case MessageType::DISCONNECT:
                dc = other.dc;
                break;
            case MessageType::CONNECT:
                cn = other.cn;
                break;
            case MessageType::QUERY_STATUS:
                qs = other.qs;
                break;
            case MessageType::SEND_STATUS:
                ss = other.ss;
                break;
            case MessageType::BULLY:
                bl = other.bl;
                break;
            case MessageType::CONNECTOR_LIST:
                cl = other.cl;
                break;
            case MessageType::CONNECTOR_LIST_UPDATE:
                cu = other.cu;
                break;
        }
    }

    Message(Message&& other) noexcept // Move constructor
    {
        type = std::move(other.type);
        switch(type)
        {
            case MessageType::CONTROL_MUSIC:
                cm = std::move(other.cm);
                break;
            case MessageType::CHANGE_SONG:
                cs = std::move(other.cs);
                break;
            case MessageType::DISCONNECT:
                dc = std::move(other.dc);
                break;
            case MessageType::CONNECT:
                cn = std::move(other.cn);
                break;
            case MessageType::QUERY_STATUS:
                qs = std::move(other.qs);
                break;
            case MessageType::SEND_STATUS:
                ss = std::move(other.ss);
                break;
            case MessageType::BULLY:
                bl = std::move(other.bl);
                break;
            case MessageType::CONNECTOR_LIST:
                cl = std::move(other.cl);
                break;
            case MessageType::CONNECTOR_LIST_UPDATE:
                cu = std::move(other.cu);
                break;
        }
    } 
    Message& operator=(const Message& other) //Copy assignment
    {
        return *this = Message(other);
    }
    Message& operator=(Message&& other) noexcept // Move assignment
    {
        return *this = Message(other);
    }

    MessageType type;

    union 
    {
        ControlMusic     cm;
        ChangeSong       cs;
        Disconnect       dc;
        Connect          cn;
        QueryStatus      qs;
        SendStatus       ss;
        Bully            bl;
        ClientList       cl;
        ClientListUpdate cu;
    };

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        switch (type)
        {
            case MessageType::CONTROL_MUSIC:
                cm.serialize(ar, version);
                break;
            case MessageType::CHANGE_SONG:
                cs.serialize(ar, version);
                break;
            case MessageType::DISCONNECT:
                dc.serialize(ar, version);
                break;
            case MessageType::CONNECT:
                cn.serialize(ar, version);
                break;
            case MessageType::QUERY_STATUS:
                qs.serialize(ar, version);
                break;
            case MessageType::SEND_STATUS:
                ss.serialize(ar, version);
                break;
            case MessageType::BULLY:
                bl.serialize(ar, version);
                break;
            case MessageType::CONNECTOR_LIST:
                cl.serialize(ar, version);
                break;
            case MessageType::CONNECTOR_LIST_UPDATE:
                cu.serialize(ar, version);
                break;
        }
    }
};
