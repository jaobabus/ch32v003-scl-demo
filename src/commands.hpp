#pragma once

#include "cli.hpp"



constexpr SCLError StringArgErrorToken = SCLE_UserErrorsStart;
constexpr SCLError IntArgInvalidChar = SCLError(SCLE_UserErrorsStart + 1);


class StringArg : public TypedArgument<StringView>
{
public:
    SCLError parse(type& value, SHLITokenInfo token) const noexcept override
    {
        value = StringView{(const char*)token.data, token.size};
        return SCLE_NoError;
    }

};


class EchoCommand : public TypedCommand<StringArg>
{
public:
    EchoCommand()
        : TypedCommand<StringArg>("echo") {}

public:
    SCLError execute(const StringView& sw) const noexcept override
    {
        uwrite(sw.data(), sw.size());
        uwrite("\r\n", 2);
        return SCLE_NoError;
    }
};


class IntArg : public TypedArgument<int>
{
public:
    SCLError parse(type& value, SHLITokenInfo token) const noexcept override
    {
        const char* it = (char*)token.data, * end = it + token.size;
        if (*it == '-')
            it++;
        value = 0;
        while (it != end)
        {
            value *= 10;
            if (*it >= '0' and *it <= '9')
                value += *it - '0';
            else
                return IntArgInvalidChar;
            ++it;
        }
        if (((char*)token.data)[0] == '-')
            value = -value;
        return SCLE_NoError;
    }

};


class AddCommand : public TypedCommand<IntArg, IntArg>
{
public:
    AddCommand()
        : TypedCommand("add") {}

public:
    SCLError execute(const int& a, const int& b) const noexcept override
    {
        char buf[10];
        int r = a + b;
        bool neg = false;
        uint8_t it = 7;
        if (r < 0)
            r = -r, neg = true;
        while (r != 0)
        {
            buf[it] = '0' + r % 10;
            r /= 10;
            --it;
        }
        if (neg)
            buf[it--] = '-';
        buf[8] = '\r';
        buf[9] = '\n';
        uwrite(buf + it + 1, 9 - it);
        return SCLE_NoError;
    }
};
