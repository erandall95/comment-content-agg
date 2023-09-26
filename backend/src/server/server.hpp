#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <fstream>

#include "../uWebSockets/uSockets/src/libusockets.h" // https://github.com/uNetworking/uWebSockets
#include "../uWebSockets/src/App.h"
#include "../json/json.hpp" // https://github.com/nlohmann/json

#include "../sole/sole.hpp" // https://github.com/r-lyeh-archived/sole
#include "../database/database.hpp"
#include "../helper_classes/video_details.hpp"
#include "../process/worker_handler_process.hpp"
#include "../google_api/google_api.hpp"
#include "../dto/server/from_estimate.hpp"
#include "../dto/server/from_enqueue.hpp"
#include "../dto/server/from_checkin.hpp"

class Server {
    private:
    WorkerProcessHandler *wph;
    Database *db;
    nlohmann::json *config;
    
    // submit video and comments for process estimation, return tokens and time
    // TODO: error handling
    void server_estimate_video(std::string body, std::string *resp) {
        nlohmann::json body_json = nlohmann::json::parse(body);
        FromEstimate fe = FromEstimate(0, 0, 0);
        std::string video_id = (std::string)body_json["video_id"];
        std::string optional_description = (std::string)body_json["optional_description"];
        VideoDetails vd = VideoDetails("", video_id, optional_description);
        std::string GOOGLE_API_KEY = (*config)["secrets"]["GOOGLE_API_KEY"];
        get_video_details(GOOGLE_API_KEY, &vd);
        get_video_comments(GOOGLE_API_KEY, "", &vd);
        // tokenize all comments
        fe.num_comments = vd.raw_comments.size();
        for(uint i = 0; i < fe.num_comments; i++) {
            if(vd.optional_description.size() > 1) {
                wph->est_tokens(vd.raw_comments[i].rawComment, vd.optional_description, &fe.num_tokens);
            } else {
                wph->est_tokens(vd.raw_comments[i].rawComment, vd.description, &fe.num_tokens);
            }
        }
        // negative value represents unknown as wph has never processed tokens
        fe.est_sec = wph->est_processing_time(fe.num_tokens);
        // num_tokens, est_time, num_comments
        *resp = fe.to_json().dump();
    };

    // submit video and comments for processing, return uuid
    // uuid is key in processing map
    void server_enqueue_video(std::string body, std::string *resp) {
        sole::uuid u4 = sole::uuid4();
        FromEnqueue fe = FromEnqueue(u4.str(), FromEstimate(0, 0, 0));
        nlohmann::json body_json = nlohmann::json::parse(body);
        std::string video_id = (std::string)body_json["video_id"];
        std::string optional_description = (std::string)body_json["optional_description"];
        VideoDetails vd = VideoDetails(u4.str(), video_id, optional_description);

        std::string GOOGLE_API_KEY = (*config)["secrets"]["GOOGLE_API_KEY"];
        get_video_details(GOOGLE_API_KEY, &vd);
        get_video_comments(GOOGLE_API_KEY, "", &vd);

        // // TODO: streamline this as it's duped from server_estimate_video
        fe.fe.num_comments = vd.raw_comments.size();
        for(uint i = 0; i < fe.fe.num_comments; i++) {
            if(vd.optional_description.size() > 1) {
                wph->est_tokens(vd.raw_comments[i].rawComment, vd.optional_description, &fe.fe.num_tokens);
            } else {
                wph->est_tokens(vd.raw_comments[i].rawComment, vd.description, &fe.fe.num_tokens);
            }
        }
        // negative value represents unknown as wph has never processed tokens
        fe.fe.est_sec = wph->est_processing_time(fe.fe.num_tokens);
        wph->pass_comments_to_processors(vd);
        *resp = fe.to_json().dump();
    };

    // get status of video comment proccessing
    void server_get_all_video_docs(std::string *resp) {
        FromCheckin fc = FromCheckin();
        wph->checkin(&fc);
        db->get_all_video_docs(&fc);
        *resp = fc.to_json().dump();
    };
    public:
    Server(nlohmann::json *config, WorkerProcessHandler *wph, Database *db) {
        this->wph = wph;
        this->db = db;
        this->config = config;
        wph->run();
    };

