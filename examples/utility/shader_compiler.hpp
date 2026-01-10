#pragma once
#include "vector"
#include "string_view"
#include "slang.h"
#include "slang-com-ptr.h"

enum class ShaderStage
{
    eMesh,
    eAmplification,
    ePixel,
    eCompute,
};

class ShaderCompiler
{
public:
    ShaderCompiler();
    [[nodiscard]] std::vector<uint8_t> CompileShader(std::string_view file_path, ShaderStage stage) const;

private:
    Slang::ComPtr<slang::IGlobalSession> m_global_session;
};