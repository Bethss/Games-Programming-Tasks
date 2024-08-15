/**
* Author: Elizabeth Akindeko
* Assignment: Rise of the AI
* Date due: 2024-07-27, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include <vector>
#include <cmath>
#include "Entity.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 840,
WINDOW_HEIGHT = 680;
constexpr float BG_RED = 0.2039f,
BG_GREEN = 0.6353f,
BG_BLUE = 0.949f,
BG_OPACITY = 1.0f;
constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";
constexpr float MILLISECONDS_IN_SECOND = 1000.0f;
constexpr char SPRITESHEET_FILEPATH[] = "Butterfly_Anim_Sprite_Sheet.png",
FONTSHEET_FILEPATH[] = "LLPixel_Fonts_Sprite_Sheet.png",
SKULL_FILEPATH[] = "Skull_a1.png";

constexpr GLint NUMBER_OF_TEXTURES = 1,
LEVEL_OF_DETAIL = 0,
TEXTURE_BORDER = 0;

constexpr int LEFT = 0,
RIGHT = 1,
UP = 2,
DOWN = 3;

constexpr int FONTBANK_SIZE_U = 16;
constexpr int FONTBANK_SIZE_V = 7;

int g_george_walking[SPRITESHEET_DIMENSIONS][SPRITESHEET_DIMENSIONS] =
{
    { 1, 5, 9,  13 }, // for Butterfly to move to the left,
    { 3, 7, 11, 15 }, // for Butterfly to move to the right,
    { 2, 6, 10, 14 }, // for Butterfly to move upwards,
    { 0, 4, 8,  12 }  // for Butterfly to move downwards
};

GLuint g_george_texture_id;
GLuint g_font_texture_id;
GLuint g_skull_texture_id;

float g_player_speed = 1.0f;  // move 1 unit per second

Entity* g_butterfly;
Entity* g_skull1;
Entity* g_skull2;
Entity* g_skull3;
std::vector<Entity*> g_bullets;

SDL_Window* g_display_window = nullptr;
AppStatus g_app_status = RUNNING;

ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;
float g_skull1_move_timer = 0.0f;
int g_skull1_direction = RIGHT;

bool g_game_over = false;
bool g_player_won = false;

GLuint load_texture(const char* filepath);
void initialise();
void process_input();
void update();
void render();
void shutdown();
bool is_nearby(glm::vec3 pos1, glm::vec3 pos2, float distance);
void remove_offscreen_bullets();
void move_skull2(Entity* skull, float delta_time, const Entity* butterfly);
void check_bullet_collisions();
void check_game_over();


GLuint load_texture(const char* filepath) {
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL) {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER,
        GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void draw_text(ShaderProgram* shader_program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE_U;
    float height = 1.0f / FONTBANK_SIZE_V;

    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    for (int i = 0; i < text.size(); i++) {
        int spritesheet_index = (int)text[i] - 18;

        float offset = (font_size + spacing) * i;

        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE_U) / FONTBANK_SIZE_U;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE_U) / FONTBANK_SIZE_V;

        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    shader_program->set_model_matrix(model_matrix);
    glUseProgram(shader_program->get_program_id());

    glVertexAttribPointer(shader_program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(shader_program->get_position_attribute());

    glVertexAttribPointer(shader_program->get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(shader_program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(shader_program->get_position_attribute());
    glDisableVertexAttribArray(shader_program->get_tex_coordinate_attribute());
}

void initialise() {
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Butterfly & Skulls Interaction",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr) {
        std::cerr << "Error: SDL window could not be created.\n";
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_george_texture_id = load_texture(SPRITESHEET_FILEPATH);
    g_font_texture_id = load_texture(FONTSHEET_FILEPATH);
    g_skull_texture_id = load_texture(SKULL_FILEPATH);

    // Initializing the butterfly entity
    g_butterfly = new Entity(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f), g_george_texture_id, 1.25f);
    g_butterfly->set_animation(g_george_walking[DOWN], SPRITESHEET_DIMENSIONS);

    // Initializing the first skull entity (square pattern movement)
    g_skull1 = new Entity(glm::vec3(-4.0f, -3.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f), g_skull_texture_id, 1.0f);

    // Initializing the second skull entity (turns at screen edges)
    g_skull2 = new Entity(glm::vec3(-4.5f, 3.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f), g_skull_texture_id, 1.0f);

    // Initializing the third skull entity (chases butterfly from the start)
    g_skull3 = new Entity(glm::vec3(4.0f, 3.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f), g_skull_texture_id, 1.5f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    if (g_game_over) return;  // Stop processing input if the game is over

    g_butterfly->set_animation(g_george_walking[DOWN], SPRITESHEET_DIMENSIONS);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            g_app_status = TERMINATED;
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);
    glm::vec3 direction(0.0f);

    if (keys[SDL_SCANCODE_LEFT]) {
        direction.x = -1.0f;
        g_butterfly->set_animation(g_george_walking[LEFT], SPRITESHEET_DIMENSIONS);
    }
    if (keys[SDL_SCANCODE_RIGHT]) {
        direction.x = 1.0f;
        g_butterfly->set_animation(g_george_walking[RIGHT], SPRITESHEET_DIMENSIONS);
    }
    if (keys[SDL_SCANCODE_UP]) {
        direction.y = 1.0f;
        g_butterfly->set_animation(g_george_walking[UP], SPRITESHEET_DIMENSIONS);
    }
    if (keys[SDL_SCANCODE_DOWN]) {
        direction.y = -1.0f;
        g_butterfly->set_animation(g_george_walking[DOWN], SPRITESHEET_DIMENSIONS);
    }

    g_butterfly->move(direction, FIXED_TIMESTEP);

    if (keys[SDL_SCANCODE_SPACE]) {
        // Fire a bullet
        GLuint bullet_texture_id = load_texture("platform.png");
        Entity* bullet = new Entity(g_butterfly->get_position() - glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.2f, 0.2f, 1.0f), glm::vec3(0.0f), bullet_texture_id, 2.0f);
        g_bullets.push_back(bullet);
    }
}

void update() {
    if (g_game_over) return;  // Stop updating if the game is over

    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;
    if (delta_time < FIXED_TIMESTEP) {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP) {
        // Updating bullets
        for (auto bullet : g_bullets) {
            bullet->move(glm::vec3(-1.0f, 0.0f, 0.0f), FIXED_TIMESTEP);
        }

        remove_offscreen_bullets();
        check_bullet_collisions();
        check_game_over();

        // Animate butterfly
        g_butterfly->animate(FIXED_TIMESTEP, SPRITESHEET_DIMENSIONS);

        if (g_skull1->is_active()) {
            // Moving first skull in a square pattern, it is unaffected by butterfly's prozximity
            g_skull1_move_timer += FIXED_TIMESTEP;
            if (g_skull1_move_timer >= 1.0f) { // Change direction every 1 second
                g_skull1_move_timer = 0.0f;
                g_skull1_direction = (g_skull1_direction + 1) % 4; // Move to the next direction
            }

            glm::vec3 skull1_direction(0.0f);
            switch (g_skull1_direction) {
            case RIGHT:
                skull1_direction.x = 1.0f;
                break;
            case UP:
                skull1_direction.y = 1.0f;
                break;
            case LEFT:
                skull1_direction.x = -1.0f;
                break;
            case DOWN:
                skull1_direction.y = -1.0f;
                break;
            }
            g_skull1->move(skull1_direction, FIXED_TIMESTEP);
        }

        // Move second skull (turns at screen edges and chases butterfly when close)
        if (g_skull2->is_active()) {
            move_skull2(g_skull2, FIXED_TIMESTEP, g_butterfly);
        }

        // Move third skull (chases butterfly automatically)
        if (g_skull3->is_active()) {
            glm::vec3 direction_to_butterfly = g_butterfly->get_position() - g_skull3->get_position();
            direction_to_butterfly = glm::normalize(direction_to_butterfly);
            g_skull3->move(direction_to_butterfly, FIXED_TIMESTEP);
        }

        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Render butterfly
    g_butterfly->render(&g_shader_program);

    // Render skulls
    g_skull1->render(&g_shader_program);
    g_skull2->render(&g_shader_program);
    g_skull3->render(&g_shader_program);

    // Render bullets
    for (auto bullet : g_bullets) {
        bullet->render(&g_shader_program);
    }

    // Display win/lose message if game is over
    if (g_game_over) {
        std::string message = g_player_won ? "You Win" : "You Lose";
        draw_text(&g_shader_program, g_font_texture_id, message, 1.0f, 0.05f, glm::vec3(-4.0f, 0.0f, 0.0f));
    }

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() {
    SDL_Quit();

    // claening up memory
    delete g_butterfly;
    delete g_skull1;
    delete g_skull2;
    delete g_skull3;
    for (auto bullet : g_bullets) {
        delete bullet;
    }
}

bool is_nearby(glm::vec3 pos1, glm::vec3 pos2, float distance) {
    return glm::length(pos1 - pos2) < distance;
}

void remove_offscreen_bullets() {
    g_bullets.erase(
        std::remove_if(
            g_bullets.begin(),
            g_bullets.end(),
            [](Entity* bullet) {
                glm::vec3 position = bullet->get_position();
                if (position.x < -5.0f || position.x > 5.0f || position.y < -3.75f || position.y > 3.75f) {
                    delete bullet; // Cleaning up memory
                    return true;
                }
                return false;
            }),
        g_bullets.end());
}

void move_skull2(Entity* skull, float delta_time, const Entity* butterfly) {
    glm::vec3 position = skull->get_position();
    static glm::vec3 movement_direction(0.0f, -1.0f, 0.0f);  // Start by moving downwards

    // Boundary values for the screen
    const float minX = -5.0f;
    const float maxX = 5.0f;
    const float minY = -3.75f;
    const float maxY = 3.75f;

    if (is_nearby(position, butterfly->get_position(), 1.5f)) {
        // Chase the butterfly when close
        glm::vec3 direction_to_butterfly = butterfly->get_position() - position;
        movement_direction = glm::normalize(direction_to_butterfly);
    }
    else {
        // Move up and down if not near the butterfly
        if (position.y <= minY || position.y >= maxY) {
            movement_direction.y *= -1.0f; // Reverse vertical direction
        }
    }

    glm::vec3 new_position = position + movement_direction * skull->get_speed() * delta_time;

    // Clamping the position to ensure it stays within the screen boundaries
    new_position.x = glm::clamp(new_position.x, minX, maxX);
    new_position.y = glm::clamp(new_position.y, minY, maxY);

    skull->set_position(new_position);
}

void check_bullet_collisions() {
    for (auto bullet : g_bullets) {
        glm::vec3 bullet_position = bullet->get_position();

        if (g_skull1->is_active() && is_nearby(bullet_position, g_skull1->get_position(), 0.5f)) {
            g_skull1->set_active(false);
        }
        if (g_skull2->is_active() && is_nearby(bullet_position, g_skull2->get_position(), 0.5f)) {
            g_skull2->set_active(false);
        }
        if (g_skull3->is_active() && is_nearby(bullet_position, g_skull3->get_position(), 0.5f)) {
            g_skull3->set_active(false);
        }
    }
}

void check_game_over() {
    if (!g_skull1->is_active() && !g_skull2->is_active() && !g_skull3->is_active()) {
        g_game_over = true;
        g_player_won = true;
    }

    if (!g_game_over && (g_skull1->is_active() && is_nearby(g_butterfly->get_position(), g_skull1->get_position(), 0.5f)) ||
        (g_skull2->is_active() && is_nearby(g_butterfly->get_position(), g_skull2->get_position(), 0.5f)) ||
        (g_skull3->is_active() && is_nearby(g_butterfly->get_position(), g_skull3->get_position(), 0.5f))) {
        g_game_over = true;
        g_player_won = false;
        g_butterfly->set_active(false);  // Stop the butterfly from moving
    }
}

int main(int argc, char* argv[]) {
    initialise();

    while (g_app_status == RUNNING) {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
