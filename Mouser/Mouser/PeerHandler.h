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

        void disconnectPeer(Peer * peer);
        int PeerHandler::getNumPeers() const;
        Peer * getPeer(int idx) const;
        Peer * getDefaultPeer() const; // For testing only, returns first peer in list
        void connectToPeer();
        void handlePeerConnectionRequest(WPARAM wParam);

    private:

        PeerHandler() {};

        void connectToPeerThread(sockaddr_in inAddr);

        PeerHandler(PeerHandler const&);    // Singleton only
        void operator=(PeerHandler const&); // Singleton only

        std::vector<Peer*> peers;

};

