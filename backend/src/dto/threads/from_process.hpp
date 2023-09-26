#ifndef FROM_PROCESS_HPP
#define FROM_PROCESS_HPP

#include <vector>
#include <string>
#include "../../helper_classes/processed_comment.hpp"
#include "to_process.hpp"

class FromProcess {
    public:
    std::string uuid;
    double processing_time;
    ProcessedComment processed_comment = {{"", ""}};
    FromProcess(std::string uuid, ProcessedComment processed_comment) {
        this->uuid = uuid;
        this->processed_comment = processed_comment;
    };
};

#endif