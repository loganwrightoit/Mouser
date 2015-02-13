#pragma once

#include "WinSock2.h"
#include "StreamSender.h"
#include "CursorUtil.h"
#include <atlimage.h> // IStream_ and CImage functions
#include <map>
#include <queue>

static const int MAX_CHAT_LENGTH = 512;
class Packet;

class Peer
{
    
    public:

        Peer(SOCKET sock);
        ~Peer();

        void queuePacket(Packet* pkt);
        void sendPacket(Packet* pkt);

        size_t getQueueSize() const;

        void sendStreamImage();
        void sendStreamCursor();
        void sendChatMsg(wchar_t* msg);
        void sendName();
        SOCKET getSocket() const;

        HWND getRoot();
        HWND getStream();

        wchar_t* getName();
        
        void setInputFocus();
        void setChatWindow(HWND hWnd);
        void setStreamWindow(HWND hWnd);
        void streamTo();
        void openChatWindow();
        void openStreamWindow();

        void onDestroyRoot();

        void DrawStreamImage(HDC hdc, RECT rect);
        void DrawStreamCursor(HDC hdc, RECT rect);

        HWND hChatEditBox;
        HWND hChatButton;
        HWND hChatListBox;
        HWND hChatStreamButton;
        HWND hChatIsTypingLabel;

    private:

        Peer(const Peer&); //not implemented anywhere
        void operator=(const Peer&); //not implemented anywhere
        
        void AddChat(LPWSTR msg);

        void sendThread();
        void rcvThread();

        void getStreamInfo(Packet* pkt);
        void getStreamClose(Packet* pkt);
        void getStreamImage(Packet* pkt);
        void getStreamCursor(Packet* pkt);
        void getChatText(Packet* pkt);
        void getChatIsTyping(Packet* pkt);
        void getName(Packet* pkt);
        void getStreamClose();

        bool encode_utf8(std::pair<char*, size_t>* outPair, wchar_t* wstr);
        wchar_t* encode_utf16(char* str);

        wchar_t* getUserName();

        wchar_t* _name;
        SOCKET _socket;
        StreamSender* _streamSender;
        CursorUtil* _cursorUtil;
        std::queue<Packet*> sendQueue;
        bool _streaming;

        CImage _cachedStreamImage;
        POINT _cachedStreamCursor;

        HANDLE ghMutex;

        HWND _hWnd;
        HWND _hWnd_stream;

};