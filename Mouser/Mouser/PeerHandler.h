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

        void addPeer(Peer* peer);
        void removePeer(Peer* peer);

		void directConnectToPeer(const char* ip);
        void connectToPeer();
        std::vector<Peer*> getPeers() const;
        void handlePeerConnectionRequest(WPARAM wParam);
        Peer* getPeer(HWND hWnd);
        Peer* PeerHandler::getPeer(SOCKET socket);

    private:

        PeerHandler() {};

        void connectToPeerThread(sockaddr_in inAddr);

        PeerHandler(PeerHandler const&);    // Singleton only
        void operator=(PeerHandler const&); // Singleton only

        std::vector<Peer*> peers;

};

