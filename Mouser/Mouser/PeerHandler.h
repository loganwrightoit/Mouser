#pragma once

#include <vector>

class PeerHandler
{

    public:

        static PeerHandler& getInstance()
        {
            static PeerHandler instance;
            return instance;
        }

        void connectToPeer();
        void disconnectPeer(Peer * peer);
        std::vector<Peer*> getPeers() const;
        void handlePeerConnectionRequest(WPARAM wParam);

    private:

        PeerHandler() {};

        void connectToPeerThread(sockaddr_in inAddr);

        PeerHandler(PeerHandler const&);    // Singleton only
        void operator=(PeerHandler const&); // Singleton only

        std::vector<Peer*> peers;

};

