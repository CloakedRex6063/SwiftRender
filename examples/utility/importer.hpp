#pragma once
#include "swift_structs.hpp"
#include "meshoptimizer.h"
#include "tiny_gltf.h"
#include "span"
#include "glm/fwd.hpp"

struct Vertex
{
    std::array<float, 3> position;
    float uv_x;
    std::array<float, 3> normal;
    float uv_y;
    std::array<float, 4> tangent;
};

struct Mesh
{
    std::string name;
    std::vector<meshopt_Meshlet> meshlets;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> meshlet_vertices;
    std::vector<uint32_t> meshlet_triangles;
    int material_index;
};

struct Material
{
    std::array<float, 4> albedo;

    std::array<float, 3> emissive;
    int albedo_index;

    int emissive_index;
    int metal_rough_index;
    float metallic;
    float roughness;

    int normal_index;
    int occlusion_index;
    float alpha_cutoff;
    Swift::AlphaMode alpha_mode;
};

struct Texture
{
    std::string name;
    uint32_t sampler_index;
    uint32_t width;
    uint32_t height;
    uint16_t mip_levels;
    uint16_t array_size;
    Swift::Format format;
    std::vector<uint8_t> pixels;
};

struct Sampler
{
    std::string name;
    Swift::Filter min_filter;
    Swift::Filter mag_filter;
    Swift::Wrap wrap_u;
    Swift::Wrap wrap_y;
    Swift::Wrap wrap_w;
};

struct Node
{
    std::string name;
    uint32_t transform_index;
    uint32_t mesh_index;
};

struct Model
{
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<Texture> textures;
    std::vector<Sampler> samplers;
    std::vector<glm::mat4> transforms;
    std::vector<Node> nodes;
};

class Importer
{
public:
    Model LoadModel(std::string_view path);

private:
    static const float *GetAttributeData(const std::string &name, const tinygltf::Model &model,
                                         const tinygltf::Primitive &primitive, uint32_t &numAttributes);

    static std::vector<Vertex> LoadVertices(const tinygltf::Model &model, const tinygltf::Primitive &primitive,
                                            std::vector<uint32_t> &indices);

    static std::vector<uint32_t> LoadIndices(const tinygltf::Model &model, const tinygltf::Primitive &primitive);

    static std::tuple<std::vector<meshopt_Meshlet>, std::vector<uint32_t>, std::vector<uint8_t> > BuildMeshlets(
        std::span<const Vertex> vertices, std::span<const uint32_t> indices);

    static std::tuple<std::vector<Node>, std::vector<glm::mat4>> LoadNodes(const tinygltf::Model &model);
    static void LoadNode(const tinygltf::Model &model, int node_index, const glm::mat4 &parent_transform,
                          std::vector<Node> &nodes, std::vector<glm::mat4> &transforms);

    static std::vector<uint32_t> RepackMeshlets(std::span<meshopt_Meshlet> meshlets,
                                                std::span<const uint8_t> meshlet_triangles);

    static void LoadTangents(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices);

    static Texture LoadTexture(const tinygltf::Model &model, const tinygltf::Texture &texture);

    static glm::mat4 GetLocalTransform(const tinygltf::Node &node);

    static std::vector<Mesh> LoadMesh(const tinygltf::Model &model, const tinygltf::Mesh &mesh);

    static Material LoadMaterial(const tinygltf::Model &model, const tinygltf::Material &material);

    static Sampler LoadSampler(const tinygltf::Sampler &sampler);

    static Swift::Filter ToFilter(int filter);

    static Swift::Wrap ToWrap(int wrap);

    tinygltf::TinyGLTF m_loader;
};
