
#include "metadata.h"

Metadata::Metadata() :
    pos(0), len(0), timestamp(0)
{

}

Metadata::Metadata(qint64 pos, qint64 len, qint64 timestamp) :
    pos(pos), len(len), timestamp(timestamp)
{

}

