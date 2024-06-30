#pragma once


#include <scl/parse.h>
#include <scl/inplace.h>
#include <scl/command.hpp>
#include <scl/executor.h>

void uwrite(const char* str, size_t size);
void coprintf(...);


void buf_insert(char* buffer, size_t buf_size, size_t index, char ch);
void buf_pop(char* buffer, size_t buf_size, size_t index);

class ConsoleBuffer
{
public:
    ConsoleBuffer()
        : _cursor(0), _buf_size(0), _escape(0)
    {
        memset(_buffer, 0, sizeof(_buffer));
    }

    virtual void on_command(char* buffer, size_t size) = 0;

    template<typename T>
    void safe_redraw(T cb)
    {
        uwrite("\e[?25l\e7", 8);
        cb();
        uwrite("\e8\e[?25h", 8);
    }

    void on_char(char ch)
    {
        if (_escape)
        {
            switch (_escape)
            {
            case 1:
                if (ch == '[')
                    _escape = 2;
                else
                    _escape = 0, coprintf("Esc error unknown 1lvl esc '%c' (%d)", ch, int(ch));
                break;
            case 2:
                switch (ch)
                {
                case 'A':
                    break;
                case 'B':
                    break;
                case 'C':
                    if (_cursor < _buf_size)
                    {
                        _cursor++;
                        uwrite("\e[1C", 4);
                    }
                    break;
                case 'D':
                    if (_cursor > 0)
                    {
                        _cursor--;
                        uwrite("\e[1D", 4);
                    }
                    break;
                case '3':
                    coprintf("DEL");
                    if (_cursor < _buf_size)
                    {
                        uwrite("\e[1D", 4);
                        buf_pop(_buffer, sizeof(_buffer), _cursor);
                        _buf_size--;
                        safe_redraw([&]
                        {
                            uwrite(_buffer + _cursor, _buf_size - _cursor);
                            uwrite(" ", 1);
                        });
                    }
                    _escape = 3;
                    break;
                default:
                    coprintf("Esc error unknown 2lvl esc '%c' (%d)", ch, int(ch));
                }
                if (_escape == 2)
                    _escape = 0;
                break;
            case 3:
                coprintf("Esc del end lvl 3: '%c' (%d)", ch, int(ch));
                _escape = 0;
                break;
            default:
                coprintf("Esc error lvl %d", _escape);
                _escape = 0;
            }
        }
        else if (ch == '\x7F')
        {
            coprintf("BS");
            if (_cursor == 0)
                return;
            _cursor--;
            uwrite("\e[1D", 4);
            buf_pop(_buffer, sizeof(_buffer), _cursor);
            _buf_size--;
            safe_redraw([&]
            {
                uwrite(_buffer + _cursor, _buf_size - _cursor);
                uwrite(" ", 1);
            });
        }
        else if (ch == '\e')
        {
            coprintf("ESC");
            _escape = 1;
        }
        else if (ch == '\n' or ch == '\r')
        {
            coprintf("Command '%s'", _buffer);
            uwrite("\r\n", 2);
            on_command(&_buffer[0], _buf_size);
            memset(_buffer, 0, sizeof(_buffer));
            _cursor = 0;
            _buf_size = 0;
        }
        else
        {
            coprintf("Add '%c' (%d)",
                     ch, int((uint8_t)ch));
            if (_cursor >= sizeof(_buffer) - 1)
               return;
            buf_insert(_buffer, sizeof(_buffer), _cursor, ch);
            _buf_size++;
            safe_redraw([&]
            {
                uwrite(_buffer + _cursor, _buf_size - _cursor);
            });
            _cursor++;
            uwrite("\e[1C", 4);
        }
        coprintf("cur:%+2d, buf:'%s', e:%d\n",
                 _cursor,
                 _buffer,
                 _escape);
    }

private:
    char _buffer[128];
    size_t _cursor;
    size_t _buf_size;
    int _escape;

};



class ConsoleExecutor : public ConsoleBuffer
{
public:
    struct CommandHandle
    {
        const SCLCommand* descriptor;
        void* opaque;
        void** arguments_opaque_table;
    };

public:
    ConsoleExecutor(CommandHandle* handlers, size_t count, const SCLAllocator* alloc)
        : _handlers(handlers), _size(count), _alloc(alloc) {}

public:
    void on_command(char* buffer, size_t size) override
    {
        shli_parse_inplace(buffer, size);
        auto first = shli_parse_data(buffer);
        auto next = shli_next_token(first);
        while (next.token == SHLT_Whitespace)
            next = shli_next_token(next);
        for (size_t i = 0; i < _size; i++)
        {
            auto& hnd = _handlers[i];
            if (hnd.descriptor->is_command(hnd.opaque, (const char*)first.data, first.size))
            {
                auto sz = size - ((char*)next.data - buffer);
                auto err = scl_execute_inplace(hnd.opaque, hnd.descriptor, hnd.arguments_opaque_table, _alloc, (char*)next.data, sz);
                if (err.error != 0)
                {
                    uwrite("Error ", 6);
                    char buf[2];
                    buf[1] = '0' + err.error % 10;
                    err.error /= 10;
                    buf[0] = '0' + err.error % 10;
                    uwrite(buf, 1);
                    uwrite(" in ", 4);
                    buf[0] = '0' + err.token;
                    uwrite(buf, 1);
                    uwrite("\r\n", 2);
                }
                return;
            }
        }
        ((char*)first.data)[first.size] = '\0';
        uwrite("Error: command '", 16);
        uwrite((char*)first.data, first.size);
        uwrite("' not found\r\n", 13);
    }

private:
    CommandHandle* _handlers;
    size_t _size;
    const SCLAllocator* _alloc;

};

template<typename Command>
constexpr ConsoleExecutor::CommandHandle default_handle(Command* cmd, void** args_opaque_table)
{
    return ConsoleExecutor::CommandHandle
    {
        &Command::sc_descriptor,
        cmd,
        args_opaque_table
    };
}

