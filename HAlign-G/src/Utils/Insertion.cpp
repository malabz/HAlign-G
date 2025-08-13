#include "Insertion.hpp"

bool utils::Insertion::operator==(const Insertion &rhs) const noexcept
{
    return index == rhs.index && number == rhs.number;
}


