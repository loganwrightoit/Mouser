#pragma once

class CursorUtil
{

    public:

        CursorUtil(void* peer, HWND hWnd);
        ~CursorUtil();

        void stream(int sendRate);
        void stop();

        POINT getCursor();
        RECT getRect(HICON cursor);

    private:

        void streamThread(int sendRate);

        void* _peer;
        POINT _cursor;
        HWND _hWnd;
        bool _streaming;

};

