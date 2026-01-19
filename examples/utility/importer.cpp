#include "importer.hpp"
#include "format"
#include "mikktspace.h"
#include "span"
#include "algorithm"
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

Model Importer::LoadModel(const std::string_view path)
{
    tinygltf::Model model;
    std::string warn, error;
    const std::string filepath(path);
    if (path.ends_with(".gltf"))
    {
        if (const auto result = m_loader.LoadASCIIFromFile(&model, &error, &warn, filepath); !result)
        {
            printf(std::format(" Error: {} ", error, " Warn: {}", warn).c_str());
        }
    }

    if (path.ends_with(".glb"))
    {
        if (const auto result = m_loader.LoadBinaryFromFile(&model, &error, &warn, filepath); !result)
        {
            printf(std::format(" Error: {} ", error, " Warn: {}", warn).c_str());
        }
    }

    Model m{};
    for (auto& mesh : model.meshes)
    {
        auto meshes = LoadMesh(model, mesh);
        m.meshes.insert(m.meshes.end(), meshes.begin(), meshes.end());
    }

    for (auto& texture : model.textures)
    {
        auto tex = LoadTexture(model, texture);
        m.textures.emplace_back(tex);
    }

    for (auto& material : model.materials)
    {
        auto mat = LoadMaterial(model, material);
        m.materials.emplace_back(mat);
    }

    for (auto& sampler : model.samplers)
    {
        auto samp = LoadSampler(sampler);
        m.samplers.emplace_back(samp);
    }

    std::tie(m.nodes, m.transforms) = LoadNodes(model);

    return m;
}

const float* Importer::GetAttributeData(const std::string& name,
                                        const tinygltf::Model& model,
                                        const tinygltf::Primitive& primitive,
                                        uint32_t& numAttributes)
{
    if (primitive.attributes.find(name) == primitive.attributes.end())
    {
        return {};
    }
    const auto attribute = primitive.attributes.at(name);
    const auto& accessor = model.accessors[attribute];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];
    numAttributes = uint32_t(accessor.count);
    return reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
}

std::vector<Vertex> Importer::LoadVertices(const tinygltf::Model& model,
                                           const tinygltf::Primitive& primitive,
                                           std::vector<uint32_t>& indices)
{
    std::vector<Vertex> vertices;
    uint32_t num_pos = 0;
    uint32_t num_waste = 0;
    const float* const positions = GetAttributeData("POSITION", model, primitive, num_pos);
    const float* const normals = GetAttributeData("NORMAL", model, primitive, num_waste);
    const float* const tex_coords = GetAttributeData("TEXCOORD_0", model, primitive, num_waste);
    vertices.resize(num_pos);
    for (size_t i = 0; i < num_pos; ++i)
    {
        Vertex& v = vertices[i];

        std::memcpy(v.position.data(), positions + i * 3, sizeof(float) * 3);
        std::memcpy(v.normal.data(), normals + i * 3, sizeof(float) * 3);

        v.uv_x = tex_coords[i * 2 + 0];
        v.uv_y = tex_coords[i * 2 + 1];

        v.tangent = {0.f, 0.f, 0.f, 0.f};
    }
    LoadTangents(vertices, indices);

    return vertices;
}

