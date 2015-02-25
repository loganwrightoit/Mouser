#pragma once

class FileSender
{

    public:

        struct FileInfo
        {
            wchar_t path[MAX_PATH];
            size_t size;
        };

        FileSender();
        ~FileSender();

};

