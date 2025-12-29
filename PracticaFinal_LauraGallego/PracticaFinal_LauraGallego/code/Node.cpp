#include "Node.hpp"
#include "Camera.hpp" 

namespace udit
{
    Node::Node()
        : parent(nullptr),
        position(0.0f),
        rotation(0.0f),
        scale(1.0f),
        local_matrix(1.0f),
        global_matrix(1.0f)
    {
    }

    Node::~Node()
    {
        for (auto child : children) {
            delete child;
        }
        children.clear();
    }

    void Node::add_child(Node* child)
    {
        child->parent = this;
        children.push_back(child);
    }

    void Node::remove_child(Node* child)
    {
        // Implementación básica opcional
    }

    void Node::update()
    {
        local_matrix = glm::mat4(1.0f);
        local_matrix = glm::translate(local_matrix, position);

        local_matrix = glm::rotate(local_matrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        local_matrix = glm::rotate(local_matrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        local_matrix = glm::rotate(local_matrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));

        local_matrix = glm::scale(local_matrix, scale);

        if (parent) {
            global_matrix = parent->global_matrix * local_matrix;
        }
        else {
            global_matrix = local_matrix;
        }

        for (auto child : children) {
            child->update();
        }
    }

    void Node::render(const Camera& camera)
    {
        for (auto child : children) {
            child->render(camera);
        }
    }
}