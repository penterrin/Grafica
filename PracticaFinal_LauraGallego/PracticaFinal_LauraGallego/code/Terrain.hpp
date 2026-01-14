// Este código es de dominio público
// penterrin@gmail.com

#pragma once

#include "Node.hpp"
#include <vector>
#include <string>
#include <glad/gl.h>
#include <glm.hpp>

namespace udit
{
    class Terrain : public Node
    {
    private:
        GLuint vao_id;
        GLuint vbo_ids[2]; 
        GLuint texture_id; 
        GLuint shader_program_id;

        GLsizei number_of_vertices;

        
        GLint model_loc, view_loc, proj_loc;
        GLint max_height_loc, texture_loc;
        GLint fog_color_loc, fog_density_loc;

    public:
        
        Terrain(float width, float depth, unsigned x_slices, unsigned z_slices, const std::string& texture_path);
        ~Terrain();

        
        virtual void render(const Camera& camera) override;

    private:
        void compile_shaders();
        void load_heightmap(const std::string& path);
    };
}