#pragma once
#include <entt.hpp>
#include "ComTransform.h"


//TODO: 目前没有跨维度保护
//TODO: 可以用模板简化重复代码
namespace HierarchyUtils
{
    inline void MarkWorldDirty3D(entt::registry& registry, entt::entity entity)
    {
        auto* t = registry.try_get<Transform3DComponent>(entity);
        if (!t) return;
        t->isWorldDirty = true;
        for (auto child : t->children)
            MarkWorldDirty3D(registry, child);
    }

    inline void MarkWorldDirty2D(entt::registry& registry, entt::entity entity)
    {
        auto* t = registry.try_get<Transform2DComponent>(entity);
        if (!t) return;
        t->isWorldDirty = true;
        for (auto child : t->children)
            MarkWorldDirty2D(registry, child);
    }

    inline void MarkWorldDirty(entt::registry& registry, entt::entity entity)
    {
        MarkWorldDirty3D(registry, entity);
        MarkWorldDirty2D(registry, entity);
    }

    inline void Detach3D(entt::registry& registry, entt::entity entity)
    {
        auto& t = registry.get<Transform3DComponent>(entity);
        if (t.parent == entt::null) return;

        auto& parentT = registry.get<Transform3DComponent>(t.parent);
        auto& siblings = parentT.children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());

        t.parent = entt::null;
        t.isWorldDirty = true;
    }

    inline void Detach2D(entt::registry& registry, entt::entity entity)
    {
        auto& t = registry.get<Transform2DComponent>(entity);
        if (t.parent == entt::null) return;

        auto& parentT = registry.get<Transform2DComponent>(t.parent);
        auto& siblings = parentT.children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());

        t.parent = entt::null;
        t.isWorldDirty = true;
    }

    inline void Attach3D(entt::registry& registry, entt::entity child, entt::entity parent)
    {
        Detach3D(registry, child);

        auto& childT = registry.get<Transform3DComponent>(child);
        auto& parentT = registry.get<Transform3DComponent>(parent);

        childT.parent = parent;
        parentT.children.push_back(child);

        MarkWorldDirty3D(registry, child);
    }

    inline void Attach2D(entt::registry& registry, entt::entity child, entt::entity parent)
    {
        Detach2D(registry, child);

        auto& childT = registry.get<Transform2DComponent>(child);
        auto& parentT = registry.get<Transform2DComponent>(parent);

        childT.parent = parent;
        parentT.children.push_back(child);

        MarkWorldDirty2D(registry, child);
    }

    // 遍历副本，操作本体
    inline void DestroyWithChildren3D(entt::registry& registry, entt::entity entity)
    {
        auto* t = registry.try_get<Transform3DComponent>(entity);
        if (t)
        {
            // 先复制 children，因为遍历时会修改原容器
            auto childrenCopy = t->children;
            for (auto child : childrenCopy)
                DestroyWithChildren3D(registry, child);

            Detach3D(registry, entity);
        }
        registry.destroy(entity);
    }
    // 遍历副本，操作本体。
    inline void DestroyWithChildren2D(entt::registry& registry, entt::entity entity)
    {
        auto* t = registry.try_get<Transform2DComponent>(entity);
        if (t)
        {
            auto childrenCopy = t->children;
            for (auto child : childrenCopy)
                DestroyWithChildren2D(registry, child);

            Detach2D(registry, entity);
        }
        registry.destroy(entity);
    }
}