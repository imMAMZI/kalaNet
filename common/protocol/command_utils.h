#ifndef COMMON_PROTOCOL_COMMAND_UTILS_H
#define COMMON_PROTOCOL_COMMAND_UTILS_H

#include <QString>
#include "protocol/commands.h"

namespace common {

    QString commandToString(Command command);
    Command commandFromString(const QString& commandString);

}

#endif // COMMON_PROTOCOL_COMMAND_UTILS_H
