// Node.hpp
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
        // Relaciones familiares
        Node* parent;
        std::vector<Node*> children;

        // Transformaciones (Posición, Rotación, Escala)
        glm::vec3 position;
        glm::vec3 rotation; // En grados (Euler angles)
        glm::vec3 scale;

        // Matrices calculadas
        glm::mat4 local_matrix;
        glm::mat4 global_matrix;

    public:
        Node();
        virtual ~Node();

        // Gestión del grafo
        void add_child(Node* child);
        void remove_child(Node* child);

        // El corazón del motor: Update y Render
        // update_self: calcula la matriz de este nodo
        // render: se dibuja a sí mismo (será sobrescrito por hijos como Mesh)
        virtual void update();
        virtual void render(const Camera& camera);

        // Setters para mover cosas fácilmente
        void set_position(const glm::vec3& pos) { position = pos; }
        void set_rotation(const glm::vec3& rot) { rotation = rot; }
        void set_scale(const glm::vec3& scl) { scale = scl; }

        // Getters
        glm::vec3 get_position() const { return position; }
        glm::vec3 get_rotation() const { return rotation; }       
        glm::vec3 get_scale()    const { return scale; }
        const glm::mat4& get_global_matrix() const { return global_matrix; }

    private:
        void update_matrices();
    };
}