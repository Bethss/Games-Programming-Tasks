#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

// Default constructor
Entity::Entity()
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(0.0f), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
    m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
    m_texture_id(0), m_velocity(0.0f), m_acceleration(0.0f), m_width(0.0f), m_height(0.0f),
    m_entity_type(PADDLE)  // Default to PADDLE
{
    // Initialize m_walking with zeros or any default value
    for (int i = 0; i < SECONDS_PER_FRAME; ++i)
        for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = 0;
}

// Parameterized constructor
Entity::Entity(GLuint texture_id, float speed, glm::vec3 acceleration, float jump_power, int walking[4][4], float animation_time,
    int animation_frames, int animation_index, int animation_cols,
    int animation_rows, float width, float height, EntityType type)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(speed), m_acceleration(acceleration), m_jumping_power(jump_power), m_animation_cols(animation_cols),
    m_animation_frames(animation_frames), m_animation_index(animation_index),
    m_animation_rows(animation_rows), m_animation_indices(nullptr),
    m_animation_time(animation_time), m_texture_id(texture_id), m_velocity(0.0f),
    m_width(width), m_height(height), m_entity_type(type)
{
    face_right();
    set_walking(walking);
}

// Simpler constructor for partial initialization
Entity::Entity(GLuint texture_id, float speed, float width, float height, EntityType type)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(speed), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
    m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
    m_texture_id(texture_id), m_velocity(0.0f), m_acceleration(0.0f), m_width(width), m_height(height),
    m_entity_type(type)
{
    // Initialize m_walking with zeros or any default value
    for (int i = 0; i < SECONDS_PER_FRAME; ++i)
        for (int j = 0; j < SECONDS_PER_FRAME; ++j) m_walking[i][j] = 0;
}

Entity::~Entity() { }

void Entity::draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index)
{
    // Step 1: Calculate the UV location of the indexed frame
    float u_coord = (float)(index % m_animation_cols) / (float)m_animation_cols;
    float v_coord = (float)(index / m_animation_cols) / (float)m_animation_rows;

    // Step 2: Calculate its UV size
    float width = 1.0f / (float)m_animation_cols;
    float height = 1.0f / (float)m_animation_rows;

    // Step 3: Just as we have done before, match the texture coordinates to the vertices
    float tex_coords[] =
    {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width, v_coord,
        u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };

    float vertices[] =
    {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };

    // Step 4: And render
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());

    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

bool const Entity::check_collision(Entity* other) const
{
    float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width + other->m_width) / 2.0f);
    float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);

    return x_distance < 0.0f && y_distance < 0.0f;
}

void const Entity::check_collision_y(Entity* collidable_entities, int collidable_entity_count)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity* collidable_entity = &collidable_entities[i];

        if (check_collision(collidable_entity))
        {
            float y_distance = fabs(m_position.y - collidable_entity->m_position.y);
            float y_overlap = fabs(y_distance - (m_height / 2.0f) - (collidable_entity->m_height / 2.0f));
            if (m_velocity.y > 0)
            {
                m_position.y -= y_overlap;
                m_velocity.y = 0;

                // Collision!
                m_collided_top = true;
            }
            else if (m_velocity.y < 0)
            {
                m_position.y += y_overlap;
                m_velocity.y = 0;

                // Collision!
                m_collided_bottom = true;
            }
        }
    }
}

void const Entity::check_collision_x(Entity* collidable_entities, int collidable_entity_count)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity* collidable_entity = &collidable_entities[i];

        if (check_collision(collidable_entity))
        {
            float x_distance = fabs(m_position.x - collidable_entity->m_position.x);
            float x_overlap = fabs(x_distance - (m_width / 2.0f) - (collidable_entity->m_width / 2.0f));
            if (m_velocity.x > 0)
            {
                m_position.x -= x_overlap;
                m_velocity.x = 0;

                // Collision!
                m_collided_right = true;

            }
            else if (m_velocity.x < 0)
            {
                m_position.x += x_overlap;
                m_velocity.x = 0;

                // Collision!
                m_collided_left = true;
            }
        }
    }
}

void Entity::update_ai(float delta_time) {
    if (m_is_ai_controlled) {
        // Moving AI up or down based on the current direction
        if (m_ai_moving_up) {
            m_movement.y = 1.0f;
        }
        else {
            m_movement.y = -1.0f;
        }

        // Updating the paddle position based on movement
        m_velocity.y = m_movement.y * m_speed;
        m_position.y += m_velocity.y * delta_time;

        // Reversing direction when hitting the screen edge
        if (m_position.y + (m_height / 2.0f) >= 3.75f) {
            m_ai_moving_up = false; // Start moving down
        }
        else if (m_position.y - (m_height / 2.0f) <= -3.75f) {
            m_ai_moving_up = true; // Start moving up
        }

        m_movement.y = 0.0f; // Reseting movement after applying it
    }
}


void Entity::update(float delta_time, Entity* collidable_entities, int collidable_entity_count)
{
    m_collided_top = false;
    m_collided_bottom = false;
    m_collided_left = false;
    m_collided_right = false;

    // Updating AI movement if applicable
    update_ai(delta_time);

    if (m_entity_type == PADDLE) {
        // Applying vertical movement for paddles
        m_velocity.y = m_movement.y * m_speed;
        m_position.y += m_velocity.y * delta_time;

        // Preventing paddles from moving off the screen
        if (m_position.y + (m_height / 2.0f) > 3.75f) {
            m_position.y = 3.75f - (m_height / 2.0f);
        }
        else if (m_position.y - (m_height / 2.0f) < -3.75f) {
            m_position.y = -3.75f + (m_height / 2.0f);
        }

        m_movement.y = 0.0f;  // Reseting movement after applying it

    }
    else if (m_entity_type == BALL) {
        // Applying both horizontal and vertical movement for the ball
        m_velocity += m_acceleration * delta_time;
        m_position += m_velocity * delta_time;

        // Bouncing the ball off the top and bottom boundaries
        if (m_position.y + (m_height / 2.0f) > 3.75f) {
            m_position.y = 3.75f - (m_height / 2.0f);
            m_velocity.y = -m_velocity.y; // Bounce off the top
        }
        else if (m_position.y - (m_height / 2.0f) < -3.75f) {
            m_position.y = -3.75f + (m_height / 2.0f);
            m_velocity.y = -m_velocity.y; // Bounce off the bottom
        }

        check_collision_y(collidable_entities, collidable_entity_count);
        check_collision_x(collidable_entities, collidable_entity_count);
    }

    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
    m_model_matrix = glm::scale(m_model_matrix, m_scale);
}

void Entity::render(ShaderProgram* program)
{
    program->set_model_matrix(m_model_matrix);
    glUseProgram(program->get_program_id());

    float vertices[] =
    {
        -0.5f * m_width, -0.5f * m_height,
         0.5f * m_width, -0.5f * m_height,
         0.5f * m_width,  0.5f * m_height,
        -0.5f * m_width, -0.5f * m_height,
         0.5f * m_width,  0.5f * m_height,
        -0.5f * m_width,  0.5f * m_height
    };

    float tex_coords[] =
    {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());

    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}
