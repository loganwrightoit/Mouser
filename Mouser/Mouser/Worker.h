#pragma once

#include <queue>

class Worker
{

    public:

        Worker(SOCKET socket);
        ~Worker();

        HWND getHandle() const;
        void sendPacket(Packet* pkt);

        HANDLE getReadyEvent() const;
        void setReady();
        bool ready() const;

        void clearFileSender(void* peer);
        void clearStreamSender(void* peer);

    private:

        static LRESULT CALLBACK WorkerProc(HWND, UINT, WPARAM, LPARAM);
        void run();
        void queuePacket(Packet* pkt);
        void sendThread();
        
        HANDLE _ghReadyEvent;
        SOCKET _socket;
        HANDLE ghMutex;
        HWND _hwnd;
        bool isReady;
        std::queue<Packet*> _outPkts;

};

