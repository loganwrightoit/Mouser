#pragma once

class StringUtil
{

    public:

        StringUtil();
        ~StringUtil();

        std::string utf8_encode(const std::wstring &wstr);
        std::wstring utf8_decode(const std::string &str);

};

