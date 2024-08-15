/**
* Author: Elizabeth Akindeko
* Assignment: Lunar Lander
* Date due: 2024-07-13, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
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
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState {
    Entity* rocket;
    Entity* platform;
    Entity* mountain;
    int score;
    float altitude;
    float fuel;
    float horizontal_speed;
    float vertical_speed;
};


enum AppStatus { RUNNING, TERMINATED };

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH = 840,
WINDOW_HEIGHT = 680;

constexpr float BG_RED = 0.0f,
BG_BLUE = 0.0f,
BG_GREEN = 0.0f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0f;
constexpr char ROCKET_FILEPATH[] = "Lunar_Landar_Rocket.png";
constexpr char MOUNTAIN_FILEPATH[] = "Lunar_Landar_Mountain.png";
constexpr char PLATFORM_FILEPATH[] = "platform.png";
constexpr char FIRE_FILEPATH[] = "Lunar_Landar_Rocket_Fire.png";
constexpr char FONTSHEET_FILEPATH[] = "LLPixel_Fonts_Sprite_Sheet.png";
constexpr char EXPLOSION_FILEPATH[] = "Lunar_Landar_Explosion.png";


constexpr int FONTBANK_SIZE_U = 16;
constexpr int FONTBANK_SIZE_V = 7;


// ––––– GLOBAL VARIABLES ––––– //
GameState g_game_state;
SDL_Window* g_display_window;
AppStatus g_app_status = TERMINATED;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;
float INITIAL_FUEL = 1000.0f;

GLuint FONT_TEXTURE_ID;

void initialise();
void process_input();
void update();
void render();
void shutdown();
GLuint load_texture(const char* filepath);


void draw_text(ShaderProgram* shader_program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE_U;
    float height = 1.0f / FONTBANK_SIZE_V;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        // int spritesheet_index = (int)text[i];  // ascii value of character
        int spritesheet_index = (int)text[i] - 18;  // Adjusted for the correct offset to match the ASCII starting at '!'

        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE_U) / FONTBANK_SIZE_U;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE_U) / FONTBANK_SIZE_V;



        // 3. Inset the current pair in both vectors
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

    // 4. And render all of them using the pairs
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


GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components,
        STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

// Function definitions
void initialise() {
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Lunar Lander", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
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

    GLuint rocket_texture_id = load_texture(ROCKET_FILEPATH);
    GLuint mountain_texture_id = load_texture(MOUNTAIN_FILEPATH);
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
    GLuint fire_texture_id = load_texture(FIRE_FILEPATH);
    FONT_TEXTURE_ID = load_texture(FONTSHEET_FILEPATH);

    // Initializing entities with positions within the viewport
    g_game_state.rocket = new Entity(rocket_texture_id, glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 1.0f), true, true);
    g_game_state.mountain = new Entity(mountain_texture_id, glm::vec3(0.0f, -3.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.0f, 1.5f, 1.0f), false, true); // Ensure active = true
    g_game_state.platform = new Entity(platform_texture_id, glm::vec3(0.0f, -2.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.2f, 1.0f), false, true);

    g_game_state.rocket->set_fire_texture(fire_texture_id);
    GLuint explosion_texture_id = load_texture(EXPLOSION_FILEPATH);
    g_game_state.rocket->set_explosion_texture(explosion_texture_id);


    g_game_state.fuel = INITIAL_FUEL;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_app_status = RUNNING;
}


void process_input() {
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    glm::vec3 acceleration(0.0f);

    if (keys[SDL_SCANCODE_LEFT] && g_game_state.fuel > 0) {
        acceleration.x = -0.1f;
        g_game_state.fuel -= 10.0f * FIXED_TIMESTEP;
    }
    if (keys[SDL_SCANCODE_RIGHT] && g_game_state.fuel > 0) {
        acceleration.x = 0.1f;
        g_game_state.fuel -= 10.0f * FIXED_TIMESTEP;
    }
    if (keys[SDL_SCANCODE_UP] && g_game_state.fuel > 0) {
        acceleration.y = 0.2f;
        g_game_state.fuel -= 10.0f * FIXED_TIMESTEP;
    }

    g_game_state.rocket->set_acceleration(acceleration);
}

void update() {
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;
    if (delta_time < FIXED_TIMESTEP) {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP) {
        glm::vec3 gravity(0.0f, -0.001f, 0.0f);
        g_game_state.rocket->set_acceleration(g_game_state.rocket->get_acceleration() + gravity);
        g_game_state.rocket->update(FIXED_TIMESTEP);

        // Updating live stats based on the rocket's state
        g_game_state.altitude = g_game_state.rocket->get_position().y;
        g_game_state.horizontal_speed = g_game_state.rocket->get_velocity().x;
        g_game_state.vertical_speed = g_game_state.rocket->get_velocity().y;

        // Collision detection
        float x_distance = fabs(g_game_state.rocket->get_position().x - g_game_state.platform->get_position().x);
        float y_distance = fabs(g_game_state.rocket->get_position().y - g_game_state.platform->get_position().y);

        bool crashed = false;

        if (x_distance < 0.5f && y_distance < 0.5f) {
            // Checking if vertical speed is too high for a safe landing
            if (fabs(g_game_state.vertical_speed) > 30.0f) {
                crashed = true;
            }
        }

        if (crashed) {
            g_game_state.rocket->set_texture_id(g_game_state.rocket->get_explosion_texture_id());
            LOG("CRASH! Rocket exploded.");
            g_app_status = TERMINATED;  // End
        }

        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Rendering game entities
    g_game_state.mountain->render(&g_shader_program);
    g_game_state.platform->render(&g_shader_program);
    g_game_state.rocket->render(&g_shader_program);

    // Converting values to strings
    std::string altitude_text = "ALTITUDE: " + std::to_string(static_cast<int>(g_game_state.altitude));
    std::string fuel_text = "FUEL: " + std::to_string(static_cast<int>(g_game_state.fuel));
    std::string horizontal_speed_text = "HORIZONTAL SPEED: " + std::to_string(static_cast<int>(g_game_state.horizontal_speed));
    std::string vertical_speed_text = "VERTICAL SPEED: " + std::to_string(static_cast<int>(g_game_state.vertical_speed));

    // Rendering the text
    draw_text(&g_shader_program, FONT_TEXTURE_ID, altitude_text, 0.25f, 0.005f, glm::vec3(-4.5f, 3.0f, 0.0f));
    draw_text(&g_shader_program, FONT_TEXTURE_ID, fuel_text, 0.25f, 0.005f, glm::vec3(-4.5f, 2.5f, 0.0f));
    draw_text(&g_shader_program, FONT_TEXTURE_ID, horizontal_speed_text, 0.25f, 0.005f, glm::vec3(0.0f, 3.0f, 0.0f));
    draw_text(&g_shader_program, FONT_TEXTURE_ID, vertical_speed_text, 0.25f, 0.005f, glm::vec3(0.0f, 2.5f, 0.0f));

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown()
{
    SDL_Quit();

    delete g_game_state.rocket;
    delete g_game_state.mountain;
    delete g_game_state.platform;
}

int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
