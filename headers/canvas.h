#ifndef SGE_CANVAS_H
#define SGE_CANVAS_H

#include <iostream>

#include <glad/glad.h>

typedef struct {
    GLuint fbo = 0;
    GLuint texture_id = 0;
    GLuint rbo = 0;
    float width = 768;
    float height = 512;
} canvas;

canvas initFramebuffer(int width, int height) {
    // Create Framebuffer
    canvas canvas;
    canvas.width = width;
    canvas.height = height;
    glGenFramebuffers(1, &canvas.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, canvas.fbo);

    // Create Color Attachment Texture
    glGenTextures(1, &canvas.texture_id);
    glBindTexture(GL_TEXTURE_2D, canvas.texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvas.texture_id, 0);

    // Create Renderbuffer
    glGenRenderbuffers(1, &canvas.rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, canvas.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, canvas.rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cout << "ERROR::Framebuffer: Framebuffer is not complete\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    return canvas;
}

void rescale_framebuffer(canvas canvas) {
    glBindTexture(GL_TEXTURE_2D, canvas.texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas.width, canvas.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvas.texture_id, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, canvas.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, canvas.width, canvas.height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, canvas.rbo);
}

#endif