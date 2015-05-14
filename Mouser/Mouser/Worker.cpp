#include "stdafx.h"
#include "Worker.h"
#include <thread>

Worker::Worker(SOCKET socket) : isReady(false)
{
    // Create queue mutex lock
    if ((ghMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
    {
        printf("[P2P]: CreateMutex() failed, queue not thread-safe.\n");
    }

    _socket = socket;
    std::thread t(&Worker::run, this);
    t.detach();
}

Worker::~Worker()
{
    CloseHandle(ghMutex);
}

void Worker::run()
{
    // Force system to create message queue
    MSG msg;
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    WNDCLASSEX wx = {};
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = WorkerProc;
    wx.hInstance = getHInst();
    wx.lpszClassName = L"WorkerClass";

    if (!RegisterClassEx(&wx))
    {
        printf("Unable to register worker class.\n");
    }

    //_ghReadyEvent = CreateEvent(
    //    NULL,               // default security attributes
    //    TRUE,               // manual-reset event
    //    FALSE,              // initial state is nonsignaled
    //    NULL  // object name
    //    );
    //if (_ghReadyEvent == NULL)
    //{
    //    printf("CreateEvent failed (%d)\n", GetLastError());
    //    return;
    //}

    // Create message-only window
    _hwnd = CreateWindowEx(0, wx.lpszClassName, wx.lpszClassName, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, this);
    
    // We should be able to receive packet messages now
    //SetEvent(_ghReadyEvent);

    // Start send thread
    std::thread t(&Worker::sendThread, this);
    t.detach();

    // Main message loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
        //TranslateMessage(&msg); // Process virtual key events
        DispatchMessage(&msg);
    }
}

HANDLE Worker::getReadyEvent() const
{
    return _ghReadyEvent;
}

void Worker::setReady()
{
    isReady = true;
}

LRESULT CALLBACK Worker::WorkerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Worker* worker;
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        worker = (Worker*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)worker);
    }
    else
    {
        worker = (Worker*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    switch (msg)
    {
        case WM_CREATE:
            {
                // Relying on other methods to determine when the window is ready
                // to receive messages is insufficient, so send the name here
                Peer* peer = PeerHandler::getInstance().getPeer(worker->_socket);
                auto buffer = peer->getNameData();
                worker->queuePacket(new Packet(Packet::NAME, buffer.first, buffer.second));
            }
            break;
        case WM_EVENT_SEND_PACKET:
            {
                OutputDebugString(L"Sending name packet to peer");
                auto pair = (std::pair<Worker*, Packet*>*)wParam;
                pair->first->queuePacket(pair->second);
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void Worker::sendPacket(Packet* pkt)
{
    // WorkerProc isn't passed 'this,' so we'll pass it here when sending packet
    SendMessage(_hwnd, WM_EVENT_SEND_PACKET, (WPARAM) &std::make_pair(this, pkt), NULL);
}

void Worker::queuePacket(Packet* pkt)
{
    if (WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0)
    {
        _outPkts.push(pkt);
        ReleaseMutex(ghMutex);
    }
}

void Worker::sendThread()
{
    while (1)
    {
        if (_outPkts.size() > 0)
        {
            if (WaitForSingleObject(ghMutex, INFINITE) == WAIT_OBJECT_0)
            {
                Packet* pkt = _outPkts.front();
                NetworkManager::getInstance().sendPacket(_socket, pkt);
                _outPkts.pop();
                delete pkt;
                ReleaseMutex(ghMutex);
            }
        }
        else
        {
            Sleep(15); // Awakened 66 times a second, more than sufficient
        }
    }
}

HWND Worker::getHandle() const
{
    return _hwnd;
}

bool Worker::ready() const
{
    return isReady && _outPkts.size() < 4;
}

void Worker::clearFileSender(void* peer)
{
    ((Peer*)peer)->clearFileSender();
}

void Worker::clearStreamSender(void* peer)
{
    ((Peer*)peer)->clearStreamSender();
}