#pragma once
#include <entt.hpp>

namespace CameraSystem {
    void Update(entt::registry& registry);
}

namespace RenderSubmitSystem {
    void SubmitCameras(entt::registry& registry);
    void SubmitEntities(entt::registry& registry);
}

namespace PlayerSystem {
    void Update(entt::registry& registry, float deltaTime);
}

namespace CubeSystem {
    void Update(entt::registry& registry, float deltaTime);
}

namespace DestroySystem {
    void Update(entt::registry& registry);
}

namespace LightSystem {
    void Update(entt::registry& registry);
}

namespace AnimationSystem {
    void Update(entt::registry& registry, float deltaTime);
}

namespace HierarchySystem {
    void Update(entt::registry& registry);
}