// Este código es de dominio público
// penterrin@gmail.com

#pragma once


#include <vector>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>


namespace udit
{
    class Camera;
    class Node
    {
    protected:
        
        Node* parent;
        std::vector<Node*> children;

       
        glm::vec3 position;
        glm::vec3 rotation; 
        glm::vec3 scale;

       
        glm::mat4 local_matrix;
        glm::mat4 global_matrix;

    public:
        Node();
        virtual ~Node();

       
        void add_child(Node* child);
        void remove_child(Node* child);

       
        virtual void update();
        virtual void render(const Camera& camera);

        
        void set_position(const glm::vec3& pos) { position = pos; }
        void set_rotation(const glm::vec3& rot) { rotation = rot; }
        void set_scale(const glm::vec3& scl) { scale = scl; }

       
        glm::vec3 get_position() const { return position; }
        glm::vec3 get_rotation() const { return rotation; }       
        glm::vec3 get_scale()    const { return scale; }
        const glm::mat4& get_global_matrix() const { return global_matrix; }

    private:
        void update_matrices();
    };
}