#ifndef SMED_UNDOREDO_HPP
#define SMED_UNDOREDO_HPP

#include <omega/util/types.hpp>

using Operation = int;

struct Undo {
    Operation operation;
    f32 time;
};

struct Redo {};

#endif // SMED_UNDOREDO_HPP
