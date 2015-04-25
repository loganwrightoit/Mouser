#pragma once

#include "WinSock2.h"
#include "StreamSender.h"
#include "FileSender.h"
#include "CursorUtil.h"
#include "Worker.h"
#include <atlimage.h> // IStream_ and CImage functions
#include <map>
#include <queue>
#include <fstream>

static const int MAX_CHAT_LENGTH = 512;
static const int FILE_BUFFER = 1000000;

class Packet;

class Peer
{
    
    public:

        Peer(SOCKET sock);
        ~Peer();

        void makeFileSendRequest(wchar_t* path);
        void stopSharing();
        void makeStreamRequest(HWND hWnd);
        void sendStreamImage();
        void sendStreamResize();
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
        void openChatWindow();
        void openStreamWindow();

        void onDestroyRoot();
        void clearStreamSender();
        void clearCursorUtil();

        void DrawStreamImage(HDC hdc);
        void DrawStreamCursor(HDC hdc);

        void createMenu(HWND hWnd);
        void onShareMenuInit();
        HMENU getMenu();
        HMENU getShareMenu();
        void flushShareMenu();

        Worker* getWorker();

        HWND hChatEditBox;
        HWND hChatButton;
        HWND hChatListBox;
        HWND hChatStopSharingButton;
        HWND hChatIsTypingLabel;

        void openDownloadDialog();

        bool onResize;

    private:

        Peer(const Peer&); //not implemented anywhere
        void operator=(const Peer&); //not implemented anywhere
        
        void addChat(LPWSTR msg);

        void rcvThread();
        
        int  displayFileSendRequestMessageBox(const FileSender::FileInfo info);
        void getFileSendRequest(Packet* pkt);
        void getFileSendAllow(Packet* pkt);
        void getFileSendDeny(Packet* pkt);
        
        int  displayStreamRequestMessageBox(wchar_t* name);
        void getStreamRequest(Packet* pkt);
        void getStreamAllow(Packet* pkt);
        void getStreamDeny(Packet* pkt);
        void getStreamImage(Packet* pkt);
        void getStreamCursor(Packet* pkt);
        void getStreamClose(Packet* pkt);

        void getChatText(Packet* pkt);
        void getChatIsTyping(Packet* pkt);
        void getName(Packet* pkt);
        void getStreamClose();
       
        bool encode_utf8(std::pair<char*, size_t>* outPair, wchar_t* wstr);
        wchar_t* encode_utf16(char* str);
		
        wchar_t* _name;
        SOCKET _socket;
        StreamSender* _streamSender;
        FileSender* _fileSender;
        CursorUtil* _cursorUtil;
        
        CImage _cachedStreamImage;
        POINT _cachedStreamCursor;
        RECT updateRegion;

        HWND _hWnd;
        HWND _hWnd_stream;
        HWND _hWnd_stream_src;

        HMENU _menu;
        HMENU _menuShareScreen;

        FileSender::FileInfo _temp;

        Worker* _worker;

};