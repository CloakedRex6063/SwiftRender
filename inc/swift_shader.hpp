#pragma once
#include "swift_macros.hpp"
#include "swift_structs.hpp"

namespace Swift
{
    class IShader
    {
    public:
        SWIFT_DESTRUCT(IShader);
        SWIFT_NO_MOVE(IShader);
        SWIFT_NO_COPY(IShader);
        [[nodiscard]] virtual void* GetPipeline() const = 0;
        ShaderType GetShaderType() const { return m_shader_type; }

    protected:
        explicit IShader(const ShaderType shader_type) : m_shader_type(shader_type) {}

    private:
        ShaderType m_shader_type;
    };
}  // namespace Swift
