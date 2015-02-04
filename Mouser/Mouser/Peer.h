#pragma once

#include "WinSock2.h"

static const int MAX_CHAT_LENGTH = 512;
class Packet;

class Peer
{

    public:

        Peer(SOCKET sock);
        ~Peer();

        void sendStreamImage();
        void sendStreamCursor();
        void sendChatMsg(wchar_t* msg);
        SOCKET getSocket() const;

        HWND getRoot();
        wchar_t* getIdentifier();


        void setInputFocus();
        void setChatWindow(HWND hWnd);
        void openChatWindow();
        void openStreamWindow();

        void clearHwnd();

    private:

        Peer(const Peer&); //not implemented anywhere
        void operator=(const Peer&); //not implemented anywhere
        
        void AddChat(LPWSTR msg);

        void rcvThread();

        void getStreamOpen(Packet* pkt);
        void getStreamClose(Packet* pkt);
        void getStreamImage(Packet* pkt);
        void getStreamCursor(Packet* pkt);
        void getChatText(Packet* pkt);

        SOCKET _socket;
        HWND _hWnd;
        HWND _hWnd_stream;
        POINT _cursor;

};

