#pragma once

#include "Node.hpp"
#include <vector>
#include <string>
#include <glad/gl.h>
#include <glm.hpp>

// Incluimos Assimp para cargar modelos
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace udit
{
    class Mesh : public Node
    {
    private:
        // Estructura interna para los vértices
        struct Vertex {
            glm::vec3 Position;
            glm::vec3 Normal;
            glm::vec2 TexCoords; 
        };



        // Datos de la malla
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // OpenGL IDs (Buffers)
        GLuint VAO, VBO, EBO;
        GLuint shader_program_id;
        GLuint texture_id;        
        GLint model_loc, view_loc, proj_loc, color_loc, viewPos_loc;

        float opacity;

        void setup_mesh();
        void compile_shaders();
        void process_node(aiNode* node, const aiScene* scene);
        void process_mesh(aiMesh* mesh, const aiScene* scene);

    public:
        
        Mesh(const std::string& path);
        ~Mesh();
        void set_opacity(float val) { opacity = val; }
       
        virtual void render(const Camera& camera) override;

  
     
    };
}