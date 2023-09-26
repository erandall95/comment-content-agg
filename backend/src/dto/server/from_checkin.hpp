#ifndef FROM_CHECKIN_HPP
#define FROM_CHECKIN_HPP

#include <vector>

#include "../../helper_classes/video_details.hpp"

class FromCheckin {
    public:
    std::vector<VideoDetails> in_progress_video_details = {};
    std::vector<VideoDetails> completed_video_details = {};
    FromCheckin() {};

    nlohmann::json to_json() {
        nlohmann::json j;
        for(uint i = 0; i < this->in_progress_video_details.size(); i++) {
            j["in_progress"][i] = this->in_progress_video_details[i].to_json();
        }
        for(uint i = 0; i < this->completed_video_details.size(); i++) {
            j["completed"][i] = this->completed_video_details[i].to_json();
        }
        return j;
    };
};

#endif