#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "game_map.h"

enum GameState {
    GAME_ACTIVE, GAME_PAUSED, GAME_OVER
};

const unsigned int SCREEN_WIDTH = 960;
const unsigned int SCREEN_HEIGHT = 800;
const glm::vec2 PLAYER_SIZE(100.0f, 40.0f);
const float PLAYER_VELOCITY = 500.0f;
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -400.0f);

typedef struct {
    GameState state = GAME_ACTIVE;
    bool keys[1024] = {false};
    canvas canvas;
    std::vector<GameMap> Maps;
    std::map<std::string, GLuint> Shaders;
    std::map<std::string, texture2D> Textures;
    std::map<std::string, GameObject> Objects;
    SpriteRenderer renderer;
    unsigned int activeMap = 0;
} Game;

GameMap initGameObjects(
    std::vector<std::vector<std::array<unsigned int, 3>>> tileData,
    std::vector<std::string> texture_keys,
    std::map<std::string, texture2D> textures,
    canvas gameCanvas
) {
    // tileData is = [texture_id, sprite_id, layer + 1]
    GameMap map = {};
    // Calculate tile dimensions
    map.size[1] = tileData.size() / 3;
    map.size[0] = tileData[0].size();
    map.tileSize = glm::vec2(gameCanvas.width / static_cast<float>(map.size[0]), gameCanvas.height / static_cast<float>(map.size[1]));
    for (int i = 0; i < tileData.size(); i++) {
        for (int j = 0; j < map.size[0]; j++) {
            // If texture_id equals 0 ignore
            if (tileData[i][j][0] == 0) continue;
            GameObject obj = {
                .position = glm::vec2(map.tileSize.x * j, map.tileSize.y * i - gameCanvas.height * ((int)tileData[i][j][2] - 1)),
                .size = glm::vec2(map.tileSize.x, map.tileSize.y),
                .spriteOffset = glm::vec2(tileData[i][j][1] % (textures[texture_keys[tileData[i][j][0] - 1]].width / 32) * 32, tileData[i][j][1] / (textures[texture_keys[tileData[i][j][0] - 1]].height / 32) * 32),
                .isBreakable = tileData[i][j][2] == 2,
                .texture = textures[texture_keys[tileData[i][j][0] - 1]]
            };
            map.tiles.push_back(obj);
        }
    }

    return map;
}

Game initGame(bool fullWindow) {
    Game game;
    if (!fullWindow)
        game.canvas = initFramebuffer(768, 512);
    else {
        game.canvas.width = SCREEN_WIDTH;
        game.canvas.height = SCREEN_HEIGHT;
    }
    GLuint sprite_shader_id = loadShaderFromFile(".\\shaders\\sprite.vs", ".\\shaders\\sprite.frag", nullptr);
    game.Shaders["sprite"] = sprite_shader_id;
    glm::mat4 projection = glm::ortho(0.0f, game.canvas.width, game.canvas.height, 0.0f, -1.0f, 1.0f);
    setShaderInt(sprite_shader_id, "image", 0, true);
    setShaderMatrix4(sprite_shader_id, "projection", projection);
    game.renderer.shader = sprite_shader_id;
    game.renderer.VAO = initRenderSprite();

    // load maps
    // GameMap map0 = loadMap(".\\maps\\map0.sgem", game.Textures, game.canvas, initGameObjects);
    GameMap map1 = loadMap(".\\maps\\map1.sgem", game.Textures, game.canvas, initGameObjects);
    // game.Maps.push_back(map0);
    game.Maps.push_back(map1);
    glm::vec2 playerPos = glm::vec2(game.canvas.width / 2 - PLAYER_SIZE.x / 2, game.canvas.height - PLAYER_SIZE.y);
    GameObject player = {
        .position = playerPos, .size = PLAYER_SIZE, .spriteOffset = glm::vec2(64.0f, 144.0f), .spriteSize = glm::vec2(32.0f, 16.0f),
        .texture=game.Textures[".\\textures\\18-Initial_TileSet.png"]
    };
    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - 10.0f, 0.0f);
    GameObject ball = {
        .position = ballPos, .size = glm::vec2(20.0f, 20.0f), .spriteOffset = glm::vec2(64.0f, 96.0f),
        .spriteSize = glm::vec2(12.0f, 12.0f), .velocity = INITIAL_BALL_VELOCITY, .collisionType = CIRCULAR, .disabled = true,
        .texture=game.Textures[".\\textures\\18-Initial_TileSet.png"]
    };
    game.Objects["player"] = player;
    game.Objects["ball"] = ball;

    return game;
}

void proccessInputs(Game &game, float dt) {
    if (game.state != GAME_ACTIVE) return;

    float velocity = PLAYER_VELOCITY * dt;
    if (game.keys[GLFW_KEY_A] || game.keys[GLFW_KEY_LEFT]) {
        if (game.Objects["player"].position.x >= 0.0f) 
        {
            game.Objects["player"].position.x -= velocity;
            if (game.Objects["ball"].disabled) game.Objects["ball"].position.x -= velocity;
        };
    }
    if (game.keys[GLFW_KEY_D] || game.keys[GLFW_KEY_RIGHT]) {
        if (game.Objects["player"].position.x <= (game.canvas.width - game.Objects["player"].size.x - 0.0f)) {
            game.Objects["player"].position.x += velocity;
            if (game.Objects["ball"].disabled) game.Objects["ball"].position.x += velocity;
        };
    }
    if (game.keys[GLFW_KEY_SPACE]) game.Objects["ball"].disabled = false;
}

