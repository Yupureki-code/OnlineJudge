#pragma once

#include <cstdint>
#include <string>

#include <odb/core.hxx>

#include "types.hxx"

namespace oj::db {

#pragma db object table("solution_actions")
struct SolutionAction {
    #pragma db id auto type("INT UNSIGNED")
    uint32_t id;

    #pragma db type("INT UNSIGNED")
    uint32_t solution_id;

    #pragma db type("INT UNSIGNED")
    uint32_t user_id;

    #pragma db type("ENUM('like','comment','favorite')")
    std::string action_type;

    #pragma db type("DATETIME")
    DateTime created_at;
};

} // namespace oj::db
