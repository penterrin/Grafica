// Este código es de dominio público
// penterrin@gmail.com

#pragma once

#include <cstdint>

namespace udit
{

    using Monochrome8 = uint8_t;

    union Rgba8888
    {
        enum { RED, GREEN, BLUE, ALPHA };

        uint32_t value;
        uint8_t  components[4];
    };

}
