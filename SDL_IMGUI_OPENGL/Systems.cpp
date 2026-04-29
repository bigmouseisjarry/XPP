#include "Systems.h"
#include "ComCamera.h"
#include "ComTransform.h"
#include "ComGameplay.h"
#include "ComPhysics.h"
#include "ComRender.h"
#include "GameSettings.h"
#include "HierarchyUtils.h"

namespace AssistSystem {

    static void PlayerUpdateAnimation(entt::registry& registry, entt::entity entity, PlayerComponent& player, Visible2DComponent& visible)
    {
        TextureID texID;
        switch (player.state)
        {
        case PlayerState::Idle:
            texID = (player.direction == PlayerDirection::Left)
                ? ResourceManager::Get()->GetTextureID("resources/idle/leftidle.png")
                : ResourceManager::Get()->GetTextureID("resources/idle/rightidle.png");
            break;
        case PlayerState::Walk:
            texID = (player.direction == PlayerDirection::Left)
                ? ResourceManager::Get()->GetTextureID("resources/walk/leftwalk.png")
                : ResourceManager::Get()->GetTextureID("resources/walk/rightwalk.png");
            break;
        case PlayerState::Jump:
        case PlayerState::Fall:
            texID = (player.direction == PlayerDirection::Left)
                ? ResourceManager::Get()->GetTextureID("resources/walk/leftwalk.png")
                : ResourceManager::Get()->GetTextureID("resources/walk/rightwalk.png");
            break;
        default:
            return;
        }

        visible.material->SetTexture(TextureSemantic::Albedo, texID);

        if (registry.all_of<AnimatedSpriteComponent>(entity)) {
            auto& anim = registry.get<AnimatedSpriteComponent>(entity);
            anim.m_FrameDur = 0.1f;
            anim.m_CurrentFrame = 0;
            anim.m_AnimTimer = 0.0f;
            anim.ResetLoopCount();
        }
        else
            registry.emplace<AnimatedSpriteComponent>(entity, 0.1f);
    }

    static void PlayerChangeState(entt::registry& registry, entt::entity entity, PlayerComponent& player, Visible2DComponent& visible, PlayerState newState)
    {
        if (player.state == newState) return;
        player.state = newState;
        PlayerUpdateAnimation(registry, entity, player, visible);
    }

    static void HandleIdleState(entt::registry& registry, entt::entity entity, PlayerComponent& player, Physics2DComponent& physics, Visible2DComponent& visible)
    {
        glm::vec2 vel = physics.velocity;
        bool moving = false;

        if (GameSettings::Get()->IsActionDown(InputAction::MoveForward)) {
            vel.x = player.moveSpeed;
            player.direction = PlayerDirection::Right;
            moving = true;
        }
        else if (GameSettings::Get()->IsActionDown(InputAction::MoveBackward)) {
            vel.x = -player.moveSpeed;
            player.direction = PlayerDirection::Left;
            moving = true;
        }

        if (GameSettings::Get()->IsActionPressed(InputAction::Jump) && physics.isGrounded) {
            vel.y = player.jumpForce;
            physics.velocity = vel;
            physics.isGrounded = false;
            PlayerChangeState(registry, entity, player, visible, PlayerState::Jump);
            return;
        }

        if (moving) {
            physics.velocity = vel;
            player.state = PlayerState::ToWalk;
            player.nextState = PlayerState::Walk;
            TextureID texID = (player.direction == PlayerDirection::Left)
                ? ResourceManager::Get()->GetTextureID("resources/walk/leftfromidle.png")
                : ResourceManager::Get()->GetTextureID("resources/walk/rightfromidle.png");
            visible.material->SetTexture(TextureSemantic::Albedo, texID);
            auto& anim = registry.get<AnimatedSpriteComponent>(entity);
            anim.m_FrameDur = 0.1f;
            anim.m_CurrentFrame = 0;
            anim.m_AnimTimer = 0.0f;
            anim.ResetLoopCount();
        }
    }

    static void HandleToWalkState(entt::registry& registry, entt::entity entity, PlayerComponent& player, Visible2DComponent& visible)
    {
        if (registry.get<AnimatedSpriteComponent>(entity).GetLoopCount() > 0)
            PlayerChangeState(registry, entity, player, visible, PlayerState::Walk);
    }

