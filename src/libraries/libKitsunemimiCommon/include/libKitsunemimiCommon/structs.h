#ifndef KITSUNEMIMI_STRUCTS_H
#define KITSUNEMIMI_STRUCTS_H

#include <stdint.h>

#define UNINTI_POINT_32 0x0FFFFFFF

namespace Kitsunemimi
{

struct Position
{
    uint32_t x = UNINTI_POINT_32;
    uint32_t y = UNINTI_POINT_32;
    uint32_t z = UNINTI_POINT_32;
    uint32_t w = UNINTI_POINT_32;

    Position() {}

    Position(const Position &other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
    }

    Position& operator=(const Position &other)
    {
        if(this != &other)
        {
            x = other.x;
            y = other.y;
            z = other.z;
        }

        return *this;
    }

    bool operator==(const Position &other)
    {
        return(this->x == other.x
               && this->y == other.y
               && this->z == other.z);
    }

    bool isValid() const
    {
        return(x != UNINTI_POINT_32
               && y != UNINTI_POINT_32
               && z != UNINTI_POINT_32);
    }
};

}

#endif // KITSUNEMIMI_STRUCTS_H
