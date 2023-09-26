#ifndef RAW_COMMENT_HPP
#define RAW_COMMENT_HPP

#include <string>

#include "../json/json.hpp"

class RawComment {
    public:
    std::string rawComment = "";
    std::string publishedAt = "";

    RawComment(std::string rawComment, std::string publishedAt) {
        this->rawComment = rawComment;
        this->publishedAt = publishedAt;
    };

    nlohmann::json to_json() {
        nlohmann::json j;
        j["raw_comment"] = this->rawComment;
        j["publishedAt"] = this->publishedAt;
        return j;
    }
};

#endif