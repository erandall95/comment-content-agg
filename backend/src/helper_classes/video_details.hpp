#ifndef VIDEO_DETAILS_HPP
#define VIDEO_DETAILS_HPP

#include <string>
#include <vector>

#include "../json/json.hpp"

#include "raw_comment.hpp"
#include "processed_comment.hpp"

class VideoDetails {
    public:
    std::string uuid;
    std::string id;
    std::string title;
    std::string description;
    std::string optional_description;
    std::vector<RawComment> raw_comments = {};
    std::vector<ProcessedComment> processed_comments = {};
    int to_process = 0;
    int processed = 0;
    // Default constructor
    VideoDetails() {
        // Initialize member variables with default values
        uuid = "";
        id = "";
        title = "";
        description = "";
        optional_description = "";
    };
    VideoDetails(std::string uuid, std::string id, std::string optional_description) {
        this->uuid = uuid;
        this->id = id;
        this->optional_description = optional_description;
    };

    nlohmann::json to_json() {
        nlohmann::json j;
        j["uuid"] = this->uuid;
        j["id"] = this->id;
        j["title"] = this->title;
        j["description"] = this->description;
        j["optional_description"] = this->optional_description;
        for(uint i = 0; i < this->raw_comments.size(); i++) {
            j["raw_comments"][i] = this->raw_comments[i].to_json();
        }
        for(uint i = 0; i < this->processed_comments.size(); i++) {
            j["processed_comments"][i] = this->processed_comments[i].to_json();
        }
        j["to_process"] = this->to_process;
        j["processed"] = this->processed;
        return j;
    }
};

#endif