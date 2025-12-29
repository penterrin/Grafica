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
        GLuint vbo_ids[2]; // Coordenadas y UVs
        GLuint texture_id; // Solo el Heightmap (mapa de alturas)
        GLuint shader_program_id;

        GLsizei number_of_vertices;

        // IDs de variables dentro del shader
        GLint model_loc, view_loc, proj_loc;
        GLint max_height_loc, texture_loc;
        GLint fog_color_loc, fog_density_loc;

    public:
        // Constructor simple
        Terrain(float width, float depth, unsigned x_slices, unsigned z_slices, const std::string& texture_path);
        ~Terrain();

        // Sobrescribimos render para que el terreno se pinte a sí mismo
        virtual void render(const Camera& camera) override;

    private:
        void compile_shaders();
        void load_heightmap(const std::string& path);
    };
}