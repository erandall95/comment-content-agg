#ifndef FROM_ESTIMATE_HPP
#define FROM_ESTIMATE_HPP

#include <string>
#include "../../json/json.hpp" // https://github.com/nlohmann/json

class FromEstimate {
    public:
    uint num_tokens;
    double est_sec;
    uint num_comments;
    FromEstimate(uint num_tokens, double est_sec, uint num_comments) {
        this->num_tokens = num_tokens;
        this->est_sec = est_sec;
        this->num_comments = num_comments;
    };

    nlohmann::json to_json() {
        nlohmann::json j;
        j["num_tokens"] = this->num_tokens; 
        j["est_sec"] = this->est_sec; 
        j["num_comments"] = this->num_comments; 
        return j;
    }
};

#endif