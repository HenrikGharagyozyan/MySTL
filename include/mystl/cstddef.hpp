#pragma once

#include <cstddef> // Подтягиваем базовые типы из C

namespace mystl 
{

    using nullptr_t = decltype(nullptr);

    using size_t    = std::size_t;
    using ptrdiff_t = std::ptrdiff_t;

}