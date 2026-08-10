#ifndef SGE_TEXTURE_H
#define SGE_TEXTURE_H

#include <glad/glad.h>

#include "shader.h"

typedef struct Texture2D {
    GLuint texture_id = 0;
    GLuint width, height;
    GLuint internal_format = GL_RGB;
    GLuint image_format = GL_RGB;
    GLuint wrap_s = GL_REPEAT;
    GLuint wrap_t = GL_REPEAT;
    GLuint filter_min = GL_LINEAR;
    GLuint filter_max = GL_LINEAR;
    bool show = false; // ImGui window flag
} texture2D;

typedef struct {
    GLuint shader;
    GLuint VAO;
} SpriteRenderer;

void generateTexture2D(texture2D *texture, GLuint width, GLuint height, unsigned char* data) {
    if (texture->texture_id == 0) glGenTextures(1, &texture->texture_id);
    texture->width = width;
    texture->height = height;

    glBindTexture(GL_TEXTURE_2D, texture->texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, texture->internal_format, width, height, 0, texture->image_format, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture->wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture->wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture->filter_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture->filter_max);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint initRenderSprite() {
    GLuint VAO, VBO;
    float vertices[] = {
        // pos      // tex
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

void drawSprite(
    SpriteRenderer spr, texture2D tex,
    glm::vec2 position, glm::vec2 spriteOffset = glm::vec2(0.0f, 0.0f),
    glm::vec2 size = glm::vec2(32.0f, 32.0f), glm::vec2 spriteSize = glm::vec2(32.0f, 32.0f),
    float rotate = 0.0f, glm::vec3 color = glm::vec3(1.0f)
) {
    glUseProgram(spr.shader);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position, 0.0f)); // first translate (transformations are: scale happens first, then rotation, and then final translation happens; reversed order)
    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // move origin of rotation to center of the quad
    model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0f, 0.0f, 0.0f)); // rotate
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // move origin back
    model = glm::scale(model, glm::vec3(size, 1.0f));

    setShaderMatrix4(spr.shader, "model", model);
    setShaderVector3f(spr.shader, "spriteColor", color.x, color.y, color.z);
    setShaderVector2f(spr.shader, "spriteSize", spriteSize.x, spriteSize.y);
    setShaderVector2f(spr.shader, "spriteOffset", spriteOffset.x, spriteOffset.y);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.texture_id);

    glBindVertexArray(spr.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

#endif