glm::vec2 ballMovement(GameObject &ball, float dt, unsigned int window_width) {
    if (!ball.disabled) {
        ball.position += ball.velocity * dt;
        if (ball.position.x <= 0.0f) {
            ball.velocity.x = -ball.velocity.x;
            ball.position.x = 0.0f;
        } else if (ball.position.x + ball.size.x >= window_width - 0.0f) {
            ball.velocity.x = -ball.velocity.x;
            ball.position.x = window_width - 0.0f - ball.size.x;
        }
        if (ball.position.y <= 0.0f) {
            ball.velocity.y = -ball.velocity.y;
            ball.position.y = 0.0f;
        }
    }

    return ball.position;
}

void ballReset(GameObject &ball, glm::vec2 position, glm::vec2 velocity) {
    ball.position = position;
    ball.velocity = velocity;
    ball.disabled = true;
}

void ballCollision(Game &game) {
    // handling tile collision
    for (GameObject &tile : game.Maps[game.activeMap].tiles) {
        if (tile.disabled || !tile.isBreakable) continue;
        Collision collision = checkCollision(game.Objects["ball"], tile);

        if (std::get<0>(collision)) {
             tile.disabled = true;

            Direction dir = std::get<1>(collision);
            glm::vec2 diff_vector = std::get<2>(collision);
            if (dir == LEFT || dir == RIGHT) {
                game.Objects["ball"].velocity.x = -game.Objects["ball"].velocity.x;
                float penetration = game.Objects["ball"].size.x * 0.5f - std::abs(diff_vector.x);
                if (dir == LEFT)
                    game.Objects["ball"].position.x += penetration;
                else
                    game.Objects["ball"].position.x -= penetration;
            } else {
                game.Objects["ball"].velocity.y = -game.Objects["ball"].velocity.y;
                float penetration = game.Objects["ball"].size.x * 0.5f - std::abs(diff_vector.y);
                if (dir == DOWN)
                    game.Objects["ball"].position.y += penetration;
                else
                    game.Objects["ball"].position.y -= penetration;
            }
        }
    }

    // handling player collision
    Collision result = checkCollision(game.Objects["ball"], game.Objects["player"]);
    if (!game.Objects["ball"].disabled && std::get<0>(result)) {
        float centerBoard = game.Objects["player"].position.x + game.Objects["player"].size.x * 0.5f;
        float distance = (game.Objects["ball"].position.x + game.Objects["ball"].size.x * 0.5f) - centerBoard;
        float percentage = distance / (game.Objects["player"].size.x * 0.5f);

        float strength = 2.0f;
        glm::vec2 oldVelocity = game.Objects["ball"].velocity;
        game.Objects["ball"].velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        game.Objects["ball"].velocity.y = -game.Objects["ball"].velocity.y;
        game.Objects["ball"].velocity.y = -1.0f * abs(game.Objects["ball"].velocity.y);
        game.Objects["ball"].velocity = glm::normalize(game.Objects["ball"].velocity) * glm::length(oldVelocity);
    }
}

void updateGame(Game &game, float dt) {
    ballMovement(game.Objects["ball"], dt, game.canvas.width);

    ballCollision(game);

    if (game.Objects["ball"].position.y >= game.canvas.height) {
        game.Objects["player"].position = glm::vec2(game.canvas.width / 2 - PLAYER_SIZE.x / 2, game.canvas.height - PLAYER_SIZE.y);
        game.Objects["ball"].position = game.Objects["player"].position + glm::vec2(PLAYER_SIZE.x / 2.0f - 10.0f, 0.0f);
        game.Objects["ball"].disabled = true;
    }
}

void renderGame(Game game) {
    // if (game.canvas.fbo) glBindFramebuffer(GL_FRAMEBUFFER, game.canvas.fbo);
    drawMap(game.renderer, game.Maps[game.activeMap]);
    drawSprite(
        game.renderer, game.Objects["player"].texture, game.Objects["player"].position,
        game.Objects["player"].spriteOffset, game.Objects["player"].size, game.Objects["player"].spriteSize,
        game.Objects["player"].rotation, game.Objects["player"].color
    );
    drawSprite(
        game.renderer, game.Objects["ball"].texture, game.Objects["ball"].position,
        game.Objects["ball"].spriteOffset, game.Objects["ball"].size, game.Objects["ball"].spriteSize,
        game.Objects["ball"].rotation, game.Objects["ball"].color
    );

    // if (game.canvas.fbo) glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void clearResources(Game game, bool texturesOnly = false) {
    if (!texturesOnly){
        for (auto iter : game.Shaders) {
            glDeleteProgram(iter.second);
        }
    }
    for (auto iter : game.Textures) {
        glDeleteTextures(1, &iter.second.texture_id);
    }
    game.Textures.clear();
}

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

Game Breakout;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // when a user presses the escape key, we set the WindowShouldClose property to true, closing the application
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            Breakout.keys[key] = true;
        else if (action == GLFW_RELEASE)
            Breakout.keys[key] = false;
    }
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout", nullptr, nullptr);
    if (window == nullptr)
        return 1;
        
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return 1;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Breakout = initGame(true);
    glm::vec4 clear_color = glm::vec4(0.5f, 0.3f, 0.1f, 1.00f);

    // DeltaTime variables
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float current_frame = glfwGetTime();
        deltaTime = current_frame - lastFrame;
        lastFrame = current_frame;
        glfwPollEvents();

        proccessInputs(Breakout, deltaTime);

        updateGame(Breakout, deltaTime);

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        renderGame(Breakout);

        glfwSwapBuffers(window);
    }

    clearResources(Breakout, false);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}