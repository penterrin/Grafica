// Este código es de dominio público
// penterrin@gmail.com

#pragma once

#include "Node.hpp"
#include "Light.hpp"
#include <vector>
#include <string>
#include <glad/gl.h>
#include <glm.hpp>


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace udit
{
    class Mesh : public Node
    {
    private:
        
        struct Vertex {
            glm::vec3 Position;
            glm::vec3 Normal;
            glm::vec2 TexCoords; 
        };



        
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        
        GLuint VAO, VBO, EBO;
        GLuint shader_program_id;
        GLuint texture_id;        
        GLint model_loc, view_loc, proj_loc, color_loc, viewPos_loc;

        float opacity;

        void setup_mesh();
        void compile_shaders();
        void process_node(aiNode* node, const aiScene* scene);
        void process_mesh(aiMesh* mesh, const aiScene* scene);

        Light* light_ptr;

    public:
        
        Mesh(const std::string& path);
        ~Mesh();

        float get_opacity() const { return opacity; }

        void set_opacity(float val) { opacity = val; }
       
        virtual void render(const Camera& camera) override;

        void set_light(Light* l) { light_ptr = l; }
     
    };
}