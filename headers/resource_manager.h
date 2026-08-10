#ifndef SGE_RESOURCE_MANAGER_H
#define SGE_RESOURCE_MANAGER_H

#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "texture.h"
#include "shader.h"

GLuint loadShaderFromFile(const char *vShader, const char *fShader, const char *gShader) {
    std::string vertexCode, fragCode, geoCode;
    try {
        std::ifstream vertexFile(vShader);
        std::ifstream fragFile(fShader);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vertexFile.rdbuf();
        fShaderStream << fragFile.rdbuf();
        vertexFile.close();
        fragFile.close();
        vertexCode = vShaderStream.str();
        fragCode = fShaderStream.str();
        if (gShader != nullptr) {
            std::ifstream geoFile(gShader);
            std::stringstream gShaderStream;
            gShaderStream << geoFile.rdbuf();
            geoFile.close();
            geoCode = gShaderStream.str();
        }
    } catch (std::exception e) {
        std::cout << "ERROR::Shader: Failed to read shader files" << std::endl;
    }
    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragCode.c_str();
    const char *gShaderCode = geoCode.c_str();
    return compileShader(vShaderCode, fShaderCode, gShader != nullptr ? gShaderCode : nullptr);
}

texture2D loadTextureFromFile(const char *file, bool alpha) {
    texture2D tex;
    if (alpha) {
        tex.internal_format = GL_RGBA;
        tex.image_format = GL_RGBA;
    }
    int width, height, nrChannels;
    unsigned char *data = stbi_load(file, &width, &height, &nrChannels, 0);
    generateTexture2D(&tex, width, height, data);
    stbi_image_free(data);

    return tex;
}

#endif