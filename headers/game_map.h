#ifndef SGE_GAME_MAP
#define SGE_GAME_MAP

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

typedef struct {
    glm::vec2 position = glm::vec2(0.0f, 0.0f);
    glm::vec2 size = glm::vec2(1.0f, 1.0f);
    glm::vec2 spriteOffset = glm::vec2(0.0f, 0.0f);
    glm::vec2 spriteSize = glm::vec2(32.0f, 32.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float velocity = 0.0f;
    float rotation = 0.0f;
    bool isBreakable = false;
    bool disabled = false;
    texture2D texture;
} GameObject;

typedef struct {
    std::vector<GameObject> tiles;
    std::array<unsigned int, 2> size = {16, 16};
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

void drawMap(SpriteRenderer renderer, GameMap map) {
    for (GameObject tile : map.tiles) {
        if (!tile.disabled) {
            drawSprite(renderer, tile.texture, tile.position, tile.spriteOffset, tile.size, tile.spriteSize, tile.rotation, tile.color);
        }
    }
}

#endif