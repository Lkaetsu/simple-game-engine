#ifndef SGE_UTILS_H
#define SGE_UTILS_H

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <nfd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MAP_LAYERS 3
#define MAX_MAP_SIZE 32
#define MAX_SPRITESHEETS 10

typedef struct {
    int spriteID = -1;
    ImVec2 uv0;
    ImVec2 uv1;
} sprite;

typedef struct {
    int spriteSheetID = 0;
    int width;
    int height;
    int tileWidth = 32;
    int tileHeight = 32;
    GLuint texture;
    bool show = false;
    std::string path = "";
    sprite sprites[64];
} image;

typedef struct {
    int map[2][MAX_MAP_SIZE][MAX_MAP_SIZE][3] = {};
    int size[2] = {16, 16};
    std::string saveFile = "";
    int currentLayer = 0;
    int showLayers = 7;
    image spritesheets[MAX_SPRITESHEETS] = {};
} map;

map init_map (){
    map new_map = {};
    return new_map;
}

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
{
    FILE* f = fopen(file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    fclose(f);
    bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}

// Empty an array of spritesheets
void emptySpritesheets(image spritesheets[MAX_SPRITESHEETS]) {
    for (int i = 0; i < MAX_SPRITESHEETS; i++) {
        spritesheets[i].spriteSheetID = 0;
        spritesheets[i].show = 0;
    }
}

// Get sprites from a spritesheet
bool getSprites(image *spritesheet) {
    int i = 0;
    for (int j = 0; j < spritesheet->height; j+=spritesheet->tileHeight){
        for (int k = 0; k < spritesheet->width; k+=spritesheet->tileWidth){
            ImVec2 uv0 = ImVec2((float)k / spritesheet->width, (float)j / spritesheet->height);
            ImVec2 uv1 = ImVec2((float)(spritesheet->tileWidth + k) / spritesheet->width, float(spritesheet->tileHeight + j) / spritesheet->height);
            spritesheet->sprites[i].spriteID = i; 
            spritesheet->sprites[i].uv0 = uv0;
            spritesheet->sprites[i].uv1 = uv1;
            i++;
            if (i > 63) return false;
        }
    }
    return true;
}

// Load a spritesheet into an available position in the array
bool loadSpritesheet(std::string path, image spritesheets[MAX_SPRITESHEETS]) {
    int free_index = -1;
    for(int i = 0; i < MAX_SPRITESHEETS; i++) {
        if (spritesheets[i].spriteSheetID == 0){
            free_index = i;
            break;
        } else if (spritesheets[i].path.compare(path) == 0) {
            free_index = -1;
            break;
        }
    }
    if (free_index != -1) {
        spritesheets[free_index].show = LoadTextureFromFile(path.c_str(), &spritesheets[free_index].texture, &spritesheets[free_index].width, &spritesheets[free_index].height);
        spritesheets[free_index].path = path;
        spritesheets[free_index].spriteSheetID = free_index + 1; 
        getSprites(&spritesheets[free_index]);
    } else {
        printf("File is already open or there is no more space spritesheet slots.");
        return 0;
    }
    return 1;
}

// Load map state from a file
map openMap(const char* openPath) {
    map map = {};
    std::ifstream file(openPath);
    std::string line;
    std::string filename = "";
    int j = -1, k = 0;
    bool readingFilename = 0;
    bool readingMap = 0;
    int layer = -1;

    if (!file.is_open()){
        printf("Error: Could not open file.\n");
        return map;
    }

    while(std::getline(file, line)) {
        if (line[0] == ']') readingMap = 0;
        for(int i = 0; i < line.length(); i++) {
            if (readingMap) {
                if (line[i] == ',' && line[i - 1] != ')') {
                    map.map[0][j][k][layer] = (line[i - 1] - '0');
                    i++;
                    for (int l = 0; l < 3 && line[i + l] != ')'; l++) {
                        filename += line[i + l];
                    }
                    map.map[1][j][k][layer] = atoi(filename.c_str());
                    filename = "";
                    k++;
                }
                continue;
            }
            if (readingFilename) {
                if (line[i] == '\"') break;
                filename += line[i];
                continue;
            }
            if(line[i] == '=') {
                if (line[i - 3] != 'm') {
                    readingFilename = 1;
                    i++;
                } else {
                    readingMap = 1;
                    layer++;
                    j = -1;
                    break;
                }
            }
        }
        if (readingFilename) {
            loadSpritesheet(filename, map.spritesheets);
            readingFilename = 0;
            filename = "";
        }
        if (readingMap) {
            k = 0;
            j++;
        }
        if (layer > MAP_LAYERS) break;
    }
    file.close();

    return map;
}

// Save current map state to file
bool saveMapState(const char* savePath, map map){
    FILE* file = std::fopen(savePath, "w");
    if (file == nullptr) return false;

    for (int i = 0; i < 10 && map.spritesheets[i].spriteSheetID > 0; i++){
        std::fprintf(file, "SprSh[%d]=\"%s\";\n", i, map.spritesheets[i].path.c_str());
    }
    for(int k = 0; k < MAP_LAYERS; k++){
        std::fputs("map=[\n", file);
        for (int i = 0; i < map.size[1]; i++) {
            std::fputs("[", file);
            for (int j = 0; j < map.size[0]; j++){
                std::fprintf(file, "(%d,%d),", map.map[0][i][j][k], map.map[1][i][j][k]);
            }
            std::fputs("]\n", file);
        }
        std::fputs("]\n", file);
    }
    std::fclose(file);

    return true;
}

#endif