std::vector<uint32_t> Importer::LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
    std::vector<uint32_t> indices;
    const auto& accessor = model.accessors[primitive.indices];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];
    indices.resize(accessor.count);
    const uint8_t* indexRawData = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
    const uint8_t* byteIndices = nullptr;
    const uint16_t* shortIndices = nullptr;
    const uint32_t* intIndices = nullptr;
    switch (accessor.componentType)
    {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            byteIndices = indexRawData;
            for (uint32_t i = 0; i < accessor.count; i++)
            {
                indices[i] = (static_cast<uint32_t>(byteIndices[i]));
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            shortIndices = reinterpret_cast<const uint16_t*>(indexRawData);
            for (uint32_t i = 0; i < accessor.count; i++)
            {
                indices[i] = (static_cast<uint32_t>(shortIndices[i]));
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            intIndices = reinterpret_cast<const uint32_t*>(indexRawData);
            for (uint32_t i = 0; i < accessor.count; i++)
            {
                indices[i] = (static_cast<uint32_t>(intIndices[i]));
            }
            break;
        default:;
    }
    return indices;
}

std::tuple<std::vector<meshopt_Meshlet>, std::vector<uint32_t>, std::vector<uint8_t>> Importer::BuildMeshlets(
    const std::span<const Vertex> vertices,
    const std::span<const uint32_t> indices)
{
    std::vector<meshopt_Meshlet> meshlets;
    std::vector<uint32_t> mesh_vertices;
    std::vector<uint8_t> mesh_triangles;
    const auto max_meshlets = meshopt_buildMeshletsBound(indices.size(), 64, 124);
    meshlets.resize(max_meshlets);
    mesh_vertices.resize(max_meshlets * 64);
    mesh_triangles.resize(max_meshlets * 124 * 3);
    const auto meshlet_count = meshopt_buildMeshlets(meshlets.data(),
                                                     mesh_vertices.data(),
                                                     mesh_triangles.data(),
                                                     indices.data(),
                                                     indices.size(),
                                                     reinterpret_cast<const float*>(vertices.data()),
                                                     vertices.size(),
                                                     sizeof(Vertex),
                                                     64,
                                                     124,
                                                     0.f);
    const auto& [vertex_offset, triangle_offset, vertex_count, triangle_count] = meshlets[meshlet_count - 1];
    mesh_vertices.resize(vertex_offset + vertex_count);
    mesh_triangles.resize(triangle_offset + (triangle_count * 3 + 3 & ~3));
    meshlets.resize(meshlet_count);
    return {meshlets, mesh_vertices, mesh_triangles};
}

std::tuple<std::vector<Node>, std::vector<glm::mat4>> Importer::LoadNodes(const tinygltf::Model& model)
{
    std::vector<Node> nodes;
    std::vector<glm::mat4> transforms;

    for (const auto& node : model.scenes[0].nodes)
    {
        LoadNode(model, node, glm::mat4(1.0f), nodes, transforms);
    }

    return {nodes, transforms};
}

void Importer::LoadNode(const tinygltf::Model& model,
                        const int node_index,
                        const glm::mat4& parent_transform,
                        std::vector<Node>& nodes,
                        std::vector<glm::mat4>& transforms)
{
    const tinygltf::Node& gltf_node = model.nodes[node_index];

    glm::mat4 world_transform = parent_transform * GetLocalTransform(gltf_node);

    const auto transform_idx = static_cast<uint32_t>(transforms.size());
    transforms.push_back(world_transform);

    if (gltf_node.mesh != -1)
    {
        nodes.push_back(Node{
            .name = gltf_node.name,
            .transform_index = transform_idx,
            .mesh_index = static_cast<uint32_t>(gltf_node.mesh),
        });
    }

    for (const int child : gltf_node.children)
    {
        LoadNode(model, child, world_transform, nodes, transforms);
    }
}

std::vector<uint32_t> Importer::RepackMeshlets(std::span<meshopt_Meshlet> meshlets,
                                               const std::span<const uint8_t> meshlet_triangles)
{
    std::vector<uint32_t> repacked_meshlets;
    for (auto& m : meshlets)
    {
        const auto triangle_offset = static_cast<uint32_t>(repacked_meshlets.size());

        for (uint32_t i = 0; i < m.triangle_count; ++i)
        {
            const auto idx0 = meshlet_triangles[m.triangle_offset + i * 3 + 0];
            const auto idx1 = meshlet_triangles[m.triangle_offset + i * 3 + 1];
            const auto idx2 = meshlet_triangles[m.triangle_offset + i * 3 + 2];
            auto packed = (static_cast<uint32_t>(idx0) & 0xFF) << 0 | (static_cast<uint32_t>(idx1) & 0xFF) << 8 |
                          (static_cast<uint32_t>(idx2) & 0xFF) << 16;
            repacked_meshlets.push_back(packed);
        }

        m.triangle_offset = triangle_offset;
    }
    return repacked_meshlets;
}

void Importer::LoadTangents(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
    struct Pair
    {
        std::span<Vertex> vertices;
        std::span<uint32_t> indices;
    } pair{
        .vertices = vertices,
        .indices = indices,
    };
    SMikkTSpaceInterface space_interface{
        .m_getNumFaces = [](const SMikkTSpaceContext* ctx) -> int
        { return static_cast<Pair*>(ctx->m_pUserData)->indices.size() / 3; },
        .m_getNumVerticesOfFace = [](const SMikkTSpaceContext*, int) -> int { return 3; },
        .m_getPosition =
            [](const SMikkTSpaceContext* ctx, float out[], int faceIdx, int vertIdx)
        {
            const auto pair = static_cast<Pair*>(ctx->m_pUserData);
            int idx = pair->indices[faceIdx * 3 + vertIdx];
            out[0] = pair->vertices[idx].position[0];
            out[1] = pair->vertices[idx].position[1];
            out[2] = pair->vertices[idx].position[2];
        },
        .m_getNormal =
            [](const SMikkTSpaceContext* ctx, float out[], int faceIdx, int vertIdx)
        {
            const auto pair = static_cast<Pair*>(ctx->m_pUserData);
            int idx = pair->indices[faceIdx * 3 + vertIdx];
            out[0] = pair->vertices[idx].normal[0];
            out[1] = pair->vertices[idx].normal[1];
            out[2] = pair->vertices[idx].normal[2];
        },
        .m_getTexCoord =
            [](const SMikkTSpaceContext* ctx, float out[], int faceIdx, int vertIdx)
        {
            const auto pair = static_cast<Pair*>(ctx->m_pUserData);
            int idx = pair->indices[faceIdx * 3 + vertIdx];
            out[0] = pair->vertices[idx].uv_x;
            out[1] = pair->vertices[idx].uv_y;
        },
        .m_setTSpaceBasic = static_cast<decltype(SMikkTSpaceInterface::m_setTSpaceBasic)>(
            [](const SMikkTSpaceContext* ctx, const float tangent[], const float sign, const int faceIdx, const int vertIdx)
            {
                const auto pair = static_cast<Pair*>(ctx->m_pUserData);
                int idx = pair->indices[faceIdx * 3 + vertIdx];
                pair->vertices[idx].tangent = {tangent[0], tangent[1], tangent[2], sign};
            })};

    const SMikkTSpaceContext context{
        .m_pInterface = &space_interface,
        .m_pUserData = &pair,
    };
    genTangSpaceDefault(&context);
}

Texture Importer::LoadTexture(const tinygltf::Model& model, const tinygltf::Texture& texture)
{
    const auto image = model.images[texture.source];
    Texture t{
        .name = texture.name,
        .width = (uint32_t)image.width,
        .height = (uint32_t)image.height,
        .mip_levels = 1,
        .array_size = 1,
        .format = Swift::Format::eRGBA8_UNORM,
        .pixels = image.image,
    };
    return t;
}

glm::mat4 Importer::GetLocalTransform(const tinygltf::Node& node)
{
    if (!node.matrix.empty())
    {
        return glm::make_mat4(node.matrix.data());
    }

    auto T = glm::mat4(1.0f);
    auto R = glm::mat4(1.0f);
    auto S = glm::mat4(1.0f);

    if (!node.translation.empty())
    {
        T = glm::translate(glm::mat4(1.0f), glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
    }

    if (!node.rotation.empty())
    {
        const glm::quat q(static_cast<float>(node.rotation[3]),
                          static_cast<float>(node.rotation[0]),
                          static_cast<float>(node.rotation[1]),
                          static_cast<float>(node.rotation[2]));
        R = glm::mat4_cast(q);
    }

    if (!node.scale.empty())
    {
        S = glm::scale(glm::mat4(1.0f), glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
    }

    return T * R * S;
}

std::vector<Mesh> Importer::LoadMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh)
{
    std::vector<Mesh> meshes;
    for (const auto& prim : mesh.primitives)
    {
        auto indices = LoadIndices(model, prim);
        const auto vertices = LoadVertices(model, prim, indices);
        auto [meshlets, meshlet_vertices, mesh_triangles] = BuildMeshlets(vertices, indices);
        const auto repacked_triangles = RepackMeshlets(meshlets, mesh_triangles);
        Mesh m{
            .name = mesh.name,
            .meshlets = meshlets,
            .vertices = vertices,
            .meshlet_vertices = meshlet_vertices,
            .meshlet_triangles = repacked_triangles,
            .material_index = prim.material,
        };
        meshes.emplace_back(m);
    }
    return meshes;
}

Material Importer::LoadMaterial(const tinygltf::Model& model, const tinygltf::Material& material)
{
    std::array<float, 4> albedo{};
    std::ranges::transform(material.pbrMetallicRoughness.baseColorFactor,
                           albedo.begin(),
                           [](const double d) { return static_cast<float>(d); });
    std::array<float, 3> emissive{};
    std::ranges::transform(material.emissiveFactor, emissive.begin(), [](const double d) { return static_cast<float>(d); });
    Swift::AlphaMode alpha_mode = Swift::AlphaMode::eOpaque;
    if (material.alphaMode == "TRANSPARENT")
    {
        alpha_mode = Swift::AlphaMode::eTransparent;
    }

    int albedo_index = -1;
    if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
    {
        albedo_index = material.pbrMetallicRoughness.baseColorTexture.index;
    }

    int emissive_index = -1;
    if (material.emissiveTexture.index != -1)
    {
        emissive_index = material.emissiveTexture.index;
    }

    int metal_rough_index = -1;
    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
    {
        metal_rough_index = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
    }

    int normal_index = -1;
    if (material.normalTexture.index != -1)
    {
        normal_index = material.normalTexture.index;
    }

    int occlusion_index = -1;
    if (material.occlusionTexture.index != -1)
    {
        occlusion_index = material.occlusionTexture.index;
    }

    const Material m{
        .albedo = albedo,
        .emissive = emissive,
        .albedo_index = albedo_index,
        .emissive_index = emissive_index,
        .metal_rough_index = metal_rough_index,
        .metallic = static_cast<float>(material.pbrMetallicRoughness.metallicFactor),
        .roughness = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor),
        .normal_index = normal_index,
        .occlusion_index = occlusion_index,
        .alpha_cutoff = static_cast<float>(material.alphaCutoff),
        .alpha_mode = alpha_mode,
    };
    return m;
}

Sampler Importer::LoadSampler(const tinygltf::Sampler& sampler)
{
    Sampler s{
        .name = sampler.name,
        .min_filter = ToFilter(sampler.minFilter),
        .mag_filter = ToFilter(sampler.magFilter),
        .wrap_u = ToWrap(sampler.wrapS),
        .wrap_y = ToWrap(sampler.wrapT),
    };

    return s;
}

Swift::Filter Importer::ToFilter(int filter)
{
    switch (filter)
    {
        case 9728:
            return Swift::Filter::eNearest;
        case 9729:
            return Swift::Filter::eLinear;
        case 9984:
            return Swift::Filter::eNearestMipNearest;
        case 9985:
            return Swift::Filter::eLinearMipNearest;
        case 9986:
            return Swift::Filter::eNearestMipLinear;
        case 9987:
            return Swift::Filter::eLinearMipLinear;
    }
    return Swift::Filter::eNearest;
}

Swift::Wrap Importer::ToWrap(const int wrap)
{
    switch (wrap)
    {
        case 10497:
            return Swift::Wrap::eRepeat;
        case 33071:
            return Swift::Wrap::eClampToEdge;
        case 33648:
            return Swift::Wrap::eMirroredRepeat;
    }
    return Swift::Wrap::eRepeat;
}
