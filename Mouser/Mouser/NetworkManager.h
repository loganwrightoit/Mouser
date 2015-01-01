#pragma once

#include "WinSock2.h"
#pragma comment (lib, "Ws2_32.lib")

class NetworkManager
{

    public:

        static NetworkManager& GetInstance()
        {
            static NetworkManager instance;
            return instance;
        }

        bool Send(SOCKET sock, char * inBytes, u_int inSize);
        u_int GetReceiveLength(SOCKET sock);
        bool Receive(SOCKET sock, char * buffer, u_int recvSize);
        bool SendMulticast(char * identifier);
        void RcvMulticast();

        void Init(HWND hWnd);
        void SetBlocking(SOCKET sock, bool block);
        sockaddr_in GetMulticastSenderInfo();

        USHORT GetPeerListenerPort();

    private:

        NetworkManager() {};
        ~NetworkManager();
                
        void StartWinsock();
        void JoinMulticastGroup(HWND hWnd);
        void SetupPeerListener(HWND hWnd);
        bool LeaveMulticastGroup();

        SOCKET GetPeerListenerSocket();
        SOCKET GetMulticastSocket();

        NetworkManager(NetworkManager const&);  // Singleton only
        void operator=(NetworkManager const&);  // Singleton only

        SOCKET m_mcst_sock;
        SOCKET m_p2p_lstn_sock;

};