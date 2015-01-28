#pragma once

#include "WinSock2.h"

class Packet;

class Peer
{

    public:

        Peer(SOCKET sock);
        ~Peer();

        void sendStreamImage();
        void sendStreamCursor();
        SOCKET getSocket() const;

        void openChatWindow();
        void openStreamWindow();

    private:

        Peer(const Peer&); //not implemented anywhere
        void operator=(const Peer&); //not implemented anywhere
        
        void AddChat(LPWSTR msg);

        void rcvThread();

        void getStreamOpen(Packet * pkt);
        void getStreamClose(Packet * pkt);
        void getStreamImage(Packet * pkt);
        void getStreamCursor(Packet * pkt);

        SOCKET _socket;
        HWND _hWnd;
        HWND _hWnd_stream;
        POINT _cursor;

};

