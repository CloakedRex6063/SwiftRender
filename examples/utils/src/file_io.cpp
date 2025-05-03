#include "file_io.hpp"

#include "fstream"

std::vector<char> Swift::ReadBinaryFile(std::string_view file_path)
{
    std::ifstream file(file_path.data(), std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return {};
    }
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    return buffer;
}

std::vector<char> Swift::ReadTextFile(std::string_view file_path)
{
    std::ifstream file(file_path.data(), std::ios::ate);
    if (!file.is_open())
    {
        return {};
    }
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    return buffer;
}