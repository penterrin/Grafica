
// Este código es de dominio público
// angel.rodriguez@udit.es

#ifndef SCENE_HEADER
#define SCENE_HEADER

    #include "Camera.hpp"
    #include "Skybox.hpp"
    #include "Mesh.hpp"
    #include "Terrain.hpp"
    #include <SDL3/SDL.h>

    namespace udit
    {

        class Scene
        {
        private:

            Camera camera;
            Skybox skybox;

            Mesh* cat_opaque;
            Mesh* cat_ghost;
            Terrain* terrain;

            int    width;
            int    height;

            int current_effect;

            GLuint framebuffer_id;
            GLuint texture_colorbuffer_id; 
            GLuint rbo_id;                 

            GLuint screen_quad_vao;
            GLuint screen_quad_vbo;
            GLuint screen_shader_id;

            float  angle_around_x;
            float  angle_around_y;
            float  angle_delta_x;
            float  angle_delta_y;

            bool   pointer_pressed;
            float  last_pointer_x;
            float  last_pointer_y;

            void init_framebuffer();
            void init_screen_quad();
            void compile_screen_shader();

        public:

            Scene(int width, int height);
            ~Scene();

            void update(float delta_time, const bool* keys);
            void render   ();      

            void resize   (int width, int height);
            void on_drag  (float pointer_x, float pointer_y);
            void on_click (float pointer_x, float pointer_y, bool down);

            void on_key_down(int key);

        };

    }

#endif
