#pragma once

#include <cstddef> // Bring in basic types from C

namespace mystl 
{

    using nullptr_t = decltype(nullptr);

    using size_t    = std::size_t;
    using ptrdiff_t = std::ptrdiff_t;

}