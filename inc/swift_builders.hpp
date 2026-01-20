#pragma once
#include "enum_flags.hpp"
#include "swift_context.hpp"
#include "swift_structs.hpp"

#include <filesystem>

namespace Swift
{
    struct ContextBuilder
    {
        ContextBuilder(const uint32_t width,
                       const uint32_t height,
                       void* window_handle
#ifdef SWIFT_LINUX
                       ,
                       void* display_handle
#endif
        )
        {
            m_width = width;
            m_height = height;
            m_window_handle = window_handle;
#ifdef SWIFT_LINUX
            m_display_handle = display_handle;
#endif
        }

        ContextBuilder& SetBackend(const BackendType type)
        {
            m_backend_type = type;
            return *this;
        }

        std::shared_ptr<IContext> Build() const
        {
            return CreateContext({.backend_type = m_backend_type,
                                  .width = m_width,
                                  .height = m_height,
                                  .native_window_handle = m_window_handle,
                                  .native_display_handle = m_display_handle});
        }

    private:
        uint32_t m_width = 0;
        uint32_t m_height = 0;
#ifdef SWIFT_WINDOWS
        BackendType m_backend_type = BackendType::eD3D12;
#elif defined(SWIFT_LINUX)
        BackendType m_backend_type = BackendType::eVulkan;
#endif
        void* m_display_handle = nullptr;
        void* m_window_handle = nullptr;
    };

    struct TextureBuilder
    {
        TextureBuilder(const std::shared_ptr<IContext>& context, const uint32_t width, const uint32_t height)
        {
            m_context = context;
            m_width = width;
            m_height = height;
        }

        TextureBuilder& SetMipmapLevels(const uint32_t levels)
        {
            m_mip_levels = levels;
            return *this;
        }

        TextureBuilder& SetArraySize(const uint32_t array_size)
        {
            m_array_size = array_size;
            return *this;
        }

        TextureBuilder& SetFormat(const Format format)
        {
            m_format = format;
            return *this;
        }

        TextureBuilder& SetFlags(const EnumFlags<TextureFlags> texture_flags)
        {
            m_texture_flags = texture_flags;
            return *this;
        }

        TextureBuilder& SetData(const void* data)
        {
            m_data = data;
            return *this;
        }

        TextureBuilder& SetMSAA(MSAA msaa)
        {
            m_msaa = msaa;
            return *this;
        }

        TextureBuilder& SetResource(const std::shared_ptr<IResource>& resource)
        {
            m_resource = resource;
            return *this;
        }

        [[nodiscard]] std::shared_ptr<ITexture> Build() const
        {
            const TextureCreateInfo info{.width = m_width,
                                         .height = m_height,
                                         .mip_levels = m_mip_levels,
                                         .array_size = m_array_size,
                                         .format = m_format,
                                         .flags = m_texture_flags,
                                         .data = m_data,
                                         .msaa = m_msaa,
                                         .resource = m_resource};
            return m_context->CreateTexture(info);
        }

    private:
        std::shared_ptr<IContext> m_context;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint16_t m_mip_levels = 1;
        uint16_t m_array_size = 1;
        Format m_format = Format::eRGBA8_UNORM;
        EnumFlags<TextureFlags> m_texture_flags = TextureFlags::eNone;
        std::shared_ptr<IResource> m_resource;
        const void* m_data = nullptr;
        std::optional<MSAA> m_msaa;
    };

    struct DescriptorBuilder
    {
        DescriptorBuilder(DescriptorType descriptor_type) { m_descriptor_type = descriptor_type; }

        DescriptorBuilder& SetShaderRegister(const uint32_t shader_register)
        {
            m_shader_register = shader_register;
            return *this;
        }

        DescriptorBuilder& SetRegisterSpace(const uint32_t space)
        {
            m_register_space = space;
            return *this;
        }

