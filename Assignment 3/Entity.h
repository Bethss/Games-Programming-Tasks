#pragma once

#include "glm/mat4x4.hpp"
#include "ShaderProgram.h"

class Entity {
private:
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    glm::vec3 m_acceleration;
    glm::vec3 m_scale;
    glm::mat4 m_model_matrix;
    GLuint m_texture_id;
    GLuint m_fire_texture_id;
    GLuint m_explosion_texture_id;
    bool m_active; 
    bool m_should_update;

public:
    // Constructor with default active status
    Entity(GLuint texture_id, glm::vec3 position, glm::vec3 velocity, glm::vec3 scale, bool should_update = true, bool active = true)
        : m_texture_id(texture_id), m_position(position), m_velocity(velocity), m_scale(scale), m_active(active), m_should_update(should_update)
    {
        m_model_matrix = glm::mat4(1.0f);
        m_acceleration = glm::vec3(0.0f, -0.001f, 0.0f);
    }

    void set_active(bool active) { m_active = active; }
    bool is_active() const { return m_active; }

    void set_should_update(bool should_update) { m_should_update = should_update; }
    bool should_update() const { return m_should_update; }

    void set_position(glm::vec3 position) { m_position = position; }
    void set_velocity(glm::vec3 velocity) { m_velocity = velocity; }
    void set_acceleration(glm::vec3 acceleration) { m_acceleration = acceleration; }
    void set_scale(glm::vec3 scale) { m_scale = scale; }
    void set_texture_id(GLuint texture_id) { m_texture_id = texture_id; }
    void set_fire_texture(GLuint fire_texture_id) { m_fire_texture_id = fire_texture_id; }
    void set_explosion_texture(GLuint explosion_texture_id) { m_explosion_texture_id = explosion_texture_id; }
    
    GLuint get_explosion_texture_id() const { return m_explosion_texture_id; }
    GLuint get_fire_texture_id() const { return m_fire_texture_id; }
    glm::vec3 get_scale() const { return m_scale; }
    glm::vec3 get_velocity() const { return m_velocity; }
    glm::vec3 get_position() const { return m_position; }
    glm::vec3 get_acceleration() const { return m_acceleration; }
    GLuint get_texture_id() const { return m_texture_id; }

    void update(float delta_time);
    void render(ShaderProgram* program);
};
