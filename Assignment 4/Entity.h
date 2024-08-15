#pragma once

#include "glm/mat4x4.hpp"
#include "ShaderProgram.h"

// Constants
constexpr int SECONDS_PER_FRAME = 4;
constexpr int SPRITESHEET_DIMENSIONS = 4;

class Entity {
private:
    glm::vec3 m_position;
    glm::vec3 m_scale;
    glm::vec3 m_rotation;
    glm::mat4 m_model_matrix;
    GLuint m_texture_id;
    float m_speed;

    int* m_animation_indices;
    int m_animation_frames;
    int m_animation_index;
    float m_animation_time;
    bool m_is_active;  // Indicates whether the entity is active (alive) or not

public:
    Entity(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation, GLuint texture_id, float speed = 0.0f, bool is_active = true)
        : m_position(position), m_scale(scale), m_rotation(rotation), m_texture_id(texture_id), m_speed(speed),
        m_animation_indices(nullptr), m_animation_frames(1), m_animation_index(0), m_animation_time(0.0f), m_is_active(is_active)
    {
        m_model_matrix = glm::mat4(1.0f);
    }

    // Setters
    void set_position(const glm::vec3& position) { m_position = position; update_model_matrix(); }
    void set_scale(const glm::vec3& scale) { m_scale = scale; }
    void set_rotation(const glm::vec3& rotation) { m_rotation = rotation; }
    void set_speed(float speed) { m_speed = speed; }
    void set_animation(int* animation_indices, int animation_frames) {
        m_animation_indices = animation_indices;
        m_animation_frames = animation_frames;
    }
    void set_active(bool is_active) { m_is_active = is_active; }

    // Getters
    glm::vec3 get_position() const { return m_position; }
    glm::vec3 get_scale() const { return m_scale; }
    glm::vec3 get_rotation() const { return m_rotation; }
    float get_speed() const { return m_speed; }
    glm::mat4 get_model_matrix() const { return m_model_matrix; }
    GLuint get_texture_id() const { return m_texture_id; }
    bool is_active() const { return m_is_active; }

    // Other Methods
    void update_model_matrix();
    void render(ShaderProgram* program);
    void animate(float delta_time, int cols);
    void move(glm::vec3 direction, float delta_time);
};

void draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index, int rows, int cols);
