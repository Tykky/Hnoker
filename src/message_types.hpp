#pragma once

#include <array>
#include <cstdint>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <span>
#include <variant>

#define MAXIMUM_QUEUE_SIZE 128
#define MAXIMUM_CLIENTS 64

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
    std::uint8_t elements;
    int queue[MAXIMUM_QUEUE_SIZE];

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & current_song_id;
        ar & elapsed_time;
        ar & paused;
        ar & elements;
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
    char ip[16];
    std::uint16_t port;
    std::uint16_t bully_id;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & ip;
        ar & port;
        ar & bully_id;
    }
};

struct ClientList {
    std::uint8_t num_clients;
    std::uint16_t bully_id;
    Client clients[MAXIMUM_CLIENTS];

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & num_clients;
        ar & bully_id;
        ar & clients;
    }
};

struct ClientListUpdate {
    std::uint8_t num_clients;
    Client clients[MAXIMUM_CLIENTS];

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & num_clients;
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

    const MessageType type;

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
