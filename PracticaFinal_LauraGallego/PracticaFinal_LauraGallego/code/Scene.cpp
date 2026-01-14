// Este código es de dominio público
// penterrin@gmail.com

#include "Scene.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

namespace udit
{
    Scene::Scene(int width, int height)
        : camera(0.1f, 1000.f, float(width) / height),
        skybox("assets/skybox/sky-cube-map-"),
        terrain(nullptr),main_light(nullptr),
        width(width), height(height),
        angle_delta_x(0), angle_delta_y(0), pointer_pressed(false), current_effect(0)
    {
        
        glEnable(GL_DEPTH_TEST);

        root = new Node();
        
        load_scene_from_file("assets/scene.txt");

        // Configuración por defecto de la cámara (si no estaba en el archivo)
        camera.set_location(0.0f, 10.0f, 15.0f);
        camera.set_target(0.0f, 0.0f, 0.0f);

        // Preparación del Framebuffer para efectos de post-proceso 
        init_screen_quad();
        compile_screen_shader();
        init_framebuffer();
    }

    Scene::~Scene()
    {
        for (Mesh* m : meshes) {
            delete m;
        }
        meshes.clear();
       
        if (terrain) delete terrain;

        if (main_light) delete main_light;

        
        glDeleteFramebuffers(1, &framebuffer_id);
        glDeleteTextures(1, &texture_colorbuffer_id);
        glDeleteRenderbuffers(1, &rbo_id);
    }

    void Scene::update(float delta_time, const bool* keys)
    {
        // Control de movimiento de cámara libre (WASD)

        float speed = 5.0f * delta_time;
        if (keys[SDL_SCANCODE_LSHIFT]) speed *= 2.0f;

        // cálculo de vectores
        glm::vec3 front = glm::normalize(glm::vec3(camera.get_target() - camera.get_location()));
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        
        // if keys W, A, S, D
        if (keys[SDL_SCANCODE_W]) camera.move(front * speed);
        if (keys[SDL_SCANCODE_S]) camera.move(-front * speed);
        if (keys[SDL_SCANCODE_A]) camera.move(-right * speed);
        if (keys[SDL_SCANCODE_D]) camera.move(right * speed);

        // Rotación de cámara mediante ratón
        if (angle_delta_x != 0 || angle_delta_y != 0) {
            camera.rotate(glm::rotate(glm::mat4(1.f), angle_delta_x * 0.05f, glm::vec3(0, 1, 0)));
            camera.rotate(glm::rotate(glm::mat4(1.f), angle_delta_y * 0.05f, right));
            angle_delta_x *= 0.9f; angle_delta_y *= 0.9f;
        }

        // Animación: Rotación continua del modelo opaco
        if (cat_opaque) {
            glm::vec3 rot = cat_opaque->get_rotation();
            rot.y += 50.0f * delta_time; 
            cat_opaque->set_rotation(rot);
           
        }

        // Animación: Levitación y giro del gato fantasma
        if (cat_ghost) {
            glm::vec3 rot = cat_ghost->get_rotation();
            rot.y -= 50.0f * delta_time; 
            cat_ghost->set_rotation(rot);
            float time = SDL_GetTicks() / 1000.0f;
            float height = 8.0f + sin(time * 2.0f) * 0.5f; // Oscilación vertical (Seno)
            cat_ghost->set_position({ 2.0f, height, 0.0f });
            
        }

        if (root) root->update();
        
    }

