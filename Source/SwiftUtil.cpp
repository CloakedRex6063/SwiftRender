#include "SwiftUtil.hpp"
#include "SwiftStructs.hpp"
#include "glm/gtx/norm.hpp"

namespace
{
    Swift::Plane CreatePlane(
        const glm::vec3& position,
        const glm::vec3& normal)
    {
        return {
            .normal = glm::normalize(normal),
            .distance = glm::dot(glm::normalize(normal), position),
        };
    };

    bool IsInsidePlane(
        const Swift::Plane& plane,
        const Swift::BoundingSphere& sphere)
    {
        return glm::dot(plane.normal, sphere.center) - plane.distance > -sphere.radius;
    }

    Swift::BoundingAABB CreateBoundingAABB(
        const glm::vec3 minAABB,
        const glm::vec3 maxAABB)
    {
        const auto center = (maxAABB - minAABB) * 0.5f;
        const auto extents = maxAABB - center;
        return {center, 0, extents, 0};
    }

    [[nodiscard]]
    std::vector<std::filesystem::path> GetAllFilesInDirectory(
        const std::filesystem::path& folderPath,
        const bool recursive)
    {
        std::vector<std::filesystem::path> files;
        if (recursive)
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath))
            {
                if (std::filesystem::is_regular_file(entry.path()))
                {
                    files.emplace_back(entry.path());
                }
            }
        }
        else
        {
            for (const auto& entry : std::filesystem::directory_iterator(folderPath))
            {
                if (std::filesystem::is_regular_file(entry.path()))
                {
                    files.emplace_back(entry.path());
                }
            }
        }
        return files;
    }

    auto beginTime = std::chrono::high_resolution_clock::now();
    auto endTime = std::chrono::high_resolution_clock::now();
} // namespace

namespace Swift
{
    BoundingSphere
    Visibility::CreateBoundingSphereFromVertices(const std::span<glm::vec3> positions)
    {
        auto minAABB = glm::vec3(std::numeric_limits<float>::max());
        auto maxAABB = glm::vec3(std::numeric_limits<float>::min());
        for (auto&& position : positions)
        {
            minAABB.x = std::min(minAABB.x, position.x);
            minAABB.y = std::min(minAABB.y, position.y);
            minAABB.z = std::min(minAABB.z, position.z);

            maxAABB.x = std::max(maxAABB.x, position.x);
            maxAABB.y = std::max(maxAABB.y, position.y);
            maxAABB.z = std::max(maxAABB.z, position.z);
        }

        return BoundingSphere((maxAABB + minAABB) * 0.5f, glm::length(minAABB - maxAABB) * 0.5f);
    }

    BoundingAABB Visibility::CreateBoundingAABBFromVertices(const std::span<glm::vec3> positions)
    {
        auto minAABB = glm::vec3(std::numeric_limits<float>::max());
        auto maxAABB = glm::vec3(std::numeric_limits<float>::min());
        for (auto&& position : positions)
        {
            minAABB.x = std::min(minAABB.x, position.x);
            minAABB.y = std::min(minAABB.y, position.y);
            minAABB.z = std::min(minAABB.z, position.z);

            maxAABB.x = std::max(maxAABB.x, position.x);
            maxAABB.y = std::max(maxAABB.y, position.y);
            maxAABB.z = std::max(maxAABB.z, position.z);
        }
        return CreateBoundingAABB(minAABB, maxAABB);
    }

    void Visibility::UpdateFrustum(
        Frustum& frustum,
        const glm::mat4& viewMatrix,
        const glm::vec3 cameraPos,
        const float nearClip,
        const float farClip,
        const float fov,
        const float aspect)
    {
        const auto forward =
            glm::normalize(-glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]));
        const auto right =
            glm::normalize(glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]));
        const auto up =
            glm::normalize(glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]));
        const float halfVSide = farClip * tanf(fov * .5f);
        const float halfHSide = halfVSide * aspect;
        const glm::vec3 frontMultFar = farClip * forward;

        frustum.nearFace = CreatePlane(cameraPos + nearClip * forward, forward);
        frustum.farFace = CreatePlane(cameraPos + frontMultFar, -forward);
        frustum.rightFace =
            CreatePlane(cameraPos, glm::cross(frontMultFar - right * halfHSide, up));
        frustum.leftFace = CreatePlane(cameraPos, glm::cross(up, frontMultFar + right * halfHSide));
        frustum.topFace = CreatePlane(cameraPos, glm::cross(right, frontMultFar - up * halfVSide));
        frustum.bottomFace =
            CreatePlane(cameraPos, glm::cross(frontMultFar + up * halfVSide, right));
    }

    bool Visibility::IsInFrustum(
        const Frustum& frustum,
        const BoundingSphere& sphere,
        const glm::mat4& worldTransform)
    {
        const auto globalScale = GetScaleFromMatrix(worldTransform);
        const auto globalCenter = worldTransform * glm::vec4(sphere.center, 1.0f);
        const float maxScale = std::max(std::max(globalScale.x, globalScale.y), globalScale.z);

        const BoundingSphere boundingSphere{
            glm::vec3(globalCenter),
            sphere.radius * maxScale};

        return IsInsidePlane(frustum.leftFace, boundingSphere) &&
               IsInsidePlane(frustum.rightFace, boundingSphere) &&
               IsInsidePlane(frustum.topFace, boundingSphere) &&
               IsInsidePlane(frustum.bottomFace, boundingSphere) &&
               IsInsidePlane(frustum.nearFace, boundingSphere) &&
               IsInsidePlane(frustum.farFace, boundingSphere);
    }
    
    void Performance::BeginTimer()
    {
        beginTime = std::chrono::high_resolution_clock::now();
    }
    
    float Performance::EndTimer()
    {
        const auto endTime = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float>(endTime - beginTime).count();
    }
} // namespace Swift::Util