    static void HandleWalkState(entt::registry& registry, entt::entity entity, PlayerComponent& player, Physics2DComponent& physics, Visible2DComponent& visible)
    {
        glm::vec2 vel = physics.velocity;
        bool moving = false;

        if (GameSettings::Get()->IsActionDown(InputAction::MoveForward)) {
            vel.x = player.moveSpeed;
            player.direction = PlayerDirection::Right;
            moving = true;
        }
        else if (GameSettings::Get()->IsActionDown(InputAction::MoveBackward)) {
            vel.x = -player.moveSpeed;
            player.direction = PlayerDirection::Left;
            moving = true;
        }

        if (GameSettings::Get()->IsActionPressed(InputAction::Jump) && physics.isGrounded) {
            vel.y = player.jumpForce;
            physics.velocity = vel;
            physics.isGrounded = false;
            PlayerChangeState(registry, entity, player, visible, PlayerState::Jump);
            return;
        }

        if (!moving) {
            physics.velocity = vel;
            PlayerChangeState(registry, entity, player, visible, PlayerState::Idle);
        }
        else {
            physics.velocity = vel;
        }
    }

    static void HandleJumpState(PlayerComponent& player, Physics2DComponent& physics)
    {
        glm::vec2 vel = physics.velocity;

        if (GameSettings::Get()->IsActionDown(InputAction::MoveForward)) {
            vel.x = player.moveSpeed;
            player.direction = PlayerDirection::Right;
        }
        else if (GameSettings::Get()->IsActionDown(InputAction::MoveBackward)) {
            vel.x = -player.moveSpeed;
            player.direction = PlayerDirection::Left;
        }
        else {
            vel.x = 0.0f;
        }

        physics.velocity = vel;
    }
}

void CameraSystem::Update(entt::registry& registry)
{
    // 2D 相机：projView = projection * inverse(transform.GetMatrix())
    auto view2D = registry.view<Camera2DComponent, Transform2DComponent>();
    for (auto [entity, cam, transform] : view2D.each()) {
        if (cam.isDirty) {
            glm::mat4 view = glm::inverse(transform.GetMatrix());
            cam.projView = cam.projection * view;
            cam.isDirty = false;
        }
    }

    // 3D 相机：同步位置到 Transform3D，再从 Transform3D 读 position 算 view
    auto view3D = registry.view<Camera3DComponent, Transform3DComponent>();
    for (auto [entity, cam, transform] : view3D.each()) {
        if (cam.isDirty) {
            // 将轨道计算的位置写入 Transform3D
            transform.SetPosition(cam.CalcPosition());
            // 从 Transform3D 读 position 计算 view matrix
            glm::mat4 view = glm::lookAt(transform.position, transform.position + cam.front, cam.up);
            cam.projView = cam.projection * view;
            cam.isDirty = false;
        }
    }
}

void RenderSubmitSystem::SubmitCameras(entt::registry& registry)
{
    // 2D 相机
    auto view2D = registry.view<Camera2DComponent>();
    for (auto [entity, cam] : view2D.each()) {
        if (cam.enabled)
            Renderer::Get()->SubmitCameraUnits(cam.projView, cam.targetLayers, RenderMode::Render2D);
    }

    // 3D 相机
    auto view3D = registry.view<Camera3DComponent, Transform3DComponent>();
    for (auto [entity, cam, transform] : view3D.each()) {
        if (cam.enabled)
        {
            Renderer::Get()->SetProjection(cam.projection);
            Renderer::Get()->SubmitCameraUnits(cam.projView, cam.targetLayers, RenderMode::Render3D, transform.position);
        }
    }
}

void RenderSubmitSystem::SubmitEntities(entt::registry& registry)
{
    // 2D 可见实体
    auto view2D = registry.view<Visible2DComponent, Transform2DComponent>();
    for (auto [entity, visible, transform] : view2D.each())
    {
        if (!visible.isVisible) continue;
        Renderer::Get()->SubmitRenderUnits(
            visible.mesh, visible.material.get(), transform.GetWorldMatrix(registry), visible.renderLayer);
    }

    // 3D 可见实体（不含 Model3D）
    auto view3D = registry.view<Visible3DComponent, Transform3DComponent>(
        entt::exclude<Model3DComponent>);
    for (auto [entity, visible, transform] : view3D.each())
    {
        if (!visible.isVisible) continue;
        Renderer::Get()->SubmitRenderUnits(
            visible.mesh, visible.material.get(), transform.GetWorldMatrix(registry), visible.renderLayer);
    }

    // Model3D 实体（多子网格）
    auto viewModel = registry.view<Model3DComponent, Transform3DComponent>();
    for (auto [entity, model, transform] : viewModel.each())
    {
        for (const auto& sub : model.subMeshes)
        {
            Renderer::Get()->SubmitRenderUnits(
                sub.mesh, sub.material.get(), transform.GetWorldMatrix(registry), model.renderLayer);
        }
    }
}

