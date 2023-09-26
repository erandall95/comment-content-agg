#ifndef TO_SERVER_HPP
#define FROM_ESTIMATE_HPP

#include <string>
#include "../../json/json.hpp" // https://github.com/nlohmann/json

class ToServer {
    public:
    std::string video_id;
    std::string optional_description;
    FromEstimate(std::string video_id, std::string optional_description, std::string uuid) {
        this->video_id = video_id;
        this->optional_description = optional_description;
        this->uuid = uuid;
    };
};

#endif