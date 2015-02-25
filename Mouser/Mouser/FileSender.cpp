#include "stdafx.h"
#include "FileSender.h"

FileSender::FileSender()
{
}

FileSender::~FileSender()
{
}

/*
    1. Sender sends file name and size to peer for permission (attach incremented ID to identify response).
    2. Filename is added to map with ID key for later.
    3. Receiver sends FileSendResponse packet with ID and bool for accept or deny
            struct FileSendResponse
            {
                byte id;
                bool accept;
            }
    4. Sender creates new thread and begins file transfer.
    5. Receiver opens file transfer window, showing percentage complete.
*/