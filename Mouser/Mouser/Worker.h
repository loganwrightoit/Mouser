#pragma once

#include <queue>

class Worker
{

    public:

        Worker(SOCKET socket);
        ~Worker();

        HWND getHandle() const;
        void sendPacket(Packet* pkt);

        bool isReady() const;

    private:

        static LRESULT CALLBACK WorkerProc(HWND, UINT, WPARAM, LPARAM);
        void run();
        void queuePacket(Packet* pkt);
        void sendThread();

        SOCKET _socket;
        HANDLE ghMutex;
        HWND _hwnd;
        std::queue<Packet*> _outPkts;

};

