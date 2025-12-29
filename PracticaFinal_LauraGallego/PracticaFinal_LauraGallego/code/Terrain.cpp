#include "Terrain.hpp"
#include "Camera.hpp" // Asegúrate de incluir esto para tener acceso a la clase Camera
#include <iostream>
#include <SOIL2.h>
#include <gtc/type_ptr.hpp>
#include <cmath> // Para exp()

namespace udit
{
    Terrain::Terrain(float width, float depth, unsigned x_slices, unsigned z_slices, const std::string& texture_path)
    {
        // --- 1. GENERACIÓN DE LA MALLA (TRIANGLE STRIP) ---
        std::vector<float> coordinates;
        std::vector<float> uvs;

        for (unsigned z = 0; z < z_slices; ++z)
        {
            for (unsigned x = 0; x <= x_slices; ++x)
            {
                float u = (float)x / x_slices;
                float v1 = (float)z / z_slices;
                float v2 = (float)(z + 1) / z_slices;

                float xPos = (u * width) - (width * 0.5f);
                float zPos1 = (v1 * depth) - (depth * 0.5f);
                float zPos2 = (v2 * depth) - (depth * 0.5f);

                coordinates.push_back(xPos); coordinates.push_back(zPos1);
                uvs.push_back(u); uvs.push_back(v1);

                coordinates.push_back(xPos); coordinates.push_back(zPos2);
                uvs.push_back(u); uvs.push_back(v2);
            }
        }

        number_of_vertices = coordinates.size() / 2;

        // --- 2. OPENGL BUFFERS ---
        glGenVertexArrays(1, &vao_id);
        glGenBuffers(2, vbo_ids);

        glBindVertexArray(vao_id);

        // VBO 0: Posiciones
        glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[0]);
        glBufferData(GL_ARRAY_BUFFER, coordinates.size() * sizeof(float), coordinates.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // VBO 1: UVs
        glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[1]);
        glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(float), uvs.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindVertexArray(0);

        // Cargar heightmap y compilar shader
        load_heightmap(texture_path);
        compile_shaders();
    }

    Terrain::~Terrain()
    {
        glDeleteVertexArrays(1, &vao_id);
        glDeleteBuffers(2, vbo_ids);
        glDeleteTextures(1, &texture_id);
        glDeleteProgram(shader_program_id);
    }

    void Terrain::load_heightmap(const std::string& path)
    {
        int w, h, c;
        unsigned char* img = SOIL_load_image(path.c_str(), &w, &h, &c, SOIL_LOAD_L);
        if (img) {
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, img);
            SOIL_free_image_data(img);
        }
        else {
            std::cout << "Error loading heightmap: " << path << std::endl;
        }
    }

    void Terrain::render(const Camera& camera)
    {
        if (shader_program_id == 0) return;

        glUseProgram(shader_program_id);

        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(camera.get_projection_matrix()));
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(camera.get_transform_matrix_inverse()));
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(get_global_matrix()));

        glUniform1f(max_height_loc, 8.0f); // Altura de montañas

        // Niebla gris
        glUniform3f(fog_color_loc, 0.5f, 0.5f, 0.5f);
        glUniform1f(fog_density_loc, 0.04f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glUniform1i(texture_loc, 0);

        glBindVertexArray(vao_id);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, number_of_vertices);
        glBindVertexArray(0);

        Node::render(camera);
    }

    void Terrain::compile_shaders()
    {
        // VERTEX SHADER (Suavizado de normales)
        const char* vSource = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTex;

            out vec3 FragPos;
            out float Height;
            out vec3 Normal;

            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            uniform sampler2D heightMap;
            uniform float max_height;

            void main() {
                float h = texture(heightMap, aTex).r;
                Height = h;

                // Suavizado de normales
                float off = 1.0 / 256.0; 
                float hL = texture(heightMap, aTex + vec2(-off, 0)).r;
                float hR = texture(heightMap, aTex + vec2( off, 0)).r;
                float hD = texture(heightMap, aTex + vec2(0, -off)).r;
                float hU = texture(heightMap, aTex + vec2(0,  off)).r;
                Normal = normalize(vec3(hL - hR, 2.0 / max_height, hD - hU));

                vec3 pos3D = vec3(aPos.x, h * max_height, aPos.y);
                vec4 worldPos = model * vec4(pos3D, 1.0);
                FragPos = worldPos.xyz;
                
                gl_Position = projection * view * worldPos;
            }
        )";

        // FRAGMENT SHADER (Colores procedimentales: Roca -> Nieve)
        const char* fSource = R"(
            #version 330 core
            out vec4 FragColor;
            
            in vec3 FragPos;
            in float Height;
            in vec3 Normal;

            uniform vec3 fog_color;
            uniform float fog_density;

            void main() {
                vec3 norm = normalize(Normal);
                vec3 sunDir = normalize(vec3(0.3, 1.0, 0.5));
                float diff = max(dot(norm, sunDir), 0.25);

                // --- COLORES MATEMÁTICOS (Sin texturas externas) ---
                vec3 rockColor = vec3(0.2, 0.2, 0.2); // Gris oscuro
                vec3 snowColor = vec3(0.9, 0.9, 0.9); // Blanco

                // Mezclamos: Si es bajo -> Roca, Si es alto -> Nieve
                vec3 objectColor = mix(rockColor, snowColor, Height);

                vec3 litColor = objectColor * diff;

                // Niebla Exponencial
                float fogFactor = 1.0 / exp(pow(gl_FragCoord.z / gl_FragCoord.w * fog_density, 2.0));
                fogFactor = clamp(fogFactor, 0.0, 1.0);

                FragColor = vec4(mix(fog_color, litColor, fogFactor), 1.0);
            }
        )";

        GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &vSource, NULL); glCompileShader(v);
        GLuint f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f, 1, &fSource, NULL); glCompileShader(f);
        shader_program_id = glCreateProgram();
        glAttachShader(shader_program_id, v); glAttachShader(shader_program_id, f); glLinkProgram(shader_program_id);
        glDeleteShader(v); glDeleteShader(f);

        model_loc = glGetUniformLocation(shader_program_id, "model");
        view_loc = glGetUniformLocation(shader_program_id, "view");
        proj_loc = glGetUniformLocation(shader_program_id, "projection");
        max_height_loc = glGetUniformLocation(shader_program_id, "max_height");
        texture_loc = glGetUniformLocation(shader_program_id, "heightMap");
        fog_color_loc = glGetUniformLocation(shader_program_id, "fog_color");
        fog_density_loc = glGetUniformLocation(shader_program_id, "fog_density");
    }
}