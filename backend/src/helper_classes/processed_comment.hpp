#ifndef PROCESSED_COMMENT_HPP
#define PROCESSED_COMMENT_HPP

#include <string>
#include "raw_comment.hpp"

class ProcessedComment {
    public:
    RawComment raw_comment = {"", ""};
    size_t tokens;
    std::string reason;
    bool is_content_comment = false;
    double processing_time = 0;

    ProcessedComment(RawComment raw_comment) {
        this->raw_comment = raw_comment;
    };

    nlohmann::json to_json() {
        nlohmann::json j;
        j["raw_comment"] = this->raw_comment.to_json();
        j["tokens"] = this->tokens;
        j["reason"] = this->reason;
        j["is_content_comment"] = this->is_content_comment;
        j["processing_time"] = this->processing_time;
        return j;
    }
};

#endif