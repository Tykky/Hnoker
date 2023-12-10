#pragma once

#include <array>
#include <cstdint>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <span>

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
    CONNECTOR_LIST = 7
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
    std::array<int, MAXIMUM_QUEUE_SIZE> queue;

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
};

struct ClientList {
    std::uint8_t num_clients;
    Client clients[MAXIMUM_CLIENTS];
};
