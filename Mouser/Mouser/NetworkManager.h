#pragma once

#include "WinSock2.h"
#pragma comment (lib, "Ws2_32.lib")

class NetworkManager
{

    public:

        static NetworkManager& getInstance()
        {
            static NetworkManager instance;
            return instance;
        }

    private:

        NetworkManager() {};                    // Private constructor
        NetworkManager(NetworkManager const&);  // Singleton only
        void operator=(NetworkManager const&);  // Singleton only

        SOCKET m_mcst_sock;
        SOCKET m_p2p_lstn_sock;

};