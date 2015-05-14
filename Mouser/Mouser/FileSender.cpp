#include "stdafx.h"
#include "FileSender.h"
#include "Commctrl.h"
#include <Shlobj.h> // for SUCCEEDED method
#include <thread>

#define PRG_BAR_PRECISION 100

FileSender::FileSender(Peer* peer, BOOL sender, FileInfo info) : _peer(peer), _sender(sender)
{
    memcpy_s(&_info, sizeof(struct FileInfo), &info, sizeof(struct FileInfo));
    _remaining = _info.size;

    // Create the request dialog
    _hDlg = CreateDialogParam(getHInst(), MAKEINTRESOURCE(IDD_DOWNLOADBOX), GetDesktopWindow(), (DLGPROC)FileTransferDlgProc, (LPARAM)this);

    // Set up progress bar
    HWND _pb = GetWindow(_hDlg, IDC_DOWNLOAD_PROGRESS);
    SendMessage(_pb, PBM_SETRANGE32, 0, PRG_BAR_PRECISION);
    SendMessage(_pb, PBM_SETSTEP, 1, 0);
    SendMessage(_pb, PBM_SETPOS, 0, 0);

    // Update status text
    if (_sender)
    {
        std::wstring str(L"Waiting for ");
        str.append(_peer->getName());
        str.append(L" to accept...");
        SetDlgItemText(_hDlg, IDC_DOWNLOAD_STATUS, str.c_str());
    }
    else
    {
        prepareToReceive();

        std::wstring str(L"Receiving file from ");
        str.append(_peer->getName());
        str.append(L"...");
        SetDlgItemText(_hDlg, IDC_DOWNLOAD_STATUS, str.c_str());
    }

    // Populate filename
    wchar_t fileName[MAX_PATH];
    wcscpy_s(fileName, MAX_PATH, _info.path);
    PathStripPath(fileName);
    SetDlgItemText(_hDlg, IDC_DOWNLOAD_FILENAME, fileName);

    // Populate file size in MB to two decimal points
    float szMB = (float)_info.size / 1048576;
    std::wstring str(std::to_wstring(szMB));
    str.append(L" MB");
    SetDlgItemText(_hDlg, IDC_DOWNLOAD_SIZE, str.c_str());
}

//
// Message handler.
//
INT_PTR CALLBACK FileSender::FileTransferDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_ACTIVATE:
        if (0 == wParam) // becoming inactive
            hDlgCurrent = NULL;
        else // becoming active
            hDlgCurrent = hDlg;         
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
        }
        break;
    }

    return (INT_PTR)FALSE;
}

void FileSender::prepareToReceive()
{
    // Get desktop path
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, 0, _tempPath)))
    {
        // Grab some path properties
        wchar_t fileName[MAX_PATH];
        _wsplitpath_s(_info.path, NULL, 0, NULL, 0, fileName, MAX_PATH, _tempExt, MAX_PATH);

        // Build a new path with temporary file extension
        PathAppend(_tempPath, fileName);
        PathRenameExtension(_tempPath, L".tmp");

        // Save remaining size for decrementing as fragments received
        _remaining = _info.size;

        // Open file in preparation for file fragments
        _out.open(_tempPath, std::ifstream::binary);
        if (_out.is_open())
        {
            // Send accept packet to begin receiving file
            _peer->getWorker()->sendPacket(new Packet(Packet::FILE_SEND_ALLOW));
        }
        else
        {
            printf("[P2P]: Unable to open temporary file for receiving send.\n");
        }
    }
    else
    {
        printf("[P2P]: Unable to get temporary file path.\n");
    }
}

void FileSender::onPeerAcceptRequest()
{
    // Set status text
    std::wstring str(L"Sending file to ");
    str.append(_peer->getName());
    str.append(L"...");
    SetDlgItemText(_hDlg, IDC_DOWNLOAD_STATUS, str.c_str());

    // Start new thread to send file
    std::thread t(&FileSender::doFileSendThread, this);
    t.detach();
}

void FileSender::onPeerRejectRequest()
{
    // Update status to complete
    std::wstring str(_peer->getName());
    str.append(L" denied file transfer request");
    SetDlgItemText(_hDlg, IDC_DOWNLOAD_STATUS, str.c_str());

    // Activate "OKAY" button
    HWND hwnd = GetDlgItem(_hDlg, IDOK);
    EnableWindow(hwnd, TRUE);
}

void FileSender::getFileFragment(Packet* pkt)
{
    // Check that file is open
    if (_out.is_open())
    {
        _out.write(pkt->getData(), pkt->getSize());

        // Decrement expected size
        _remaining -= pkt->getSize();

        // Update download progress bar
        updateProgressBar();

        // Close file if expected reaches 0
        if (_remaining <= 0)
        {
            // Close file stream
            _out.close();

            // Rename extension of temporary file
            wchar_t oldPath[MAX_PATH];
            wcscpy_s(oldPath, MAX_PATH, _tempPath);
            PathRenameExtension(_tempPath, _tempExt);
            _wrename(oldPath, _tempPath);

            // Update status to complete
            SetDlgItemText(_hDlg, IDC_DOWNLOAD_STATUS, L"Download complete, saved to Desktop");

            // Activate "OK" button
            HWND hwnd = GetDlgItem(_hDlg, IDOK);
            EnableWindow(hwnd, TRUE);

            _peer->clearFileSender();
        }
    }
}

void FileSender::doFileSendThread()
{
    // Open file
    std::ifstream file(_info.path, std::ifstream::binary);

    if (file.is_open())
    {
        // Send file fragments to peer
        while (1)
        {
            if (_peer->getWorker()->ready())
            {
                char buffer[FILE_BUFFER];
                size_t size;
                if (file.read(buffer, sizeof(buffer)))
                {
                    _remaining -= FILE_BUFFER;

                    // Send fragment
                    char* data = new char[sizeof(buffer)];
                    memcpy_s(data, sizeof(buffer), buffer, sizeof(buffer));
                    _peer->getWorker()->sendPacket(new Packet(Packet::FILE_FRAGMENT, data, sizeof(buffer)));
                }
                else if ((size = (size_t)file.gcount()) > 0)
                {
                    _remaining -= size;

                    // Send final fragment
                    char* data = new char[size];
                    memcpy_s(data, size, buffer, size); // Buffer only contains data up to file.gcount()
                    _peer->getWorker()->sendPacket(new Packet(Packet::FILE_FRAGMENT, data, size));
                }
                else
                {
                    // Complete progress bar
                    updateProgressBar();

                    // Update status to complete
                    SetDlgItemText(_hDlg, IDC_DOWNLOAD_STATUS, L"Send complete");

                    // Activate "OK" button
                    HWND hwnd = GetDlgItem(_hDlg, IDOK);
                    EnableWindow(hwnd, TRUE);

                    // Close file after send
                    file.close();
                    
                    _peer->clearFileSender();
                    return;
                }

                updateProgressBar();
            }
        }
    }
}

void FileSender::updateProgressBar()
{
    size_t new_pos = ((float)_info.size - _remaining) / _info.size * PRG_BAR_PRECISION;
    int pos = SendDlgItemMessage(_hDlg, IDC_DOWNLOAD_PROGRESS, PBM_GETPOS, 0, 0);
    if (new_pos > pos)
    {
        SendDlgItemMessage(_hDlg, IDC_DOWNLOAD_PROGRESS, PBM_SETPOS, new_pos, 0);
    }
}