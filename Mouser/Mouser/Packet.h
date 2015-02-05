#pragma once

class Peer;

class Packet
{

    public:

        enum Protocol
        {
            DISCONNECT,
            STREAM_BEGIN,
            STREAM_END,
            STREAM_IMAGE,
            STREAM_CURSOR,
            CHAT_TEXT,
            CHAT_IS_TYPING
        };

        /// <summary>
        ///     <para>
        ///         Only pass data stored on heap when creating packet,
        ///         otherwise deallocation error will occur.
        ///     </para>
        ///     <para>
        ///         This should save on memory and needless try / catch statements.
        ///     </para>
        /// </summary>
        Packet(Protocol protocol, char * data = nullptr, unsigned int size = 0);
        ~Packet();

        Protocol getProtocol() const;
        char * getData() const;
        unsigned int getSize() const;

    private:
        
        char * _data;
        unsigned int _size;
        Protocol _protocol;

};

