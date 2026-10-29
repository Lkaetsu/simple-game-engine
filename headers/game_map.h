#ifndef SGE_GAME_MAP_H
#define SGE_GAME_MAP_H

#include <map>
#include <array>
#include <string>
#include <vector>
#include <functional>
#include <stdlib.h>
#include <fstream>
#include <glm/glm.hpp>

#include "canvas.h"
#include "resource_manager.h"

enum CollisionAreaType {
    RECTANGULAR, CIRCULAR
};

enum Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

typedef std::tuple<bool, Direction, glm::vec2> Collision;

typedef struct {
    glm::vec2 position = glm::vec2(0.0f, 0.0f);
    glm::vec2 size = glm::vec2(1.0f, 1.0f);
    glm::vec2 spriteOffset = glm::vec2(0.0f, 0.0f);
    glm::vec2 spriteSize = glm::vec2(32.0f, 32.0f);
    glm::vec3 color = glm::vec3(1.0f);
    glm::vec2 velocity = glm::vec2(0.0f);
    CollisionAreaType collisionType = RECTANGULAR;
    glm::vec4 collisionArea = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // [x, y, w, h]
    float rotation = 0.0f;
    bool isBreakable = false;
    bool disabled = false;
    texture2D texture;
} GameObject;

typedef struct {
    std::vector<GameObject> tiles;
    std::array<unsigned int, 2> size = {16, 16};
    glm::vec2 tileSize = glm::vec2(10.0f);
} GameMap;

unsigned int _extract_int_from_string (std::string text, char separator, unsigned int *index = 0) {
    size_t target_pos = text.find(separator, *index);
    if (target_pos == std::string::npos) return 0;
    size_t length = target_pos - *index;

    int res = atoi(text.substr(*index + 1, length).c_str());
    *index += length;
    return res;
}

// Load map state from a file
GameMap loadMap(
    const char* openPath,
    std::map<std::string, texture2D> &textures,
    canvas gameCanvas,
    std::function<GameMap (
        std::vector<std::vector<std::array<unsigned int, 3>>>,
        std::vector<std::string>,
        std::map<std::string, texture2D>,
        canvas
    )> handleMapObjects
) {
    GameMap map = {};
    std::vector<std::string> texture_keys;
    std::vector<std::vector<std::array<unsigned int, 3>>> tileData;
    std::ifstream file(openPath);
    std::string line;
    unsigned int readingSpritesheets = 0, readingMapLayers = 0, lastPos = 0;

    if (!file.is_open()){
        printf("Error: Could not open file.\n");
        return map;
    }

    while(std::getline(file, line)) {
        std::vector<std::array<unsigned int, 3>> row;
        if (readingMapLayers) {
            // Layer X
            if (line[0] != '['){
                if (line[0] == 'm') readingMapLayers++;
            } else {
                for (int i = 0; i < 32; i++){
                    unsigned int texture_id = 0, sprite_id = 0;
                    lastPos = line.find('(', lastPos);
                    if (lastPos > line.size()) break;

                    texture_id = _extract_int_from_string(line, ',', &lastPos);
                    sprite_id = _extract_int_from_string(line, ')', &lastPos);
                    std::array<unsigned int, 3> arr = {texture_id, sprite_id, readingMapLayers};
                    row.push_back(arr);
                }
                tileData.push_back(row);
                lastPos = 0;
            }
        } else if (readingSpritesheets) {
            if (line.compare("map=[") == 0) {
                readingSpritesheets = 0;
                readingMapLayers = 1;
                continue;
            }
            texture_keys.push_back(line.substr(10, line.size() - 12));
            textures[line.substr(10, line.size() - 12)] = loadTextureFromFile(line.substr(10, line.size() - 12).c_str(), true);
        } else if (line.find("SprSh") != std::string::npos) {
            texture_keys.push_back(line.substr(10, line.size() - 12));
            textures[line.substr(10, line.size() - 12)] = loadTextureFromFile(line.substr(10, line.size() - 12).c_str(), true);
            readingSpritesheets++;
        }
    }
    file.close();

    map = handleMapObjects(tileData, texture_keys, textures, gameCanvas);

    return map;
}

Direction vectorDirection(glm::vec2 target) {
        glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),      // up
        glm::vec2(1.0f, 0.0f),      // right
        glm::vec2(0.0f, -1.0f),     // down
        glm::vec2(-1.0f, 0.0f),     // left
    };

    float max = 0.0f;
    unsigned int best_match = -1;
    for (unsigned int i = 0; i < 4; i++) {
        float dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max) {
            max = dot_product;
            best_match = i;
        }
    }
    return (Direction) best_match;
}

Collision checkCollision(const GameObject& one, const GameObject& two) {
    const glm::vec2 pos1(
        one.position.x + one.size.x * one.collisionArea.x,
        one.position.y + one.size.y * one.collisionArea.y
    );
    const glm::vec2 pos2(
        two.position.x + two.size.x * two.collisionArea.x,
        two.position.y + two.size.y * two.collisionArea.y
    );

    const glm::vec2 size1(
        one.size.x * one.collisionArea.z,
        one.size.y * one.collisionArea.w
    );
    const glm::vec2 size2(
        two.size.x * two.collisionArea.z,
        two.size.y * two.collisionArea.w
    );

    if (one.collisionType == CIRCULAR && two.collisionType == CIRCULAR) {
        return std::make_tuple(false, UP, glm::vec2(0.0f));
    } else if (one.collisionType == CIRCULAR || two.collisionType == CIRCULAR) {
        const bool firstIsCircle = one.collisionType == CIRCULAR;
        const glm::vec2 circlePos = firstIsCircle ? pos1 : pos2;
        const glm::vec2 circleSize = firstIsCircle ? size1 : size2;
        const glm::vec2 rectPos = firstIsCircle ? pos2 : pos1;
        const glm::vec2 rectSize = firstIsCircle ? size2 : size1;

        const float circRadius = circleSize.x * 0.5f;

        const glm::vec2 center = circlePos + glm::vec2(circRadius);

        glm::vec2 aabb_half_extents = rectSize * 0.5f;
        glm::vec2 aabb_center = glm::vec2(
            rectPos.x + aabb_half_extents.x,
            rectPos.y + aabb_half_extents.y
        );

        glm::vec2 diff = center - aabb_center;
        glm::vec2 clamped = glm::clamp(diff, -aabb_half_extents, aabb_half_extents);
        glm::vec2 closest = aabb_center + clamped;
        diff = closest - center;

        if (glm::length(diff) <= circRadius)
            return std::make_tuple(true, vectorDirection(diff), diff);
        else
            return std::make_tuple(false, UP, glm::vec2(0.0f));
    } else {
        bool collisionX = pos1.x + size1.x >= pos2.x && pos2.x + size2.x >= pos1.x;
        bool collisionY = pos1.y + size1.y >= pos2.y && pos2.y + size2.y >= pos1.y;

        if (collisionX && collisionY) {
            const glm::vec2 center1 = pos1 + size1 * 0.5f;
            const glm::vec2 center2 = pos2 + size2 * 0.5f;

            const glm::vec2 delta = center1 - center2;

            return std::make_tuple(true, vectorDirection(delta), delta);
        }
        return std::make_tuple(false, UP, glm::vec2(0.0f));
    }
}

void drawMap(SpriteRenderer renderer, GameMap map) {
    for (GameObject tile : map.tiles) {
        if (!tile.disabled) {
            drawSprite(renderer, tile.texture, tile.position, tile.spriteOffset, tile.size, tile.spriteSize, tile.rotation, tile.color);
        }
    }
}

#endif