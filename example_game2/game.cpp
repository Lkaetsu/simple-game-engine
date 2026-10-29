#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>

#include "enemy.h"

enum GameState {
    GAME_ACTIVE, GAME_PAUSED, GAME_OVER
};

const unsigned int SCREEN_WIDTH = 960;
const unsigned int SCREEN_HEIGHT = 800;
const glm::vec2 PLAYER_SIZE(48.0f, 48.0f);
const float PLAYER_VELOCITY = 200.0f;
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -400.0f);

typedef struct {
    GameState state = GAME_PAUSED;
    bool keys[1024] = {false};
    canvas canvas;
    std::vector<GameMap> Maps;
    std::map<std::string, GLuint> Shaders;
    std::map<std::string, texture2D> Textures;
    std::map<std::string, GameObject> Objects;
    SpriteRenderer renderer;
    Direction playerDirection;
    EnemyAI ghostAI;
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

            const unsigned int columns = textures[texture_keys[tileData[i][j][0] - 1]].width / 32;

            glm::vec2 sprOffset = glm::vec2(
                (tileData[i][j][1] % columns) * 32,
                (tileData[i][j][1] / columns) * 32
                // tileData[i][j][1] % (textures[texture_keys[tileData[i][j][0] - 1]].width / 32) * 32,
                // tileData[i][j][1] / (textures[texture_keys[tileData[i][j][0] - 1]].height / 32) * 32
            );
            glm::vec4 collisionBox = glm::vec4(0.0f, 0.0f, 0.8f, 0.8f);
            if (sprOffset.x < 32.0f) {
                if (sprOffset.y < 32.0f) 
                    collisionBox = glm::vec4(0.45f, 0.15f, 0.55f, 0.45f);
                else if (sprOffset.y < 64.0f)
                    collisionBox = glm::vec4(0.2f, 0.3f, 0.45f, 0.7f);
                else if (sprOffset.y < 96.0f)
                    collisionBox = glm::vec4(0.15f, 0.0f, 0.45f, 1.0f);
                else if (sprOffset.y < 128.0f)
                    collisionBox = glm::vec4(0.15f, 0.0f, 0.45f, 0.5f);
            } else if (sprOffset.x < 64.0f) {
                if (sprOffset.y < 32.0f)
                    collisionBox = glm::vec4(0.0f, 0.15f, 1.0f, 0.45f);
                else if (sprOffset.y < 64.0f)
                    collisionBox = glm::vec4(0.15f, 0.15f, 0.5f, 0.85f);
                else if (sprOffset.y < 96.0f)
                    collisionBox = glm::vec4(0.15f, 0.0f, 0.5f, 0.6f);
                else if (sprOffset.y < 128.0f)
                    collisionBox = glm::vec4(0.0f, 0.15f, 1.0f, 0.45f);
            } else if (sprOffset.x < 96.0f) {
                if (sprOffset.y < 32.0f)
                    collisionBox = glm::vec4(0.0f, 0.15f, 0.45f, 0.45f);
                else if (sprOffset.y < 64.0f)
                    collisionBox = glm::vec4(0.15f, 0.15f, 0.5f, 0.85f);
                else if (sprOffset.y < 96.0f)
                    collisionBox = glm::vec4(0.15f, 0.0f, 0.5f, 0.6f);
                else if (sprOffset.y < 128.0f)
                    collisionBox = glm::vec4(0.15f, 0.0f, 0.45f, 1.0f);
            } else if (sprOffset.x < 128.0f) {
                if (sprOffset.y < 32.0f)
                    collisionBox = glm::vec4(0.2f, 0.3f, 0.2f, 0.2f);
                else if (sprOffset.y < 64.0f)
                    collisionBox = glm::vec4(0.2f, -0.5f, 0.35f, 2.0f);
                else if (sprOffset.y < 96.0f)
                    collisionBox = glm::vec4(0.2f, 0.0f, 0.35f, 0.5f);
                else if (sprOffset.y < 128.0f)
                    collisionBox = glm::vec4(0.15f, 0.0f, 0.45f, 1.0f);
            } else if (sprOffset.x < 160.0f) {
                if (sprOffset.y < 32.0f)
                    collisionBox = glm::vec4(-0.5f, 0.1f, 2.0f, 0.4f);
                else if (sprOffset.y < 64.0f)
                    collisionBox = glm::vec4(0.15f, 0.2f, 0.45f, 0.55f);
                else if (sprOffset.y < 96.0f)
                    collisionBox = glm::vec4(-0.5f, 0.2f, 2.0f, 0.4f);
                else if (sprOffset.y < 128.0f)
                    collisionBox = glm::vec4(0.0f, 0.15f, 1.0f, 0.45f);
            } else if (sprOffset.x < 196.0f) {
                if (sprOffset.y < 32.0f)
                    collisionBox = glm::vec4(0.0f, 0.2f, 0.2f, 0.2f);
                else if (sprOffset.y < 64.0f)
                    collisionBox = glm::vec4(0.2f, -0.5f, 0.35f, 2.0f);
                else if (sprOffset.y < 96.0f)
                    collisionBox = glm::vec4(0.0f, 0.0f, 0.2f, 0.2f);
                else if (sprOffset.y < 128.0f)
                    collisionBox = glm::vec4(0.0f, 0.3f, 1.0f, 0.3f);
            }

            GameObject obj = {
                .position = glm::vec2(map.tileSize.x * j, map.tileSize.y * i - gameCanvas.height * ((int)tileData[i][j][2] - 1)),
                .size = glm::vec2(map.tileSize.x, map.tileSize.y),
                .spriteOffset = sprOffset,
                .spriteSize = glm::vec2(32.0f, 32.0f),
                .collisionType = RECTANGULAR,
                .collisionArea = collisionBox,
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
    GameMap map0 = loadMap(".\\maps\\map0.sgem", game.Textures, game.canvas, initGameObjects);
    game.Maps.push_back(map0);
    glm::vec2 playerPos = glm::vec2(game.canvas.width / 2 - PLAYER_SIZE.x / 2, game.canvas.height / 2 + map0.tileSize.y * 2);
    GameObject player = {
        .position = playerPos,
        .size = PLAYER_SIZE,
        .spriteOffset = glm::vec2(0.0f, 160.0f),
        .spriteSize = glm::vec2(32.0f, 32.0f),
        .collisionType = CIRCULAR,
        .collisionArea = glm::vec4(0.0f, 0.0f, 0.6f, 0.6f),
        .texture = game.Textures[".\\sprites\\PacmanSpritesheet.png"]
    };
    game.Objects["player"] = player;
    GameObject ghost = {
        .position = glm::vec2(game.canvas.width / 2 - PLAYER_SIZE.x / 2, map0.tileSize.y * 4.5),
        .size = PLAYER_SIZE,
        .spriteOffset = glm::vec2(0.0f, 128.0f),
        .spriteSize = glm::vec2(32.0f, 32.0f),
        .color = glm::vec3(1.0f, 0.0f, 0.0f),
        .collisionType = RECTANGULAR,
        .collisionArea = glm::vec4(0, 0.0, 0.51f, 0.6f),
        .texture = game.Textures[".\\sprites\\PacmanSpritesheet.png"]
    };

    game.Objects["ghost"] = ghost;
    return game;
}

void proccessInputs(Game &game, float dt) {
    if (game.state != GAME_ACTIVE){
        if (game.keys[GLFW_KEY_ENTER] || game.keys[GLFW_KEY_SPACE])
            game.state = GAME_ACTIVE;
    }

    if (game.keys[GLFW_KEY_A] || game.keys[GLFW_KEY_LEFT]) {
        game.playerDirection = LEFT;
    }
    if (game.keys[GLFW_KEY_D] || game.keys[GLFW_KEY_RIGHT]) {
        game.playerDirection = RIGHT;
    }
    if (game.keys[GLFW_KEY_W] || game.keys[GLFW_KEY_UP]) {
        game.playerDirection = UP;
    }
    if (game.keys[GLFW_KEY_S] || game.keys[GLFW_KEY_DOWN]) {
        game.playerDirection = DOWN;
    }
}

void updateRedGhost(
    GameMap& map,
    GameObject& ghost,
    const GameObject& player,
    EnemyAI& ai,
    float dt,
    float ghostSpeed
) {
    glm::vec2 playerCenter = collisionCenter(player);

    glm::ivec2 playerTile(
        static_cast<int>(std::floor(playerCenter.x / map.tileSize.x)),
        static_cast<int>(std::floor(playerCenter.y / map.tileSize.y))
    );

    bool targetChanged = playerTile != ai.lastPlayerTile;
    bool pathFinished = ai.nextStep >= ai.path.size();

    if (targetChanged || pathFinished) {
        ai.path = findEnemyPathBFS(map, ghost, player.position);
        ai.nextStep = 0;
        ai.lastPlayerTile = playerTile;
    }

    if (ai.nextStep >= ai.path.size())
        return;

    glm::ivec2 nextTile = ai.path[ai.nextStep];

    glm::vec2 target = tileCenter(map, nextTile.x, nextTile.y);

    glm::vec2 ghostCenter = collisionCenter(ghost);

    glm::vec2 delta = target - ghostCenter;
    float distance = glm::length(delta);
    float maxMove = ghostSpeed * dt;

    if (distance <= maxMove || distance < 0.001f) {
        ghost.position = target - ghost.size * 0.5f;
        ai.nextStep++;
    } else {
        ghost.position += (delta / distance) * maxMove;
    }
}

void updateGame(Game &game, float dt) {
    if (game.state != GAME_ACTIVE) return;

    float velocity = PLAYER_VELOCITY * dt;
    switch (game.playerDirection) {
    case LEFT:
        game.Objects["player"].rotation = 180;
        if (game.Objects["player"].position.x >= 0.0f)
            game.Objects["player"].position.x -= velocity;
        else
            game.Objects["player"].position.x = game.canvas.width + game.Objects["player"].position.x - velocity;
        break;
    case RIGHT:
        game.Objects["player"].rotation = 0;
        if (game.Objects["player"].position.x <= (game.canvas.width - game.Objects["player"].size.x - 0.0f))
            game.Objects["player"].position.x += velocity;
        else
            game.Objects["player"].position.x = game.Objects["player"].position.x - game.canvas.width - velocity;
        break;
    case UP:
        game.Objects["player"].rotation = 270;
        if (game.Objects["player"].position.y >= 0.0f)
            game.Objects["player"].position.y -= velocity;
        else
            game.Objects["player"].position.y = game.canvas.height + game.Objects["player"].position.y - velocity;
        break;
    case DOWN:
        game.Objects["player"].rotation = 90;
        if (game.Objects["player"].position.y <= (game.canvas.height - game.Objects["player"].size.y - 0.0f))
            game.Objects["player"].position.y += velocity;
        else
            game.Objects["player"].position.y = game.Objects["player"].position.y - game.canvas.height - velocity;
        break;
    }

    updateRedGhost(
        game.Maps[game.activeMap],
        game.Objects["ghost"],
        game.Objects["player"],
        game.ghostAI,
        dt,
        PLAYER_VELOCITY * 0.7f
    );

    if (std::get<0>(checkCollision(game.Objects["player"], game.Objects["ghost"]))) {
        game.state = GAME_OVER;
        game.Objects["player"].spriteOffset = glm::vec2(0.0f, 192.0f);
        game.Objects["player"].rotation = 0.0f;
        return;
    }

    // handling tile collision
    for (GameObject &tile : game.Maps[game.activeMap].tiles) {
        if (tile.disabled) continue;
        Collision collision = checkCollision(game.Objects["player"], tile);

        if (std::get<0>(collision)) {
            if (tile.isBreakable) tile.disabled = true;
            else {
                Direction dir = std::get<1>(collision);
                glm::vec2 diff_vector = std::get<2>(collision);
                const float playerRadius = game.Objects["player"].size.x * game.Objects["player"].collisionArea.w * 0.5f;

                if (dir == LEFT || dir == RIGHT) {
                    float penetration = playerRadius - std::abs(diff_vector.x);
                    if (dir == LEFT)
                        game.Objects["player"].position.x += penetration;
                    else
                        game.Objects["player"].position.x -= penetration;
                } else {
                    float penetration = playerRadius - std::abs(diff_vector.y);
                    if (dir == DOWN)
                        game.Objects["player"].position.y += penetration;
                    else
                        game.Objects["player"].position.y -= penetration;
                }
            }
        }
    }
}

void animatePlayer(Game &game, float dt) {
    if (game.state == GAME_ACTIVE) {
        if (game.Objects["player"].spriteOffset.x < 90.0)
            game.Objects["player"].spriteOffset.x += 32.0f;
        else
            game.Objects["player"].spriteOffset.x = 0.0f;
    } else if (game.state == GAME_OVER) {
        if (game.Objects["player"].spriteOffset.x < 150.0)
            game.Objects["player"].spriteOffset.x += 32.0f;
    }
}

void renderGame(Game game) {
    // if (game.canvas.fbo) glBindFramebuffer(GL_FRAMEBUFFER, game.canvas.fbo);
    drawMap(game.renderer, game.Maps[game.activeMap]);
    drawSprite(
        game.renderer, game.Objects["ghost"].texture, game.Objects["ghost"].position,
        game.Objects["ghost"].spriteOffset, game.Objects["ghost"].size, game.Objects["ghost"].spriteSize,
        game.Objects["ghost"].rotation, game.Objects["ghost"].color
    );
    drawSprite(
        game.renderer, game.Objects["ghost"].texture, game.Objects["ghost"].position,
        glm::vec2(game.Objects["ghost"].spriteOffset.x + 32.0f, game.Objects["ghost"].spriteOffset.y),
        game.Objects["ghost"].size, game.Objects["ghost"].spriteSize,
        game.Objects["ghost"].rotation, glm::vec3(1.0f)
    );
    drawSprite(
        game.renderer, game.Objects["player"].texture, game.Objects["player"].position,
        game.Objects["player"].spriteOffset, game.Objects["player"].size, game.Objects["player"].spriteSize,
        game.Objects["player"].rotation, game.Objects["player"].color
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

Game game;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // when a user presses the escape key, we set the WindowShouldClose property to true, closing the application
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            game.keys[key] = true;
        else if (action == GLFW_RELEASE)
            game.keys[key] = false;
    }
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pacman", nullptr, nullptr);
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

    game = initGame(true);
    glm::vec4 clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.00f);

    // DeltaTime variables
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    // Player Animation Control variables
    float animTimer = 0.0f;
    float animCd    = 0.1f;
    while (!glfwWindowShouldClose(window)) {
        float current_frame = glfwGetTime();
        deltaTime = current_frame - lastFrame;
        lastFrame = current_frame;
        glfwPollEvents();

        proccessInputs(game, deltaTime);

        updateGame(game, deltaTime);

        animTimer += deltaTime;
        if (animTimer >= animCd) {
            animatePlayer(game, deltaTime);
            animTimer = 0.0f;
        }

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        renderGame(game);

        glfwSwapBuffers(window);
    }

    clearResources(game, false);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}