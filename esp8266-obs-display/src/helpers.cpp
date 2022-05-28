#include "helpers.h"

std::string expand_to_n_chars(const std::string& str, size_t n)
{
    std::string result = str;
    while (result.size() < n)
    {
        result += " ";
    }
    return result;
}

std::string expand_to_n_chars(const char* str, size_t n)
{
    return expand_to_n_chars(std::string(str), n);
}