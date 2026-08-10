#ifndef SGE_SHADER_H
#define SGE_SHADER_H

#include <string>
#include <iostream>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

void checkCompileErrors(GLuint shader_id, std::string type){
    int success;
    char infoLog[1024];
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader_id, 1024, NULL, infoLog);
        std::cout << "| ERROR::Shader: Compile-time error: Type: " << type << "\n"
            << infoLog << "\n -- --------------------------------------------------- -- "
            << std::endl;
    }
}

void checkLinkErrors(GLuint shader_program_id){
    int success;
    char infoLog[1024];
    glGetProgramiv(shader_program_id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program_id, 1024, NULL, infoLog);
        std::cout << "| ERROR::Shader: Link-time error: Type: PROGRAM" << "\n"
        << infoLog << "\n -- --------------------------------------------------- -- "
        << std::endl;
    }
}

GLuint compileShader(const char* vertSrc, const char* fragSrc, const char* geoSrc){
    GLuint sVert, sFrag, sGeo;
    GLuint shader_program_id;
    // Vertex Shader
    sVert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(sVert, 1, &vertSrc, NULL);
    glCompileShader(sVert);
    checkCompileErrors(sVert, "VERTEX");
    // Fragment Shader
    sFrag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(sFrag, 1, &fragSrc, NULL);
    glCompileShader(sFrag);
    checkCompileErrors(sFrag, "FRAGMENT");
    // Geometry Shader if given
    if (geoSrc != nullptr){
        sGeo = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(sGeo, 1, &geoSrc, NULL);
        glCompileShader(sGeo);
        checkCompileErrors(sGeo, "GEOMETRY");
    }
    // Shader Program
    shader_program_id = glCreateProgram();
    glAttachShader(shader_program_id, sVert);
    glAttachShader(shader_program_id, sFrag);
    if (geoSrc != nullptr) glAttachShader(shader_program_id, sGeo);
    glLinkProgram(shader_program_id);
    checkLinkErrors(shader_program_id);
    glDeleteShader(sVert);
    glDeleteShader(sFrag);
    if (geoSrc != nullptr) glDeleteShader(sGeo);

    return shader_program_id;
}

void setShaderFloat(GLuint shader_program_id, const char *name, float value, bool useShader = false){
    if (useShader) glUseProgram(shader_program_id);
    glUniform1f(glGetUniformLocation(shader_program_id, name), value);
}
void setShaderInt(GLuint shader_program_id, const char *name, int value, bool useShader = false){
    if (useShader) glUseProgram(shader_program_id);
    glUniform1i(glGetUniformLocation(shader_program_id, name), value);
}
void setShaderVector2f(GLuint shader_program_id, const char *name, float x, float y, bool useShader = false){
    if (useShader) glUseProgram(shader_program_id);
    glUniform2f(glGetUniformLocation(shader_program_id, name), x, y);
}
void setShaderVector3f(GLuint shader_program_id, const char *name, float x, float y, float z, bool useShader = false){
    if (useShader) glUseProgram(shader_program_id);
    glUniform3f(glGetUniformLocation(shader_program_id, name), x, y, z);
}
void setShaderVector4f(GLuint shader_program_id, const char *name, float x, float y, float z, float w, bool useShader = false){
    if (useShader) glUseProgram(shader_program_id);
    glUniform4f(glGetUniformLocation(shader_program_id, name), x, y, z, w);
}
void setShaderMatrix4(GLuint shader_program_id, const char *name, glm::mat4 &matrix, bool useShader = false){
    if (useShader) glUseProgram(shader_program_id);
    glad_glUniformMatrix4fv(glGetUniformLocation(shader_program_id, name), 1, false, glm::value_ptr(matrix));
}

#endif