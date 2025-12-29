#include "Mesh.hpp"
#include "Camera.hpp"
#include <SOIL2.h>
#include <iostream>
#include <gtc/type_ptr.hpp>


namespace udit
{
    Mesh::Mesh(const std::string& path) : opacity(1.0f)
    {
       
        Assimp::Importer importer;
        
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }

        std::cout << "------------------------------------------------" << std::endl;
        std::cout << "EXITO: Modelo encontrado: " << path << std::endl;


        // Procesar nodos recursivamente
        process_node(scene->mRootNode, scene);

        std::cout << "INFO: Se han cargado " << vertices.size() << " vertices." << std::endl;
        if (vertices.size() == 0) std::cout << "ALERTA: El modelo esta vacio!" << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        // ----------------------------------------------

        // 2. Preparar OpenGL
        setup_mesh();

        texture_id = SOIL_load_OGL_texture(
            "assets/cat.png",  // Asegúrate de que se llame así
            SOIL_LOAD_AUTO,
            SOIL_CREATE_NEW_ID,
            SOIL_FLAG_MIPMAPS /*| SOIL_FLAG_INVERT_Y*/
        );

        if (texture_id == 0) {
            std::cout << "ERROR: No se pudo cargar la textura del gato (assets/cat.jpg)" << std::endl;
        }
        compile_shaders();
    }

    Mesh::~Mesh()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteProgram(shader_program_id);
    }

    void Mesh::process_node(aiNode* node, const aiScene* scene)
    {
        // Procesar todas las mallas de este nodo
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            process_mesh(mesh, scene);
        }
        // Repetir para los hijos
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            process_node(node->mChildren[i], scene);
        }
    }

    void Mesh::process_mesh(aiMesh* mesh, const aiScene* scene)
    {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            // Posiciones
            vertex.Position.x = mesh->mVertices[i].x;
            vertex.Position.y = mesh->mVertices[i].y;
            vertex.Position.z = mesh->mVertices[i].z;

            // Normales (si existen)
            if (mesh->HasNormals())
            {
                vertex.Normal.x = mesh->mNormals[i].x;
                vertex.Normal.y = mesh->mNormals[i].y;
                vertex.Normal.z = mesh->mNormals[i].z;
            }
            else vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);

            if (mesh->mTextureCoords[0])
            {
                vertex.TexCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.TexCoords.y = mesh->mTextureCoords[0][i].y;
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        // Indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
    }

    void Mesh::setup_mesh()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // Vértices
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // Índices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Atributo 0: Posición
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // Atributo 1: Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }

    void Mesh::render(const Camera& camera)
    {
        glUseProgram(shader_program_id);

        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(camera.get_projection_matrix()));
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(camera.get_transform_matrix_inverse()));
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(get_global_matrix()));

        // --- NUEVO: Pasamos la posición de la cámara para calcular los brillos ---
        glm::vec4 camPos = camera.get_location();
        glUniform3f(glGetUniformLocation(shader_program_id, "viewPos"), camPos.x, camPos.y, camPos.z);
        glUniform1f(glGetUniformLocation(shader_program_id, "alpha"), opacity);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        Node::render(camera);
    }

    void Mesh::compile_shaders()
    {
        // VERTEX SHADER: Ahora recibe y pasa las coordenadas de textura (aTexCoords)
        const char* vShaderCode = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoords; // <--- NUEVO: Coordenadas UV del modelo

        out vec3 Normal;
        out vec3 FragPos;
        out vec2 TexCoords; // <--- NUEVO: Se lo pasamos al fragment

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main()
        {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;  
            TexCoords = aTexCoords; // <--- Pasamos la coordenada
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

        // FRAGMENT SHADER: Usa la textura en vez del color gris
        const char* fShaderCode = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 Normal;
        in vec3 FragPos;
        in vec2 TexCoords; // <--- Recibimos coordenadas

        uniform vec3 viewPos;
        uniform sampler2D texture1; // <--- La imagen del gato
        uniform float alpha;

        void main()
        {
            // Leemos el color de la textura en este punto
            vec3 objectColor = texture(texture1, TexCoords).rgb;

            // --- ILUMINACIÓN PHONG (Igual que antes pero usando el color de la textura) ---
            vec3 lightPos = vec3(5.0, 50.0, 5.0); 
            vec3 lightColor = vec3(1.0, 1.0, 0.9);

            // Ambiente
            float ambientStrength = 0.5;
            vec3 ambient = ambientStrength * lightColor;
  
            // Difusa
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            
            // Especular (Brillo)
            float specularStrength = 0.5; // Un poco menos brillo para que no parezca mojado
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);  
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); 
            vec3 specular = specularStrength * spec * lightColor;  
                
            // Multiplicamos la luz por el color de la textura
            vec3 result = (ambient + diffuse + specular) * objectColor;
            FragColor = vec4(result, alpha);
        }
    )";

        // Compilación estándar (esto no cambia)
        GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &vShaderCode, NULL); glCompileShader(v);
        GLuint f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f, 1, &fShaderCode, NULL); glCompileShader(f);
        shader_program_id = glCreateProgram();
        glAttachShader(shader_program_id, v); glAttachShader(shader_program_id, f); glLinkProgram(shader_program_id);
        glDeleteShader(v); glDeleteShader(f);

        model_loc = glGetUniformLocation(shader_program_id, "model");
        view_loc = glGetUniformLocation(shader_program_id, "view");
        proj_loc = glGetUniformLocation(shader_program_id, "projection");
        viewPos_loc = glGetUniformLocation(shader_program_id, "viewPos");
        // No hace falta buscar la location de la textura porque por defecto usa la 0
    }
}