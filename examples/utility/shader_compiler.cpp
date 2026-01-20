#include "shader_compiler.hpp"
#include "string"
#include "vector"
#include "array"

ShaderCompiler::ShaderCompiler()
{
    constexpr SlangGlobalSessionDesc desc = {};
    slang::createGlobalSession(&desc, m_global_session.writeRef());
}

std::vector<uint8_t> ShaderCompiler::CompileShader(const std::string_view file_path, const ShaderStage stage) const
{
    std::array options =
    {
        slang::CompilerOptionEntry{
            slang::CompilerOptionName::DebugInformation,
            { slang::CompilerOptionValueKind::Int,
              SLANG_DEBUG_INFO_LEVEL_STANDARD }
        },
        slang::CompilerOptionEntry{
            slang::CompilerOptionName::Optimization,
            { slang::CompilerOptionValueKind::Int,
              SLANG_OPTIMIZATION_LEVEL_NONE }
        }
    };

    slang::TargetDesc target_desc = {
        .structureSize = sizeof(slang::TargetDesc),
        .format = SLANG_DXIL,
        .profile = m_global_session->findProfile("sm_6_8"),
        .compilerOptionEntries = options.data(),
        .compilerOptionEntryCount = options.size(),
    };
    const slang::SessionDesc session_desc{
        .structureSize = sizeof(slang::SessionDesc),
        .targets = &target_desc,
        .targetCount = 1,
        .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
    };
    Slang::ComPtr<slang::ISession> session;
    m_global_session->createSession(session_desc, session.writeRef());

    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    auto* module = session->loadModule(file_path.data(), diagnosticsBlob.writeRef());
    Slang::ComPtr slang_module(module);

    if (diagnosticsBlob) printf("%s\n", (const char*)diagnosticsBlob->getBufferPointer());

    std::string entry_name;
    switch (stage)
    {
        case ShaderStage::eMesh:
            entry_name = "mesh_main";
            break;
        case ShaderStage::eAmplification:
            entry_name = "ampl_main";
            break;
        case ShaderStage::ePixel:
            entry_name = "pixel_main";
            break;
        case ShaderStage::eCompute:
            entry_name = "compute_main";
            break;
    }

    Slang::ComPtr<slang::IEntryPoint> entry_point;
    slang_module->findEntryPointByName(entry_name.data(), entry_point.writeRef());

    slang::IComponentType* components[] = {slang_module, entry_point};
    Slang::ComPtr<slang::IComponentType> composed;
    session->createCompositeComponentType(components, 2, composed.writeRef(), diagnosticsBlob.writeRef());

    Slang::ComPtr<slang::IComponentType> linked;
    composed->link(linked.writeRef(), diagnosticsBlob.writeRef());

    Slang::ComPtr<slang::IBlob> dxil_code;
    linked->getEntryPointCode(0, 0, dxil_code.writeRef(), diagnosticsBlob.writeRef());

    if (diagnosticsBlob) printf("%s\n", (const char*)diagnosticsBlob->getBufferPointer());

    std::vector bytecode(static_cast<const uint8_t*>(dxil_code->getBufferPointer()),
                         static_cast<const uint8_t*>(dxil_code->getBufferPointer()) + dxil_code->getBufferSize());
    return bytecode;
}
