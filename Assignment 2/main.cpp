/**
* Author: Elizabeth Akindeko
* Assignment: Pong Clone
* Date due: 2024-06-29, 11:59pm
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
struct GameState
{
    std::vector<Entity*> balls; // Multiple balls
    Entity* paddle1;
    Entity* paddle2;
};

enum AppStatus { RUNNING, TERMINATED };
// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH = 840,
WINDOW_HEIGHT = 680;

constexpr float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0f;
constexpr char PADDLE_FILEPATH[] = "Pong_Sweet_White_Tail.png";
constexpr char BALL_FILEPATH[] = "Pong_Candy.png";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;


constexpr int FONTBANK_SIZE_U = 16;
constexpr int FONTBANK_SIZE_V = 7;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_game_state;
int g_desired_ball_count = 1;  // Starting with one ball


SDL_Window* g_display_window;
AppStatus g_app_status = TERMINATED;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

bool g_game_over = false;
std::string g_endgame_message = "";


// Texture ID for the font
GLuint FONT_TEXTURE_ID;

void initialise();
void process_input();
void update();
void render();
void shutdown();
GLuint load_texture(const char* filepath);

// Function to draw text

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

void add_ball() {
    GLuint ball_texture_id = load_texture(BALL_FILEPATH);
    Entity* ball = new Entity(ball_texture_id, 2.0f, 0.5f, 0.5f, BALL);
    ball->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    ball->set_velocity(glm::vec3(-1.0f, 1.0f, 0.0f)); // Adjust the speed as needed
    g_game_state.balls.push_back(ball);
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Pong Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    g_app_status = RUNNING;

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, WINDOW_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // Loading textures
    GLuint paddle_texture_id = load_texture(PADDLE_FILEPATH);
    GLuint ball_texture_id = load_texture(BALL_FILEPATH);
    FONT_TEXTURE_ID = load_texture("MisterF_Fonts_Sprite_Sheet.png");  // Loading font texture

    // Initializing paddles
    g_game_state.paddle1 = new Entity(paddle_texture_id, 2.0f, 0.5f, 1.5f, PADDLE);
    g_game_state.paddle2 = new Entity(paddle_texture_id, 2.0f, 0.5f, 1.5f, PADDLE);
    g_game_state.paddle1->set_position(glm::vec3(-4.5f, 0.0f, 0.0f));
    g_game_state.paddle2->set_position(glm::vec3(4.5f, 0.0f, 0.0f));

    // Pre-creating three balls but only activating the first one initially
    for (int i = 0; i < 3; i++) {
        Entity* ball = new Entity(ball_texture_id, 2.0f, 0.5f, 0.5f, BALL);
        ball->set_position(glm::vec3(0.0f, 0.0f, 0.0f)); // Center the ball at the start
        ball->set_velocity(glm::vec3((i % 2 == 0 ? 1.0f : -1.0f), 0.5f, 0.0f)); // Velocity
        ball->set_active(i == 0); // Only the first ball is active initially
        g_game_state.balls.push_back(ball);
    }

    // Setting the desired ball count to 1 initially
    g_desired_ball_count = 1;

    // Ensuring only the first ball is active
    for (size_t i = g_desired_ball_count; i < g_game_state.balls.size(); ++i) {
        g_game_state.balls[i]->set_position(glm::vec3(-100.0f, -100.0f, 0.0f));
        g_game_state.balls[i]->set_velocity(glm::vec3(0.0f, 0.0f, 0.0f));
    }

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
        else if (event.type == SDL_KEYDOWN)
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_q:
                g_app_status = TERMINATED;
                break;
            case SDLK_t:
                if (!g_game_over) {
                    g_game_state.paddle2->toggle_ai_control();
                }
                break;
            case SDLK_1:
                if (!g_game_over) {
                    g_desired_ball_count = 1;  // Set desired ball count
                }
                break;
            case SDLK_2:
                if (!g_game_over) {
                    g_desired_ball_count = 2;
                }
                break;
            case SDLK_3:
                if (!g_game_over) {
                    g_desired_ball_count = 3;
                }
                break;
            default:
                break;
            }
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (!g_game_over) {
        if (key_state[SDL_SCANCODE_W]) g_game_state.paddle1->move_up();
        if (key_state[SDL_SCANCODE_S]) g_game_state.paddle1->move_down();
        if (!g_game_state.paddle2->is_ai_controlled())
        {
            if (key_state[SDL_SCANCODE_UP]) g_game_state.paddle2->move_up();
            if (key_state[SDL_SCANCODE_DOWN]) g_game_state.paddle2->move_down();
        }
    }
}

void check_ball_collision() {
    for (Entity* ball : g_game_state.balls) {
        if (!ball->is_active()) continue; // Skip inactive balls

        // Checking for collision with paddles
        if (ball->check_collision(g_game_state.paddle1) || ball->check_collision(g_game_state.paddle2)) {
            ball->set_velocity(glm::vec3(-ball->get_velocity().x, ball->get_velocity().y, 0.0f));
        }

        // Checking for collision with the top and bottom of the screen
        if (ball->get_position().y + (ball->get_height() / 2) > 3.75f ||
            ball->get_position().y - (ball->get_height() / 2) < -3.75f) {
            ball->set_velocity(glm::vec3(ball->get_velocity().x, -ball->get_velocity().y, 0.0f));
        }

        // Checking for collision with the left and right of the screen (endgame condition)
        if (ball->get_position().x < -5.0f) {
            g_game_over = true;
            g_endgame_message = "Player 2 Wins!";
            return;  // Exit function early if game is over
        }
        else if (ball->get_position().x > 5.0f) {
            g_game_over = true;
            g_endgame_message = "Player 1 Wins!";
            return;  // Exit function early if game is over
        }
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        // Enable or disable balls based on the desired count
        for (int i = 0; i < 3; ++i) {
            if (i < g_desired_ball_count) {
                // Reseting the ball's position and velocity before activating it
                if (!g_game_state.balls[i]->is_active()) {
                    g_game_state.balls[i]->set_position(glm::vec3(0.0f, i + -2.0f, 0.0f)); // Center the ball
                    g_game_state.balls[i]->set_velocity(glm::vec3((i % 2 == 0 ? 1.0f : -1.0f), 0.5f, 0.0f)); // Give it an initial velocity
                    g_game_state.balls[i]->set_active(true); // Activate the ball
                }
                g_game_state.balls[i]->update(FIXED_TIMESTEP, NULL, 0); // Update active balls
            }
            else {
                g_game_state.balls[i]->set_active(false); // Deactivate the ball
            }
        }

        g_game_state.paddle1->update(FIXED_TIMESTEP, NULL, 0);
        g_game_state.paddle2->update(FIXED_TIMESTEP, NULL, 0);

        check_ball_collision();

        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < g_desired_ball_count; ++i) {
        g_game_state.balls[i]->render(&g_shader_program);
    }
    g_game_state.paddle1->render(&g_shader_program);
    g_game_state.paddle2->render(&g_shader_program);

    if (g_game_over) {
        draw_text(&g_shader_program, FONT_TEXTURE_ID, g_endgame_message, 0.5f, -0.25f, glm::vec3(-2.0f, 0.0f, 0.0f));
    }

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown()
{
    SDL_Quit();

    delete g_game_state.paddle1;
    delete g_game_state.paddle2;

    for (Entity* ball : g_game_state.balls) {
        delete ball;
    }
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
