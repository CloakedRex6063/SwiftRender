#pragma once
#include "swift_structs.hpp"
#include "cmath"

namespace Swift
{
    template<typename CreationFunc, typename T>
    T* CreateObject(CreationFunc&& func, std::vector<T*>& objects, std::vector<uint32_t>& free_objects)
    {
        if (!free_objects.empty())
        {
            uint32_t index = free_objects.back();
            free_objects.pop_back();

            objects[index] = func();
            return objects[index];
        }

        objects.emplace_back(func());
        return objects.back();
    }

    template<typename T>
    void DestroyObject(T* object, std::vector<T*>& objects, std::vector<uint32_t>& free_objects)
    {
        if (auto it = std::ranges::find(objects, object); it != objects.end())
        {
            delete *it;
            *it = nullptr;
            free_objects.emplace_back(std::distance(objects.begin(), it));
        }
    }

    inline int CalculateMaxMips(const int width, const int height)
    {
        const int max_dim = std::max(width, height);
        return static_cast<int>(std::floor(std::log2(max_dim))) + 1;
    }

    inline std::array<uint32_t, 3> CalculateDispatchGroups(const uint32_t total_groups)
    {
        constexpr uint32_t MAX_DISPATCH = 65535;

        std::array<uint32_t, 3> result = {1, 1, 1};

        if (total_groups <= MAX_DISPATCH)
        {
            result[0] = total_groups;
            return result;
        }

        result[0] = MAX_DISPATCH;
        uint32_t remaining = (total_groups + MAX_DISPATCH - 1) / MAX_DISPATCH;

        if (remaining <= MAX_DISPATCH)
        {
            result[1] = remaining;
            return result;
        }

        result[1] = MAX_DISPATCH;
        result[2] = (remaining + MAX_DISPATCH - 1) / MAX_DISPATCH;

        return result;
    }
}  // namespace Swift
