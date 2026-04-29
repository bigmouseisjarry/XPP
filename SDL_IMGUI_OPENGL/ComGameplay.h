#pragma once
#include <string>

struct TagComponent {
    std::string name;
};

struct DestroyComponent {
    bool destroyed = false;
};

enum class PlayerState { Idle, ToWalk, Walk, Jump, Fall };
enum class PlayerDirection { Left, Right };

struct PlayerComponent {
    PlayerState state = PlayerState::Idle;
    PlayerState nextState = PlayerState::Idle;
    PlayerDirection direction = PlayerDirection::Left;
    float moveSpeed = 20.0f;
    float jumpForce = 40.0f;
};
