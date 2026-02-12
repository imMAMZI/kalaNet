#ifndef COMMAND_UTILS_H
#define COMMAND_UTILS_H

#include <QString>
#include "protocol/commands.h"

namespace common {

    QString commandToString(Command cmd);
    Command stringToCommand(const QString& str);

}

#endif // COMMAND_UTILS_H
