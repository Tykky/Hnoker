#pragma once

#include <cstdint>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

enum struct ControlOperation : std::uint8_t {
    START = 1,
    STOP = 2,
    SKIP = 3,
};

std::ostream& operator<< (std::ostream& stream, const ControlOperation& op) {
    switch (op)
    {
        case ControlOperation::START:
            stream << "START";
            return stream;
        case ControlOperation::STOP:
            stream << "STOP";
            return stream;
        case ControlOperation::SKIP:
            stream << "SKIP";
            return stream;
        default:
            return stream;
    }
}

struct ControlMusic {
    char type[2] = {'C', 'M'};
    std::uint16_t size = sizeof(ControlMusic);
    ControlOperation op;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & type;
        ar & size;
        ar & op;
    }
};
