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
        std::vector<Peer*> getPeers() const;
        Peer * getPeer(int idx) const;
        void connectToPeer();
        void handlePeerConnectionRequest(WPARAM wParam);

    private:

        PeerHandler() {};

        void connectToPeerThread(sockaddr_in inAddr);

        PeerHandler(PeerHandler const&);    // Singleton only
        void operator=(PeerHandler const&); // Singleton only

        void addPeerToListBox(Peer* peer);
        void removePeerFromListbox(Peer * peer);

        std::vector<Peer*> peers;

};

