#include "message_types.hpp"

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
