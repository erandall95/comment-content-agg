#ifndef GOOGLE_API_HPP
#define GOOGLE_API_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <curl/curl.h>
#include <iostream>
#include <cstdlib>

#include "../json/json.hpp"

#include "../helper_classes/video_details.hpp"
#include "../helper_classes/raw_comment.hpp"

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
};

void get_video_details(std::string google_api_key, VideoDetails* video_details) {
    std::string base_url = "https://youtube.googleapis.com/youtube/v3/videos?part=snippet%2Cstatistics%2CtopicDetails";
    std::string url = base_url + "&key=" + google_api_key + "&id=" + video_details->id;
    // perform http request
    CURL *hnd = curl_easy_init();
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());
    // // Response data
    std::string response_data;
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &response_data);
    CURLcode ret = curl_easy_perform(hnd);
    // Check if the request was successful (HTTP status code 200)
    if (ret != CURLE_OK) {
        std::cerr << "Failed to fetch data: " << curl_easy_strerror(ret) << std::endl;
        // Handle the error as needed
        // For example, you can set video_details members to some default values or throw an exception
    } else {
        // Parse the JSON
        nlohmann::json data = nlohmann::json::parse(response_data.c_str());
        video_details->title = data["items"][0]["snippet"]["title"];
        video_details->description = data["items"][0]["snippet"]["description"];
    }
    // Cleanup and free resources
    curl_easy_cleanup(hnd);
};

void get_video_comments(std::string google_api_key, std::string nextToken, VideoDetails* video_details) {
    std::string base_url = "https://youtube.googleapis.com/youtube/v3/commentThreads?part=snippet&maxResults=100";
    std::string url = base_url + "&videoId=" + video_details->id + "&key=" + google_api_key + "&pageToken=" + nextToken;
    // perform http request
    CURL *hnd = curl_easy_init();
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());
    // // Response data
    std::string response_data;
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &response_data);
    CURLcode ret = curl_easy_perform(hnd);
    // Check if the request was successful (HTTP status code 200)
    if (ret != CURLE_OK) {
        std::cerr << "Failed to fetch data: " << curl_easy_strerror(ret) << std::endl;
        // Handle the error as needed
        // For example, you can set video_details members to some default values or throw an exception
    } else {
        // Parse the JSON
        nlohmann::json data = nlohmann::json::parse(response_data.c_str());
        // push all comments onto video_description comments
        for(int i = 0; i < data["items"].size(); i++) {
            RawComment raw_comment = RawComment(data["items"][i]["snippet"]["topLevelComment"]["snippet"]["textOriginal"], data["items"][i]["snippet"]["topLevelComment"]["snippet"]["publishedAt"]);
            video_details->raw_comments.push_back(raw_comment);
            video_details->to_process++;
        }
        if(!(data["nextPageToken"].is_null())) {
            std::string nextToken = data["nextPageToken"];
            get_video_comments(google_api_key, nextToken, video_details);
        }
    }
    // Cleanup and free resources
    curl_easy_cleanup(hnd);
}

#endif