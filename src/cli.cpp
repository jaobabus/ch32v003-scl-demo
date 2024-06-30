
#include "cli.hpp"

void coprintf(...) {}

void buf_insert(char* buffer, size_t buf_size, size_t index, char ch)
{
    for (size_t i = index; i < buf_size - 1; i++)
        buffer[i + 1] = buffer[i];
    buffer[index] = ch;
}

void buf_pop(char* buffer, size_t buf_size, size_t index)
{
    for (size_t i = index + 1; i < buf_size; i++)
        buffer[i - 1] = buffer[i];
}

