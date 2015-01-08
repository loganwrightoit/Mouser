#pragma once
#pragma comment (lib, "Ws2_32.lib")

#include "stdafx.h"
#include "WinSock2.h"

class NetworkManager
{

    public:

        static NetworkManager& getInstance()
        {
            static NetworkManager instance;
            return instance;
        }

        void init(const HWND hWnd);

        bool sendPacket(SOCKET socket, Packet * pkt);

        // Returns <size,data> pair used to construct Packet
        Packet * getPacket(SOCKET socket);

        bool sendMulticast(char * identifier);
        void rcvMulticast();
        void setBlocking(SOCKET socket, bool block);
        sockaddr_in getMulticastSenderInfo() const;
        unsigned short getPeerListenerPort() const;
        bool isSocketReady(SOCKET socket) const;

    private:

        NetworkManager() {};
        ~NetworkManager()
        {
            leaveMulticastGroup();
        };
                
        void startWinsock();
        void joinMulticastGroup(const HWND hWnd);
        void setupPeerListener(const HWND hWnd);
        bool leaveMulticastGroup();

        bool doRcv(SOCKET socket, char * data, unsigned int size);
        
        SOCKET getPeerListenerSocket() const;
        SOCKET getMulticastSocket() const;

        NetworkManager(NetworkManager const&);  // Singleton only
        void operator=(NetworkManager const&);  // Singleton only

        SOCKET _mcst_socket;
        SOCKET _p2p_lstn_socket;

};