void PlayerSystem::Update(entt::registry& registry, float deltaTime)
{
    auto view = registry.view<PlayerComponent, Physics2DComponent, Visible2DComponent, Transform2DComponent>();
    for (auto [entity, player, physics, visible, transform] : view.each())
    {
        // 空中状态检测
        if (player.state == PlayerState::Jump || player.state == PlayerState::Fall)
        {
            if (physics.isGrounded)
                AssistSystem::PlayerChangeState(registry, entity, player, visible, PlayerState::Idle);
            else if (physics.velocity.y < 0 && player.state == PlayerState::Jump)
                AssistSystem::PlayerChangeState(registry, entity,player, visible, PlayerState::Fall);
        }

        switch (player.state)
        {
        case PlayerState::Idle:   AssistSystem::HandleIdleState(registry, entity, player, physics, visible); break;
        case PlayerState::ToWalk: AssistSystem::HandleToWalkState(registry, entity, player, visible); break;
        case PlayerState::Walk:   AssistSystem::HandleWalkState(registry, entity, player, physics, visible); break;
        case PlayerState::Jump:   AssistSystem::HandleJumpState(player, physics); break;
        case PlayerState::Fall:   AssistSystem::HandleJumpState(player, physics); break;
        }

    }
}

void CubeSystem::Update(entt::registry& registry, float deltaTime)
{
    auto view = registry.view< TagComponent,Transform3DComponent>();
    for (auto [entity, tag, transform] : view.each())
    {
        //if (tag.name == "Cube")
        
    }
}

void DestroySystem::Update(entt::registry& registry)
{
    auto view = registry.view<DestroyComponent>();
    std::vector<entt::entity> toDestroy;
    for (auto [entity, destroy] : view.each()) {
        if (destroy.destroyed)
            toDestroy.push_back(entity);
    }
    for (auto entity : toDestroy) {
        registry.destroy(entity);  // 组件析构自动调用，unique_ptr<Material> 释放
    }
}

void AnimationSystem::Update(entt::registry& registry, float deltaTime)
{
    auto view = registry.view<AnimatedSpriteComponent, Visible2DComponent>();
    for (auto [entity, anim, visible] : view.each())
    {
        TextureID texID = visible.material->GetTexture(TextureSemantic::Albedo);
        const Texture* tex = ResourceManager::Get()->GetTexture(texID);
        if (!tex) continue;

        anim.m_AnimTimer += deltaTime;
        if (anim.m_AnimTimer >= anim.m_FrameDur)
        {
            anim.m_CurrentFrame++;
            int maxFrame = tex->GetCols() * tex->GetRows();
            if (anim.m_CurrentFrame >= maxFrame)
            {
                anim.m_CurrentFrame = 0;
                anim.m_LoopCount++;
            }
            anim.m_AnimTimer = 0.0f;
        }

        // 应用帧 → UV 写入 Material.m_Properties
        int cols = tex->GetCols();
        int rows = tex->GetRows();
        float w = 1.0f / cols;
        float h = 1.0f / rows;
        int x = anim.m_CurrentFrame / rows;
        int y = anim.m_CurrentFrame % rows;

        visible.material->Set("u_UVscale", glm::vec2(w, h));
        visible.material->Set("u_UVoffset", glm::vec2((float)x * w, (float)(rows - 1 - y) * h));
    }
}

// 调用顺序规则：任何可能修改 Transform 的 System 之后，渲染相关的 System 之前
void HierarchySystem::Update(entt::registry& registry)
{
    // 3D：局部脏 → 标脏子树
    {
        auto view = registry.view<Transform3DComponent>();
        for (auto [entity, t] : view.each())
        {
            if (t.isDirty)
            {
				HierarchyUtils::MarkWorldDirty3D(registry, entity);
            }
        }
    }

    // 2D：局部脏 → 标脏子树
    {
        auto view = registry.view<Transform2DComponent>();
        for (auto [entity, t] : view.each())
        {
            if (t.isDirty)
            {
                HierarchyUtils::MarkWorldDirty2D(registry, entity);
            }
        }
    }
}

void LightSystem::Update(entt::registry& registry)
{
    auto view = registry.view<Light3DComponent, Transform3DComponent>();
    for (auto [entity, light, transform] : view.each())
    {
        light.position = transform.position;
    }
}
