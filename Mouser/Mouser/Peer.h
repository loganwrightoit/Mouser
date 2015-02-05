#pragma once

#include "WinSock2.h"
#include <map>

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
        void sendName();
        SOCKET getSocket() const;

        HWND getRoot();
        wchar_t* getName();
        
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
        void getChatIsTyping(Packet* pkt);
        void getName(Packet* pkt);

        std::pair<char*, size_t> encode_utf8(wchar_t* wstr);
        wchar_t* encode_utf16(char* str);

        wchar_t* getUserName();

        wchar_t* _name;
        SOCKET _socket;
        HWND _hWnd;
        HWND _hWnd_stream;
        POINT _cursor;

};