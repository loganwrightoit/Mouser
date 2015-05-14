#pragma once

#ifndef FILESENDER_PEER_H
#define FILESENDER_PEER_H
#include "Peer.h"
#endif

#include <fstream>

static HWND hDlgCurrent;

class FileSender
{

    public:

        struct FileInfo
        {
            wchar_t path[MAX_PATH];
            size_t size;
        };

        FileSender(Peer* peer, BOOL sender, FileInfo info);

        void getFileFragment(Packet* pkt);
        void onPeerAcceptRequest();
        void onPeerRejectRequest();
        static HWND getActive()
        {
            return hDlgCurrent;
        };

    private:

        static INT_PTR CALLBACK FileTransferDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

        void prepareToReceive();
        void updateProgressBar();
        void doFileSendThread();

        Peer* _peer;
        HWND _hDlg;
        
        // Holds path and size of file send
        FileSender::FileInfo _info;
        BOOL _sender; // Whether initiated transfer request
        std::ofstream _out; // Stream target for incoming file fragments
        size_t _remaining; // Amount of expected filesize read
        wchar_t _tempPath[MAX_PATH]; // Temporary file path
        wchar_t _tempExt[MAX_PATH]; // Temporary file extension

};

