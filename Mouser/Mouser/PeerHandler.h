#pragma once

class PeerHandler
{

    public:

        static PeerHandler& GetInstance()
        {
            static PeerHandler instance;
            return instance;
        }

        void ConnectToPeer();
        void ConnectToPeerThread(sockaddr_in inAddr);
        void ListenToPeerThread();
        void HandlePeerConnectionRequest(WPARAM wParam);

    private:

        PeerHandler()
        {
            m_peer_sock = INVALID_SOCKET;
        };

        PeerHandler(PeerHandler const&);    // Singleton only
        void operator=(PeerHandler const&); // Singleton only

        SOCKET m_peer_sock;

};

