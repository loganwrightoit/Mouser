#include "stdafx.h"
#include "Worker.h"
#include <thread>

Worker::Worker(SOCKET socket) : isReady(false)
{
    // Create queue mutex lock
    if ((ghMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
    {
        //AddOutputMsg(L"[P2P]: CreateMutex() failed, queue not thread-safe.");
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
    WNDCLASSEX wx = {};
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = WorkerProc;
    wx.hInstance = getHInst();
    wx.lpszClassName = L"WorkerClass";

    if (!RegisterClassEx(&wx))
    {
        exit(1);
    }
    
    // Create message-only window
    _hwnd = CreateWindowEx(0, wx.lpszClassName, wx.lpszClassName, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, this);

    // Start send thread
    std::thread t(&Worker::sendThread, this);
    t.detach();

    // TODO: Figure out event-driven system for this
    isReady = true;

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        //TranslateMessage(&msg); // Process key events
        DispatchMessage(&msg);
    }
}

void Worker::setReady()
{
    isReady = true;
}

LRESULT CALLBACK Worker::WorkerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    /*
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
    */

    switch (msg)
    {
        case WM_EVENT_SEND_PACKET:
            {
                OutputDebugString(L"Event WM_EVENT_SEND_PACKET captured...\n");
                auto pair = (std::pair<Worker*, Packet*>*)wParam;
                pair->first->queuePacket(pair->second);
            }
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void Worker::sendPacket(Packet* pkt)
{
    OutputDebugString(L"Sending WM_EVENT_SEND_PACKET event...\n");

    // WorkerProc isn't passed 'this,' so we'll pass it here when sending packet
    SendMessage(_hwnd, WM_EVENT_SEND_PACKET, (WPARAM) &std::make_pair(this, pkt), NULL);
}

void Worker::queuePacket(Packet* pkt)
{
    OutputDebugString(L"Queueing packet...\n");

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
                OutputDebugString(L"Sending packet...\n");
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