#ifndef FROM_ENQUEUE_HPP
#define FROM_ENQUEUE_HPP

#include <string>
#include "from_estimate.hpp"

class FromEnqueue {
    public:
    FromEstimate fe = {0, 0, 0};
    std::string uuid;
    FromEnqueue(std::string uuid, FromEstimate fe) {
        this->uuid = uuid;
        this->fe = fe;
    };

    nlohmann::json to_json() {
        nlohmann::json j;
        j["estimate"] = this->fe.to_json(); 
        j["uuid"] = this->uuid;
        return j;
    };
};

#endif