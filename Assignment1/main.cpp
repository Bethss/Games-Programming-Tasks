/**
* Author: Elizabeth Akindeko
* Assignment: Simple 2D Scene
* Date due: 2024-06-15, 11:59pm
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

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
LEVEL_OF_DETAIL = 0, // mipmap reduction image level
TEXTURE_BORDER = 0; // this value MUST be zero

// New sprite filepaths
constexpr char BUTTERFLY_A1_SPRITE_FILEPATH[] = "butterfly_a1.png",
ROSE_A1_SPRITE_FILEPATH[] = "rose_a1.png";

constexpr glm::vec3 INIT_SCALE = glm::vec3(1.0f, 1.0f, 0.0f),
INIT_POS_BUTTERFLY_A1 = glm::vec3(3.0f, 3.0f, 0.0f),
INIT_POS_ROSE_A1 = glm::vec3(-2.0f, -2.5f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
g_butterfly_a1_matrix,
g_rose_a1_matrix,
g_projection_matrix;

glm::vec3 g_rotation_butterfly_a1 = glm::vec3(0.0f, 0.0f, 0.0f),
g_rotation_rose_a1 = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_butterfly_a1_texture_id,
g_rose_a1_texture_id;


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Butterfly Chases Rose!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_butterfly_a1_matrix = glm::mat4(1.0f);
    g_rose_a1_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_butterfly_a1_texture_id = load_texture(BUTTERFLY_A1_SPRITE_FILEPATH);
    g_rose_a1_texture_id = load_texture(ROSE_A1_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


// ——————————— GLOBAL VARS AND CONSTS FOR TRANSFORMATIONS ——————————— //

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;


constexpr int MAX_FRAME = 40;
int  g_frame_counter = 0;
bool g_is_growing = true;

float speed = 1.0f;
int direction = 0;
float distance = 0.0f;

float g_scale_factor = 1.01f;
float g_shrink_factor = 0.99f;

// Global variable to track the rose's current position
glm::vec3 g_current_pos_rose_a1 = INIT_POS_ROSE_A1;

glm::mat4 scaled_rose_matrix = glm::mat4(1.0f);


// —————————————————————————————————————————————————————————————————— //
void update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP) {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP) {

        /* Game logic */

        // Rose Rotation
        g_rotation_rose_a1.y += ROT_INCREMENT * delta_time;

        // Rose Movement in a Square Pattern
        distance += speed * delta_time;

        glm::vec3 rose_translation = glm::vec3(0.0f);

        if (direction == 0) {
            rose_translation.x += speed * delta_time;
        }
        else if (direction == 1) {
            rose_translation.y += speed * delta_time;
        }
        else if (direction == 2) {
            rose_translation.x -= speed * delta_time;
        }
        else if (direction == 3) {
            rose_translation.y -= speed * delta_time;
        }

        if (distance >= 5.0f) {
            direction = (direction + 1) % 4;
            distance = 0.0f;
        }

        // Updating the current position of the rose
        g_current_pos_rose_a1 += rose_translation;

        // Applying rose transformations

        glm::vec3 g_scale_vector;
        g_frame_counter += 1;

        if (g_frame_counter >= MAX_FRAME) {
            g_is_growing = !g_is_growing;
            g_frame_counter = 0;
        }

        g_scale_vector = glm::vec3(g_is_growing ? g_scale_factor : g_shrink_factor,
            g_is_growing ? g_scale_factor : g_shrink_factor,
            1.0f);

        g_rose_a1_matrix = glm::mat4(1.0f); // Reseting the rose matrix first
        g_rose_a1_matrix = glm::translate(g_rose_a1_matrix, g_current_pos_rose_a1);
        g_rose_a1_matrix = glm::rotate(g_rose_a1_matrix, g_rotation_rose_a1.y, glm::vec3(0.0f, 1.0f, 0.0f));


        scaled_rose_matrix = glm::scale(scaled_rose_matrix, g_scale_vector); // Applying scaling to the global matrix
        g_rose_a1_matrix *= scaled_rose_matrix; // Applying the accumulated scaling transformations

        // Butterfly Movement (Chasing the Rose)
        glm::vec3 rose_position = glm::vec3(g_rose_a1_matrix[3]);
        glm::vec3 butterfly_position = glm::vec3(g_butterfly_a1_matrix[3]);
        glm::vec3 direction_vector = rose_position - butterfly_position;

        g_butterfly_a1_matrix = glm::mat4(1.0f); // Resetting the butterfly matrix first
        g_butterfly_a1_matrix = glm::translate(g_butterfly_a1_matrix, butterfly_position + direction_vector * speed * delta_time);

        delta_time -= FIXED_TIMESTEP;
    }
    g_accumulator = delta_time;
}



void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

void render()
    {
        glClear(GL_COLOR_BUFFER_BIT);

        // Vertices
        float vertices[] =
        {
            -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
            -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
        };

        // Textures
        float texture_coordinates[] =
        {
            0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
            0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
        };

        glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(g_shader_program.get_position_attribute());

        glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
        glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

        // Bind texture
        draw_object(g_butterfly_a1_matrix, g_butterfly_a1_texture_id);
        draw_object(g_rose_a1_matrix, g_rose_a1_texture_id);

        // We disable two attribute arrays now
        glDisableVertexAttribArray(g_shader_program.get_position_attribute());
        glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

        SDL_GL_SwapWindow(g_display_window);
    }

void shutdown() { SDL_Quit(); }


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