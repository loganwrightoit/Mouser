#include "stdafx.h"
#include "Packet.h"

Packet::Packet(Protocol protocol, char * data, unsigned int size)
: _protocol(protocol), _data(data), _size(size) {}

Packet::~Packet()
{
    if (_data)
    {
        delete[] _data;
    }
}

Packet::Protocol Packet::getProtocol() const
{
    return _protocol;
}

char * Packet::getData() const
{
    return _data;
}

unsigned int Packet::getSize() const
{
    return _size;
}