    void Scene::render()
    {
        // PASO 1: Renderizado de la escena en el Framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Dibujado de objetos básicos
        skybox.render(camera);
        if (terrain) terrain->render(camera);

        // --- DIBUJAR TODOS LOS GATOS OPACOS ---
        // Recorremos la lista y solo dibujamos los que NO son transparentes
        for (Mesh* m : meshes) {
            // Si el gato es opaco (casi 1.0), lo dibujamos ahora
            // Usamos 0.9f como margen de seguridad
            if (m->get_opacity() >= 0.9f) {
                m->render(camera);
            }
        }

        // PASO 2: Dibujado de objetos transparentes (Blending)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // Desactivamos escritura en Z-Buffer para el blending [cite: 105]

        // --- DIBUJAR TODOS LOS GATOS TRANSPARENTES ---
        // Volvemos a recorrer la lista pero ahora solo dibujamos los "fantasmas"
        for (Mesh* m : meshes) {
            if (m->get_opacity() < 0.9f) {
                m->render(camera);
            }
        }

        // Restauración del estado normal de OpenGL
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // PASO 3: Post-Proceso (Renderizado del Framebuffer en pantalla)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(screen_shader_id);
        glUniform1i(glGetUniformLocation(screen_shader_id, "mode"), current_effect);
        glBindVertexArray(screen_quad_vao);
        glBindTexture(GL_TEXTURE_2D, texture_colorbuffer_id);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    

    void Scene::init_framebuffer() {
        glGenFramebuffers(1, &framebuffer_id);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);

       
        glGenTextures(1, &texture_colorbuffer_id);
        glBindTexture(GL_TEXTURE_2D, texture_colorbuffer_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_colorbuffer_id, 0);

        
        glGenRenderbuffers(1, &rbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_id);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Scene::init_screen_quad() {
        float quadVertices[] = {
            
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };
        glGenVertexArrays(1, &screen_quad_vao);
        glGenBuffers(1, &screen_quad_vbo);
        glBindVertexArray(screen_quad_vao);
        glBindBuffer(GL_ARRAY_BUFFER, screen_quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }

    void Scene::compile_screen_shader() {
        const char* vShader = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoords;
            out vec2 TexCoords;
            void main() {
                TexCoords = aTexCoords;
                gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
            }
        )";

        const char* fShader = R"(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoords;
            
            uniform sampler2D screenTexture;
            uniform int mode; // 0=Normal, 1=Sepia, 2=Nocturna

            void main() {
                vec3 col = texture(screenTexture, TexCoords).rgb;

                if (mode == 0) // NORMAL
                {
                    FragColor = vec4(col, 1.0);
                }
                else if (mode == 1) // SEPIA (Cine antiguo)
                {
                    vec3 sepia;
                    sepia.r = (col.r * 0.393) + (col.g * 0.769) + (col.b * 0.189);
                    sepia.g = (col.r * 0.349) + (col.g * 0.686) + (col.b * 0.168);
                    sepia.b = (col.r * 0.272) + (col.g * 0.534) + (col.b * 0.131);
                    FragColor = vec4(sepia, 1.0);
                }
                else if (mode == 2) // VISION NOCTURNA (Verde y granulado)
                {
                    float gray = dot(col, vec3(0.299, 0.587, 0.114));
                    vec3 green = vec3(0.0, gray, 0.0); 
                    
                    FragColor = vec4(green * 1.5, 1.0); 
                }
            }
        )";

        
        GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &vShader, NULL); glCompileShader(v);
        GLuint f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f, 1, &fShader, NULL); glCompileShader(f);
        screen_shader_id = glCreateProgram();
        glAttachShader(screen_shader_id, v); glAttachShader(screen_shader_id, f); glLinkProgram(screen_shader_id);
        glDeleteShader(v); glDeleteShader(f);
    }

    void Scene::resize(int w, int h) {
        width = w; height = h;
        camera.set_ratio(float(width) / height);
        glViewport(0, 0, width, height);
        
        init_framebuffer();
    }
    void Scene::on_key_down(int key)
    {
        
        if (key == SDLK_F)
        {
            current_effect++;
            if (current_effect > 2) current_effect = 0; 

            
            if (current_effect == 0) std::cout << "MODO: Normal" << std::endl;
            if (current_effect == 1) std::cout << "MODO: Sepia" << std::endl;
            if (current_effect == 2) std::cout << "MODO: Vision Nocturna" << std::endl;
        }
    }

    void Scene::load_scene_from_file(const std::string& file_path) {
        std::ifstream file(file_path);

        if (!file.is_open()) {
            std::cerr << "ERROR: No se pudo abrir el archivo de escena: " << file_path << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue; // Saltar vacíos y comentarios

            std::stringstream ss(line);
            std::string type;
            ss >> type;

            if (type == "TERRAIN") {
                float w, d;
                int xs, zs;
                std::string path;
                ss >> w >> d >> xs >> zs >> path;

                // Creamos el terreno con los datos leídos
                terrain = new Terrain(w, d, xs, zs, path);
                terrain->set_position({ 0.0f, -2.0f, 0.0f });
                root->add_child(terrain);// Podrías leer la posición también si quieres
            }
            else if (type == "LIGHT") {
                float x, y, z, r, g, b;
                ss >> x >> y >> z >> r >> g >> b;

                main_light = new Light();
                main_light->set_position({ x, y, z });
                main_light->set_color({ r, g, b });
                root->add_child(main_light);
            }
            else if (type == "MESH") {
                std::string path;
                float x, y, z, opacity;
                ss >> path >> x >> y >> z >> opacity;

                Mesh* new_mesh = new Mesh(path);
                new_mesh->set_position({ x, y, z });
                new_mesh->set_opacity(opacity);

                if (main_light) new_mesh->set_light(main_light);

                // IMPORTANTE: Siempre lo guardamos en la lista
                meshes.push_back(new_mesh);
                root->add_child(new_mesh);

                // Solo asignamos a las variables de animación si están vacías
                // Así el primer gato del TXT será el que se mueva
                if (opacity >= 0.9f && cat_opaque == nullptr) cat_opaque = new_mesh;
                else if (opacity < 0.9f && cat_ghost == nullptr) cat_ghost = new_mesh;
            }
        }
    }

        void Scene::on_drag(float x, float y) {
            if (pointer_pressed) {
                angle_delta_x = (last_pointer_x - x) * 0.1f;
                angle_delta_y = (last_pointer_y - y) * 0.1f;
                last_pointer_x = x;
                last_pointer_y = y;
            }
        }
        void Scene::on_click(float x, float y, bool d) {
            pointer_pressed = d;
            if (d) { last_pointer_x = x; last_pointer_y = y; }
        }
}