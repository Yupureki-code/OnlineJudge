#pragma once

#include <cstdint>
#include <string>

#include <odb/core.hxx>

#include "types.hxx"

namespace oj::db {

#pragma db object table("comment_actions")
struct CommentAction {
    #pragma db id auto type("BIGINT UNSIGNED")
    uint64_t id;

    #pragma db type("INT UNSIGNED")
    uint32_t comment_id;

    #pragma db type("INT UNSIGNED")
    uint32_t user_id;

    #pragma db type("ENUM('like','favorite')")
    std::string action_type;

    #pragma db type("DATETIME")
    DateTime created_at;
};

} // namespace oj::db