        DescriptorBuilder& SetShaderVisibility(const ShaderVisibility visibility)
        {
            m_shader_visibility = visibility;
            return *this;
        }

        [[nodiscard]] Descriptor Build() const
        {
            return {
                .shader_register = m_shader_register,
                .register_space = m_register_space,
                .descriptor_type = m_descriptor_type,
                .shader_visibility = m_shader_visibility,
            };
        }

    private:
        uint32_t m_shader_register = 0;
        uint32_t m_register_space = 0;
        DescriptorType m_descriptor_type = DescriptorType::eConstant;
        ShaderVisibility m_shader_visibility = ShaderVisibility::eAll;
    };

    struct SamplerDescriptorBuilder
    {
        SamplerDescriptorBuilder() {}

        SamplerDescriptorBuilder& SetWrapU(const Wrap wrap)
        {
            m_wrap_u = wrap;
            return *this;
        }

        SamplerDescriptorBuilder& SetWrapY(const Wrap wrap)
        {
            m_wrap_y = wrap;
            return *this;
        }

        SamplerDescriptorBuilder& SetWrapW(const Wrap wrap)
        {
            m_wrap_w = wrap;
            return *this;
        }

        SamplerDescriptorBuilder& SetMagFilter(const Filter mag_filter)
        {
            m_mag_filter = mag_filter;
            return *this;
        }

        SamplerDescriptorBuilder& SetMinFilter(const Filter min_filter)
        {
            m_min_filter = min_filter;
            return *this;
        }

        [[nodiscard]] SamplerDescriptor Build() const
        {
            return {
                .min_filter = m_min_filter,
                .mag_filter = m_mag_filter,
                .wrap_u = m_wrap_u,
                .wrap_y = m_wrap_y,
                .wrap_w = m_wrap_w,
            };
        }

    private:
        Filter m_min_filter = Filter::eLinear;
        Filter m_mag_filter = Filter::eLinear;
        Wrap m_wrap_u = Wrap::eRepeat;
        Wrap m_wrap_y = Wrap::eRepeat;
        Wrap m_wrap_w = Wrap::eRepeat;
    };

    struct GraphicsShaderBuilder
    {
        GraphicsShaderBuilder(const std::shared_ptr<IContext>& context) { m_context = context; }
        GraphicsShaderBuilder& SetRTVFormats(const std::vector<Format>& rtv_formats)
        {
            m_rtv_formats = rtv_formats;
            return *this;
        }
        GraphicsShaderBuilder& SetDSVFormat(const Format& dsv_format)
        {
            m_dsv_format = dsv_format;
            return *this;
        }
        GraphicsShaderBuilder& SetMeshShader(const std::vector<uint8_t>& mesh_shader)
        {
            m_mesh_code = mesh_shader;
            return *this;
        }
        GraphicsShaderBuilder& SetPixelShader(const std::vector<uint8_t>& pixel_shader)
        {
            m_pixel_code = pixel_shader;
            return *this;
        }
        GraphicsShaderBuilder& SetAmplificationShader(const std::vector<uint8_t>& amplify_shader)
        {
            m_amplify_code = amplify_shader;
            return *this;
        }
        GraphicsShaderBuilder& SetDepthWriteEnable(const bool enable)
        {
            m_depth_stencil_state.depth_write_enable = enable;
            return *this;
        }
        GraphicsShaderBuilder& SetDepthTestEnable(const bool enable)
        {
            m_depth_stencil_state.depth_enable = enable;
            return *this;
        }
        GraphicsShaderBuilder& SetDepthTest(const DepthTest depth_test)
        {
            m_depth_stencil_state.depth_test = depth_test;
            return *this;
        }
        GraphicsShaderBuilder& SetStencilTestEnable(const bool enable)
        {
            m_depth_stencil_state.stencil_enable = enable;
            return *this;
        }
        GraphicsShaderBuilder& SetFillMode(const FillMode fill_mode)
        {
            m_rasterizer_state.fill_mode = fill_mode;
            return *this;
        }
        GraphicsShaderBuilder& SetCullMode(const CullMode cull_mode)
        {
            m_rasterizer_state.cull_mode = cull_mode;
            return *this;
        }
        GraphicsShaderBuilder& SetFrontFace(const FrontFace front_face)
        {
            m_rasterizer_state.front_face = front_face;
            return *this;
        }
        GraphicsShaderBuilder& SetPolygonMode(const PolygonMode polygon_mode)
        {
            m_polygon_mode = polygon_mode;
            return *this;
        }
        GraphicsShaderBuilder& SetDescriptors(const std::vector<Descriptor>& descriptors)
        {
            m_descriptors = descriptors;
            return *this;
        }
        GraphicsShaderBuilder& SetStaticSamplers(const std::vector<SamplerDescriptor>& samplers)
        {
            m_static_samplers = samplers;
            return *this;
        }

