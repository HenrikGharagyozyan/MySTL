#pragma once

#include "utility.hpp"
#include <cstddef>

namespace mystl 
{
    // Компаратор по умолчанию
    template <typename T>
    struct less 
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const 
        {
            return lhs < rhs;
        }
    };

    template <typename T>
    struct greater
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const 
        {
            return lhs > rhs;
        }
    };

    // Вспомогательная функция просеивания вниз (O(log N))
    template <typename RandomIt, typename Distance, typename Compare>
    void sift_down(RandomIt first, Distance len, Distance start, Compare comp) 
    {
        Distance parent = start;
        // Запоминаем просеиваемый элемент (через move-семантику)
        auto value = mystl::move(*(first + parent)); 

        while (true) 
        {
            Distance child = 2 * parent + 1; // Левый потомок
            if (child >= len) 
                break;

            // Выбираем наибольшего потомка
            if (child + 1 < len && comp(*(first + child), *(first + child + 1))) 
            {
                child++;
            }

            // Если родитель меньше потомка, поднимаем потомка наверх
            if (comp(value, *(first + child))) 
            {
                *(first + parent) = mystl::move(*(first + child));
                parent = child;
            } 
            else 
            {
                break;
            }
        }
        // Ставим элемент на его законное место
        *(first + parent) = mystl::move(value); 
    }

    template <typename RandomIt, typename Compare>
    void push_heap(RandomIt first, RandomIt last, Compare comp) 
    {
        using Distance = std::ptrdiff_t;
        Distance len = last - first;
        if (len < 2) 
            return;

        Distance child = len - 1;
        auto value = mystl::move(*(last - 1));

        // Просеивание вверх (O(log N))
        while (child > 0) 
        {
            Distance parent = (child - 1) / 2;
            if (comp(*(first + parent), value)) 
            {
                *(first + child) = mystl::move(*(first + parent));
                child = parent;
            } 
            else 
            {
                break;
            }
        }
        *(first + child) = mystl::move(value);
    }

    template <typename RandomIt, typename Compare>
    void pop_heap(RandomIt first, RandomIt last, Compare comp) 
    {
        if (last - first < 2) return;
        // Меняем корень (первый элемент) с последним
        mystl::swap(*first, *(last - 1));
        // Восстанавливаем свойство кучи для оставшихся элементов
        sift_down(first, static_cast<std::ptrdiff_t>(last - first - 1), static_cast<std::ptrdiff_t>(0), comp);
    }

    template <typename RandomIt, typename Compare>
    void make_heap(RandomIt first, RandomIt last, Compare comp) 
    {
        using Distance = std::ptrdiff_t;
        Distance len = last - first;
        if (len < 2) 
            return;

        // Начинаем с последнего узла, у которого есть потомки, и идем к корню (O(N))
        for (Distance i = (len - 2) / 2; i >= 0; --i) 
        {
            sift_down(first, len, i, comp);
        }
    }

} // namespace mystl