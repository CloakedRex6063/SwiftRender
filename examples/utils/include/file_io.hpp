#pragma once
#include "string"
#include "vector"

namespace Swift
{
    std::vector<char> ReadBinaryFile(std::string_view file_path);
    std::vector<char> ReadTextFile(std::string_view file_path);
}