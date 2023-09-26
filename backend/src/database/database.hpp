#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include "../json/json.hpp" // https://github.com/nlohmann/json
// brew install mongo-cxx-driver
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/json.hpp>

#include "../dto/server/from_checkin.hpp"
#include "../helper_classes/video_details.hpp"

class Database {
    private:
    const std::string SERVER_ADDRESS = "mongodb://localhost:20808/";
    const std::string CCA_DB_NAME = "CCA";
    const std::string VIDEO_DETAILS_COLLECTION = "video_details_collection";
    // https://mongocxx.org/mongocxx-v3/tutorial/
    // docker run -d -p 20808:27017 --name cca -v mongo-data:/data/db mongo:latest
    mongocxx::instance instance{};
    mongocxx::uri uri;
    mongocxx::client client;
    mongocxx::collection coll;

    public:
    Database() {
        try {
            this->uri = mongocxx::uri(this->SERVER_ADDRESS);
            this->client = mongocxx::client(this->uri);
            this->coll = client[CCA_DB_NAME][VIDEO_DETAILS_COLLECTION];
            std::cout << "Connected to MongoDB!" << std::endl;
        } catch (const std::exception& e) {
            // Handle errors.
            std::cout<< "Exception: " << e.what() << std::endl;
            exit(1);
        }
    };

    // overwrites, or create a new one
    void update_document(VideoDetails video_details) {
        try {
            // Serialize the RawComment objects to BSON
            bsoncxx::builder::basic::array raw_comments_array;
            for (const RawComment& raw_comment : video_details.raw_comments) {
                bsoncxx::builder::basic::document raw_comment_doc;
                raw_comment_doc.append(bsoncxx::builder::basic::kvp("rawComment", raw_comment.rawComment),
                                        bsoncxx::builder::basic::kvp("publishedAt", raw_comment.publishedAt));
                raw_comments_array.append(raw_comment_doc);
            }
            // Serialize the ProcessedComment objects to BSON
            bsoncxx::builder::basic::array processed_comments_array;
            for (const ProcessedComment& processed_comment : video_details.processed_comments) {
                bsoncxx::builder::basic::document processed_comment_doc;
                processed_comment_doc.append(bsoncxx::builder::basic::kvp("raw_comment", bsoncxx::builder::basic::make_document(
                                            bsoncxx::builder::basic::kvp("rawComment", processed_comment.raw_comment.rawComment),
                                            bsoncxx::builder::basic::kvp("publishedAt", processed_comment.raw_comment.publishedAt))),
                                        bsoncxx::builder::basic::kvp("tokens", static_cast<int32_t>(processed_comment.tokens)),
                                        bsoncxx::builder::basic::kvp("reason", processed_comment.reason),
                                        bsoncxx::builder::basic::kvp("isContentComment", processed_comment.is_content_comment),
                                        bsoncxx::builder::basic::kvp("processing_time", processed_comment.processing_time));
                processed_comments_array.append(processed_comment_doc);
            }
            // Serialize the VideoDetails object to BSON
            bsoncxx::document::value doc_value = bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("id", video_details.id),
                bsoncxx::builder::basic::kvp("uuid", video_details.uuid),
                bsoncxx::builder::basic::kvp("title", video_details.title),
                bsoncxx::builder::basic::kvp("description", video_details.description),
                bsoncxx::builder::basic::kvp("optional_description", video_details.optional_description),
                bsoncxx::builder::basic::kvp("raw_comments", raw_comments_array),
                bsoncxx::builder::basic::kvp("processed_comments", processed_comments_array),
                bsoncxx::builder::basic::kvp("to_process", video_details.to_process),
                bsoncxx::builder::basic::kvp("processed", video_details.processed)
            );
            // Construct the filter to find the document by id
            bsoncxx::document::value filter = bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("id", video_details.id)
            );
            // Use $set to update the document or insert a new one if it doesn't exist
            bsoncxx::document::value update = bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("$set", doc_value)
            );
            // Update or insert the document based on the filter
            mongocxx::options::update options;
            options.upsert(true);  // Create the document if it doesn't exist
            // Perform the update operation
            this->coll.update_one(filter.view(), update.view(), options);
            std::cout << "Document updated/created successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << std::endl;
        }
    };

    // find and return a document
    VideoDetails find_document_by_id(std::string id) {
        VideoDetails video_details = VideoDetails();
        try {
            bsoncxx::document::value filter = bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("id", id)
            );
            auto cursor = this->coll.find_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("id", id)));
            if(!(cursor == bsoncxx::stdx::nullopt)) {
                // NOTE: probably a better way to do all this
                nlohmann::json j = nlohmann::json::parse(bsoncxx::to_json(cursor.value()));
                video_details.uuid = j["uuid"];
                video_details.id = j["id"];
                video_details.title = j["title"];
                video_details.description = j["description"];
                video_details.optional_description = j["optional_description"];
                for (uint i = 0; i < j["raw_comments"].size(); i++) {
                    RawComment rc = RawComment(j["raw_comments"][i]["rawComment"], j["raw_comments"][i]["publishedAt"]);
                    video_details.raw_comments.push_back(rc);
                }
                for(uint i = 0; i < j["processed_comments"].size(); i++) {
                    ProcessedComment pc = ProcessedComment(RawComment(j["processed_comments"][i]["raw_comment"]["rawComment"], j["processed_comments"][i]["raw_comment"]["publishedAt"]));
                    pc.is_content_comment = j["processed_comments"][i]["isContentComment"];
                    pc.processing_time = j["processed_comments"][i]["processing_time"];
                    pc.reason = j["processed_comments"][i]["reason"];
                    pc.tokens = j["processed_comments"][i]["tokens"];
                    video_details.processed_comments.push_back(pc);
                }
                video_details.to_process = j["to_process"];
                video_details.processed = j["processed"];
            }
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << std::endl;
        }
        // Return an empty VideoDetails object if not found
        return video_details;
    };

    void get_all_video_docs(FromCheckin *fc) {
        try {
            auto cursor_all = this->coll.find({});
            for (auto doc : cursor_all) {
                VideoDetails video_details = VideoDetails();
                // NOTE: probably a better way to do all this
                nlohmann::json j = nlohmann::json::parse(bsoncxx::to_json(doc));
                video_details.uuid = j["uuid"];
                video_details.id = j["id"];
                video_details.title = j["title"];
                video_details.description = j["description"];
                video_details.optional_description = j["optional_description"];
                for(uint i = 0; i < j["raw_comments"].size(); i++) {
                    RawComment rc = RawComment(j["raw_comments"][i]["rawComment"], j["raw_comments"][i]["publishedAt"]);
                    video_details.raw_comments.push_back(rc);
                }
                for(uint i = 0; i < j["processed_comments"].size(); i++) {
                    ProcessedComment pc = ProcessedComment(RawComment(j["processed_comments"][i]["raw_comment"]["rawComment"], j["processed_comments"][i]["raw_comment"]["publishedAt"]));
                    pc.is_content_comment = j["processed_comments"][i]["isContentComment"];
                    pc.processing_time = j["processed_comments"][i]["processing_time"];
                    pc.reason = j["processed_comments"][i]["reason"];
                    pc.tokens = j["processed_comments"][i]["tokens"];
                    video_details.processed_comments.push_back(pc);
                }
                video_details.to_process = j["to_process"];
                video_details.processed = j["processed"];
                fc->completed_video_details.push_back(video_details);
            }
        } catch(const std::exception& e) {
            std::cerr << e.what() << '\n';
        }
        
    }

};


#endif