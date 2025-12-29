#include "Scene.hpp"
#include <iostream>
#include <vector>

namespace udit
{
    Scene::Scene(int width, int height)
        : camera(0.1f, 1000.f, float(width) / height),
        skybox("assets/skybox/sky-cube-map-"),
        terrain(nullptr),/* mi_modelo(nullptr),*/ cat_opaque(nullptr), cat_ghost(nullptr),
        width(width), height(height),
        angle_delta_x(0), angle_delta_y(0), pointer_pressed(false), current_effect(0)
    {
        // 1. Configurar OpenGL
        glEnable(GL_DEPTH_TEST);

        // 2. Crear objetos
        terrain = new Terrain(50.0f, 50.0f, 256, 256, "assets/height-map.png");
        terrain->set_position({ 0.0f, -2.0f, 0.0f });

        /*mi_modelo = new Mesh("assets/cat.obj");
        mi_modelo->set_position({ 0.0f, 2.0f, 0.0f });*/

        cat_opaque = new Mesh("assets/cat.obj");
        cat_opaque->set_position({ -2.0f, 8.0f, 0.0f }); // A la izquierda
        cat_opaque->set_opacity(1.0f);

        cat_ghost = new Mesh("assets/cat.obj");
        cat_ghost->set_position({ 2.0f, 8.0f, 0.0f }); // A la derecha
        cat_ghost->set_opacity(0.4f); // 40% de opacidad (FANTASMA)

        camera.set_location(0.0f, 10.0f, 15.0f);
        camera.set_target(0.0f, 0.0f, 0.0f);

        // 3. INICIAR POST-PROCESADO
        init_screen_quad();
        compile_screen_shader();
        init_framebuffer();
    }

    Scene::~Scene()
    {
        if (cat_opaque) delete cat_opaque;
        if (cat_ghost)  delete cat_ghost;
       /* if (mi_modelo) delete mi_modelo;*/
        if (terrain) delete terrain;

        // Limpiar Framebuffer
        glDeleteFramebuffers(1, &framebuffer_id);
        glDeleteTextures(1, &texture_colorbuffer_id);
        glDeleteRenderbuffers(1, &rbo_id);
    }

    void Scene::update(float delta_time, const bool* keys)
    {
        // Movimiento simple WASD
        float speed = 5.0f * delta_time;
        if (keys[SDL_SCANCODE_LSHIFT]) speed *= 2.0f;

        glm::vec3 front = glm::normalize(glm::vec3(camera.get_target() - camera.get_location()));
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));

        if (keys[SDL_SCANCODE_W]) camera.move(front * speed);
        if (keys[SDL_SCANCODE_S]) camera.move(-front * speed);
        if (keys[SDL_SCANCODE_A]) camera.move(-right * speed);
        if (keys[SDL_SCANCODE_D]) camera.move(right * speed);

        // Inercia ratón
        if (angle_delta_x != 0 || angle_delta_y != 0) {
            camera.rotate(glm::rotate(glm::mat4(1.f), angle_delta_x * 0.05f, glm::vec3(0, 1, 0)));
            camera.rotate(glm::rotate(glm::mat4(1.f), angle_delta_y * 0.05f, right));
            angle_delta_x *= 0.9f; angle_delta_y *= 0.9f;
        }

        if (cat_opaque) {
            glm::vec3 rot = cat_opaque->get_rotation();
            rot.y += 50.0f * delta_time; // Gira 50 grados por segundo
            cat_opaque->set_rotation(rot);
            cat_opaque->update(); // IMPORTANTE: Recalcular matriz
        }

        if (cat_ghost) {
            glm::vec3 rot = cat_ghost->get_rotation();
            rot.y -= 50.0f * delta_time; // Gira al revés
            cat_ghost->set_rotation(rot);
            float time = SDL_GetTicks() / 1000.0f;
            float height = 8.0f + sin(time * 2.0f) * 0.5f; // Sube y baja 0.5 metros
            cat_ghost->set_position({ 2.0f, height, 0.0f });
            cat_ghost->update(); // IMPORTANTE: Recalcular matriz
        }

        /*if (mi_modelo) mi_modelo->update();*/
        if (terrain) terrain->update();
    }

    void Scene::render()
    {
        // --- PASO 1: Renderizar en el FRAMEBUFFER (Oculto) ---
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Dibujar escena normal
        skybox.render(camera);
        if (terrain) terrain->render(camera);
       /* if (mi_modelo) mi_modelo->render(camera);*/
        if (cat_opaque) cat_opaque->render(camera);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Desactivamos la escritura en profundidad para que el fantasma no oculte cosas detrás de él incorrectamente
        glDepthMask(GL_FALSE);

        if (cat_ghost) cat_ghost->render(camera);

        // Restauramos el estado normal de OpenGL
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // --- PASO 2: Renderizar en la PANTALLA (Post-Procesado) ---
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Volver a la pantalla real
        glDisable(GL_DEPTH_TEST); // No necesitamos profundidad para una imagen plana
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(screen_shader_id);

        glUniform1i(glGetUniformLocation(screen_shader_id, "mode"), current_effect);

        glBindVertexArray(screen_quad_vao);

        // Pasamos la textura que acabamos de pintar
        glBindTexture(GL_TEXTURE_2D, texture_colorbuffer_id);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // --- FUNCIONES DE AYUDA PARA POST-PROCESADO ---

    void Scene::init_framebuffer() {
        glGenFramebuffers(1, &framebuffer_id);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);

        // Textura de color
        glGenTextures(1, &texture_colorbuffer_id);
        glBindTexture(GL_TEXTURE_2D, texture_colorbuffer_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_colorbuffer_id, 0);

        // Render Buffer (Profundidad)
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
            // Posiciones   // UVs
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
                    // Truco para aumentar el contraste
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
        // Regenerar textura framebuffer al cambiar tamaño
        init_framebuffer();
    }
    void Scene::on_key_down(int key)
    {
        // Si pulsamos 'F', cambiamos de efecto
        if (key == SDLK_F)
        {
            current_effect++;
            if (current_effect > 2) current_effect = 0; // Volver al principio

            // Log para saber qué pasa
            if (current_effect == 0) std::cout << "MODO: Normal" << std::endl;
            if (current_effect == 1) std::cout << "MODO: Sepia" << std::endl;
            if (current_effect == 2) std::cout << "MODO: Vision Nocturna" << std::endl;
        }
    }

    void Scene::on_drag(float x, float y) { if (pointer_pressed) { angle_delta_x = (last_pointer_x - x) * 0.1f; angle_delta_y = (last_pointer_y - y) * 0.1f; last_pointer_x = x; last_pointer_y = y; } }
    void Scene::on_click(float x, float y, bool d) { pointer_pressed = d; if (d) { last_pointer_x = x; last_pointer_y = y; } }
}