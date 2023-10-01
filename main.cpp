#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

glm::mat4 init_mat;

bool look_right = true;
bool has_fired = false;
const float radius = 1.0f;
const float firerate = 75.0f;
const float speed = 0.01f;
float current_time = 0.0f;

float x_pos_ball = 0.0f;
float y_pos_ball = 0.0f;
float g_firetime = 0.0f;
float g_bombsize = 1.0f;
float bomb_angle = 0.0f;

float x_pos2 = radius;
float y_pos2 = 0.0f;
float g_angle = 0.0f;

const int WINDOW_WIDTH = 1080,
WINDOW_HEIGHT = 720;

const float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float DEGREES_PER_SECOND = 90.0f;

const int NUMBER_OF_TEXTURES = 1; 
const GLint LEVEL_OF_DETAIL = 0;  
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

const char PLAYER_SPRITE_FILEPATH[] = "assets/cannon.png";

const char PLAYER_SPRITE_FILEPATH2[] = "assets/yuuka trans.png";

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 view_matrix, m_model_matrix, m_projection_matrix, m_trans_matrix;

glm::mat4 view_matrix2, m_model_matrix2, m_projection_matrix2, m_trans_matrix2;

float m_previous_ticks = 0.0f;

GLuint g_player_texture_id;
GLuint g_player_texture_id2;


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_game_is_running = false;
        }
    }
}

GLuint load_texture(const char* filepath)
{

    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        LOG(filepath);
        assert(false);
    }


    GLuint texture_id;
    glGenTextures(NUMBER_OF_TEXTURES, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    stbi_image_free(image);

    return texture_id;
}

void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);


    g_display_window = SDL_CreateWindow("I have two cheeses",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);


    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    m_model_matrix = glm::mat4(1.0f);
    view_matrix = glm::mat4(1.0f);  
    m_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f); 
    g_shader_program.set_projection_matrix(m_projection_matrix);
    g_shader_program.set_view_matrix(view_matrix);


    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);

 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_model_matrix2 = glm::mat4(1.0f);
    view_matrix2 = glm::mat4(1.0f); 
    m_projection_matrix2 = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f); 

    g_shader_program.set_projection_matrix(m_projection_matrix2);
    g_shader_program.set_view_matrix(view_matrix2);


    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_player_texture_id2 = load_texture(PLAYER_SPRITE_FILEPATH2);

 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_model_matrix2 = glm::translate(m_model_matrix2, glm::vec3(-1.5f, 0.0f, 0.0f));
    m_model_matrix2 = glm::rotate(m_model_matrix2, glm::radians(15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    m_model_matrix2 = glm::scale(m_model_matrix2, glm::vec3(2.0f, 2.0f, 0.0f));
    init_mat = m_model_matrix2;
}

void update()
{
    glClear;

    /*
    For some reason, whenever I use ticks, my update gets exponentially faster. Are my ticks increasing over each update?? 
    
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - m_previous_ticks; // the delta time is the difference from the last frame
    m_previous_ticks = ticks;
    */
    

    current_time++;

    if (current_time >= firerate) {
        has_fired = !has_fired;
        current_time = 0.0f;
    }

    g_angle += speed;

    x_pos2 = radius * glm::cos(g_angle);
    y_pos2 = radius * glm::sin(g_angle);

    m_model_matrix2 = init_mat;
    m_model_matrix2 = glm::translate(m_model_matrix2, glm::vec3(x_pos2, y_pos2, 0.0f));

    m_model_matrix = m_model_matrix2;
    m_model_matrix = glm::scale(m_model_matrix, glm::vec3(0.5f, 0.5f, 0.0f));
    m_model_matrix = glm::rotate(m_model_matrix, glm::radians(-15.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    if (has_fired) {
        x_pos_ball = g_firetime * 10.0f;
        y_pos_ball = (-40.0f * (g_firetime - 0.2f) * (g_firetime - 0.2f)) + 2.0f;
        m_model_matrix = glm::translate(m_model_matrix, glm::vec3(x_pos_ball, y_pos_ball, 0.0f));
        m_model_matrix = glm::scale(m_model_matrix, glm::vec3(g_bombsize, g_bombsize, 1.0f));
        m_model_matrix = glm::rotate(m_model_matrix, bomb_angle, glm::vec3(0.0f, 0.0f, 1.0f));

        bomb_angle -= 0.2f;
        g_bombsize += 0.05f;
        g_firetime += 0.01f;
    }
    else {
        x_pos_ball = 0.0f;
        y_pos_ball = 0.0f;
        g_firetime = 0.0f;
        g_bombsize = 1.0f;
        bomb_angle = 0.0f;
    }

    SDL_GL_SwapWindow;

}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); 
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);


    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f  
    };


    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,    
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,    
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());


    draw_object(m_model_matrix, g_player_texture_id);
    draw_object(m_model_matrix2, g_player_texture_id2);


    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
}


int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}