        std::shared_ptr<IShader> Build() const
        {
            const GraphicsShaderCreateInfo shader_create_info{
                .rtv_formats = m_rtv_formats,
                .dsv_format = m_dsv_format,
                .amplify_code = m_amplify_code,
                .mesh_code = m_mesh_code,
                .pixel_code = m_pixel_code,
                .depth_stencil_state = m_depth_stencil_state,
                .rasterizer_state = m_rasterizer_state,
                .polygon_mode = m_polygon_mode,
                .descriptors = m_descriptors,
                .static_samplers = m_static_samplers,
            };
            return m_context->CreateShader(shader_create_info);
        }

    private:
        std::shared_ptr<IContext> m_context;
        std::vector<Format> m_rtv_formats{};
        std::optional<Format> m_dsv_format = std::nullopt;
        std::vector<uint8_t> m_amplify_code;
        std::vector<uint8_t> m_mesh_code;
        std::vector<uint8_t> m_pixel_code;
        DepthStencilState m_depth_stencil_state{
            .depth_enable = false,
            .depth_write_enable = false,
            .depth_test = DepthTest::eNever,
            .stencil_enable = false,
        };
        RasterizerState m_rasterizer_state{
            .fill_mode = FillMode::eSolid,
            .cull_mode = CullMode::eBack,
            .front_face = FrontFace::eCounterClockwise,
        };
        PolygonMode m_polygon_mode = PolygonMode::eTriangle;
        std::vector<Descriptor> m_descriptors{};
        std::vector<SamplerDescriptor> m_static_samplers{};
    };

    struct BufferBuilder
    {
        BufferBuilder(const std::shared_ptr<IContext>& context, const BufferType buffer_type, uint32_t size = 65536)
        {
            m_context = context;
            m_buffer_type = buffer_type;
            m_element_size = size;
        }

        BufferBuilder& SetElementSize(uint32_t size)
        {
            m_element_size = size;
            return *this;
        }

        BufferBuilder& SetNumElements(uint32_t num_elements)
        {
            m_num_elements = num_elements;
            return *this;
        }

        BufferBuilder& SetData(const void* data)
        {
            m_data = data;
            return *this;
        }

        BufferBuilder& SetResource(const std::shared_ptr<IResource>& resource)
        {
            m_resource = resource;
            return *this;
        }

        std::shared_ptr<IBuffer> Build() const
        {
            return m_context->CreateBuffer({
                .num_elements = m_num_elements,
                .element_size = m_element_size,
                .first_element = m_first_element,
                .data = m_data,
                .type = m_buffer_type,
                .resource = m_resource,
            });
        }

    private:
        std::shared_ptr<IContext> m_context;
        uint32_t m_first_element = 0;
        uint32_t m_element_size = 0;
        uint32_t m_num_elements = 1;
        const void* m_data = nullptr;
        BufferType m_buffer_type = BufferType::eConstantBuffer;
        std::shared_ptr<IResource> m_resource = nullptr;
    };

}  // namespace Swift
