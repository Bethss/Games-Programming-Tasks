#include "Entity.h"
#include "ShaderProgram.h"
#include "glm/gtc/matrix_transform.hpp"

void Entity::update_model_matrix() {
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
    m_model_matrix = glm::rotate(m_model_matrix, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    m_model_matrix = glm::scale(m_model_matrix, m_scale);
}

void Entity::move(glm::vec3 direction, float delta_time) {
    if (!m_is_active) return;  // Skip movement if the entity is not active
    m_position += direction * m_speed * delta_time;
    update_model_matrix();
}

void Entity::animate(float delta_time, int cols) {
    if (!m_is_active) return;  // Skip animation if the entity is not active
    m_animation_time += delta_time;
    float frames_per_second = 1.0f / SECONDS_PER_FRAME;

    if (m_animation_time >= frames_per_second) {
        m_animation_time = 0.0f;
        m_animation_index++;

        if (m_animation_index >= m_animation_frames) m_animation_index = 0;
    }

    update_model_matrix();
}

void Entity::render(ShaderProgram* program) {
    if (!m_is_active) return;  // Skip rendering if the entity is not active

    program->set_model_matrix(m_model_matrix);

    if (m_animation_indices) {
        draw_sprite_from_texture_atlas(program, m_texture_id, m_animation_indices[m_animation_index], SPRITESHEET_DIMENSIONS, SPRITESHEET_DIMENSIONS);
    }
    else {
        // Render static sprite if no animation is set
        glBindTexture(GL_TEXTURE_2D, m_texture_id);

        float vertices[] = {
            -0.5f, -0.5f,
            0.5f, -0.5f,
            0.5f, 0.5f,
            -0.5f, -0.5f,
            0.5f, 0.5f,
            -0.5f, 0.5f
        };

        float tex_coords[] = {
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

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(program->get_position_attribute());
        glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
    }
}

void draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index, int rows, int cols) {
    float u_coord = (float)(index % cols) / (float)cols;
    float v_coord = (float)(index / cols) / (float)rows;

    float width = 1.0f / (float)cols;
    float height = 1.0f / (float)rows;

    float tex_coords[] = {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width, v_coord,
        u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };

    float vertices[] = {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };

    glBindTexture(GL_TEXTURE_2D, texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());

    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}
