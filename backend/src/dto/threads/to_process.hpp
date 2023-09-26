#ifndef TO_PROCESS_HPP
#define TO_PROCESS_HPP

#include <vector>
#include <string>
#include "../../helper_classes/raw_comment.hpp"

class ToProcess {
    public:
    std::string uuid;
    std::string description;
    std::vector<RawComment> raw_comments;
    ToProcess(std::string uuid, std::string description, std::vector<RawComment> raw_comments) {
        this->uuid = uuid;
        this->description = description;
        this->raw_comments = raw_comments;  
    };
};

#endif