    void run() {
        const int PORT = 3001;
        uWS::App().post("/estimate", [this](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
            // Add CORS headers
            res->writeHeader("Access-Control-Allow-Origin", "*"); // Allow requests from any origin
            res->writeHeader("Access-Control-Allow-Methods", "GET, POST"); // Define allowed HTTP methods
            res->writeHeader("Access-Control-Allow-Headers", "Origin,Content-Type,Accept,Content-Length,Accept-Language,Accept-Encoding,Connection,Access-Control-Allow-Origin"); // Define allowed headers
            std::string body = "";
            // Set up a data callback to receive the request body
            res->onData([this, res, &body](std::string_view chunk, bool isEnd) {
                // Append the chunk to the body vector
                body += chunk;
                // If it's the end of the request, process the body and send a response
                if (isEnd) {
                    std::string resp = "";
                    this->server_estimate_video(body, &resp);
                    res->end(resp);
                }
            });
            res->onAborted([]() {
                printf("aborted\n");
            });
        }).post("/process", [this](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
            // Add CORS headers
            res->writeHeader("Access-Control-Allow-Origin", "*"); // Allow requests from any origin
            res->writeHeader("Access-Control-Allow-Methods", "GET, POST"); // Define allowed HTTP methods
            res->writeHeader("Access-Control-Allow-Headers", "Origin,Content-Type,Accept,Content-Length,Accept-Language,Accept-Encoding,Connection,Access-Control-Allow-Origin"); // Define allowed headers
            std::string body = "";
            // Set up a data callback to receive the request body
            res->onData([this, res, &body](std::string_view chunk, bool isEnd) {
                // Append the chunk to the body vector
                body += chunk;
                // If it's the end of the request, process the body and send a response
                if (isEnd) {
                    std::string resp = "";
                    this->server_enqueue_video(body, &resp);
                    res->end(resp);
                }
            });
            res->onAborted([]() {
                printf("aborted\n");
            });
        }).get("/getDocs", [this](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
            // Add CORS headers
            res->writeHeader("Access-Control-Allow-Origin", "*"); // Allow requests from any origin
            res->writeHeader("Access-Control-Allow-Methods", "GET, POST"); // Define allowed HTTP methods
            res->writeHeader("Access-Control-Allow-Headers", "Origin,Content-Type,Accept,Content-Length,Accept-Language,Accept-Encoding,Connection,Access-Control-Allow-Origin"); // Define allowed headers
            res->onAborted([]() {
                printf("aborted\n");
            });
            std::string resp = "";
            this->server_get_all_video_docs(&resp);
            res->end(resp);
        }).get("/", [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
            // Add CORS headers
            res->writeHeader("Access-Control-Allow-Origin", "*"); // Allow requests from any origin
            res->writeHeader("Access-Control-Allow-Methods", "GET, POST"); // Define allowed HTTP methods
            res->writeHeader("Access-Control-Allow-Headers", "Origin,Content-Type,Accept,Content-Length,Accept-Language,Accept-Encoding,Connection,Access-Control-Allow-Origin"); // Define allowed headers
            res->writeStatus("200 OK");
            res->writeHeader("Content-Type", "text/html; charset=utf-8");
            const int numParentDirs = 4;
            // Calculate the absolute path to the index.html file in the React.js build directory
            std::string indexPath = __FILE__;
            // Navigate up to the root directory by removing segments from the end of the path
            for (int i = 0; i < numParentDirs; ++i) {
                size_t lastSlashPos = indexPath.find_last_of('/');
                if (lastSlashPos != std::string::npos) {
                    indexPath = indexPath.substr(0, lastSlashPos);
                } else {
                    res->end("Error: Invalid file path");
                    return;
                }
            }
            // Append the path to the React.js build directory and index.html
            indexPath += "/frontend/build/index.html";
            std::ifstream file(indexPath);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                res->end(content.c_str(), content.length());
            } else {
                res->end("Error: Unable to open index.html");
            }
        }).get("/*", [](auto *res, auto *req) {
            // Serve other static files (CSS, JS, images) from the build directory
            const int numParentDirs = 4;
            // Calculate the absolute path to the index.html file in the React.js build directory
            std::string indexPath = __FILE__;
            // Navigate up to the root directory by removing segments from the end of the path
            for (int i = 0; i < numParentDirs; ++i) {
                size_t lastSlashPos = indexPath.find_last_of('/');
                if (lastSlashPos != std::string::npos) {
                    indexPath = indexPath.substr(0, lastSlashPos);
                } else {
                    res->end("Error: Invalid file path");
                    return;
                }
            }
            // Append the path to the React.js build directory and index.html
            indexPath += "/frontend/build/" + (std::string)req->getUrl();
            std::ifstream file(indexPath);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                res->end(content.c_str(), content.length());
            } else {
                res->end("Error: File not found");
            }
        }).listen(PORT, [](auto *listen_socket) {
            if (listen_socket) {
                std::cout << "Listening on port " << PORT << std::endl;
                std::cout << "served at " << "http://localhost:" << PORT << "/" << std::endl;
            }
        }).run();
    };
};

#endif