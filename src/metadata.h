
#ifndef METADATA_H
#define METADATA_H

#include "qglobal.h"
class Metadata
{
public:
    Metadata(qint64 pos, qint64 len, qint64 timestamp);
    qint64 pos = 0;
    qint64 len = 0;
    qint64 timestamp = 0;
    // WebSocket text/binary
    // source clicnt for the TCP/BT server
};

#endif // METADATA_H
