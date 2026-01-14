#pragma once
#include "Node.hpp"
#include <glm.hpp>

namespace udit
{
    // Clase que representa una fuente de luz puntual en la escena
    class Light : public Node
    {
    private:
        glm::vec3 color;

    public:
        Light() : color(1.0f, 1.0f, 1.0f) {} // Blanco por defecto

        void set_color(const glm::vec3& c) { color = c; }
        glm::vec3 get_color() const { return color; }
    };
}