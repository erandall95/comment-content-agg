#ifndef WORKER_HANDLER_PROCESS_HPP
#define WORKER_HANDLER_PROCESS_HPP

#include <vector>
#include <tuple>
#include <string>
#include <map>

#include "../concurrentqueue/concurrentqueue.h" // https://github.com/cameron314/concurrentqueue
#include "../concurrentqueue/blockingconcurrentqueue.h"

#include "../database/database.hpp"
#include "../dto/threads/from_process.hpp"
#include "../dto/threads/to_process.hpp"
#include "../dto/server/from_checkin.hpp"
#include "../helper_classes/video_details.hpp"
#include "worker_process.hpp"

class WorkerProcessHandler {
    uint process_count = 0;
    Database *db;
    std::vector<WorkerProcess> processes = {};
    // std::thread handler_thread;
    std::vector<std::shared_ptr<moodycamel::BlockingConcurrentQueue<ToProcess>>> to_process_queues;
    moodycamel::ConcurrentQueue<FromProcess> from_process_queue;
    double tps = 46; //how many tokens per second this handler can process M1 Pro@ ~30-150tps
    std::map<std::string, VideoDetails> data;

    std::tuple<uint, uint> get_shortest_queue_and_size() {
        uint target_queue = 0;
        uint target_size = this->to_process_queues[target_queue]->size_approx();
        for(uint i = 0; i < this->process_count; i++) {
            if(this->to_process_queues[i]->size_approx() < target_size) {
                target_queue = i;
                target_size = this->to_process_queues[i]->size_approx();
            }
        }
        return std::tuple(target_queue, target_size);
    };

    int get_total_queues_size() {
        uint total_size = 0;
        for(uint i = 0; i < this->process_count; i++) {
            total_size += this->to_process_queues[i]->size_approx();
        }
        return total_size;
    };

    void handleProcesses() {
        for(uint i = 0; i < this->process_count; i++) {
            this->processes[i].run();
        }
        FromProcess FromProcess = {"", ProcessedComment({"",""})};
        printf("Handling Processes\n");
        while(1) {
            bool found = this->from_process_queue.try_dequeue(FromProcess);
            if(found) {
                this->tps += FromProcess.processed_comment.tokens / FromProcess.processed_comment.processing_time;
                this->tps /= 2;
                std::cout << "tokens processed per second: " << this->tps << std::endl;
                // TODO
                this->data[FromProcess.uuid].processed++;
                if(FromProcess.processed_comment.is_content_comment) {
                    this->data[FromProcess.uuid].processed_comments.push_back(FromProcess.processed_comment);
                }
                // iterate through data and any completely processed videos should be put in the db and removed from this map
                if(this->data[FromProcess.uuid].processed == this->data[FromProcess.uuid].raw_comments.size()) {
                    double completed_in = 0;
                    for(uint i = 0; i < this->data[FromProcess.uuid].processed; i++) {
                        completed_in += this->data[FromProcess.uuid].processed_comments[i].processing_time;
                    }
                    std::cout << "done in " << completed_in << std::endl;
                    // insert into db, remove from map
                    this->db->update_document(this->data[FromProcess.uuid]);
                    this->data.erase(FromProcess.uuid);
                    std::cout << this->data.size() << " videos left to parse" << std::endl;
                }
            }
        }
    };

    public:
    WorkerProcessHandler();
    WorkerProcessHandler(uint process_count, Database *db) {
        this->process_count = process_count;
        this->db = db;
        for(uint i = 0; i < this->process_count; i++) {
            auto q = std::make_shared<moodycamel::BlockingConcurrentQueue<ToProcess>>();
            this->to_process_queues.push_back(q);
            WorkerProcess ai = WorkerProcess(*this->to_process_queues[i], this->from_process_queue);
            this->processes.push_back(ai);
        }
    };

    void pass_comments_to_processors(VideoDetails videoDetails) {
        this->data[videoDetails.uuid] = videoDetails;
        ToProcess to_process = ToProcess("", "", {{"",""}});
        if(videoDetails.optional_description.size() > 1) {
            to_process = ToProcess(videoDetails.uuid, videoDetails.optional_description, videoDetails.raw_comments);
        } else {
            to_process = ToProcess(videoDetails.uuid, videoDetails.description, videoDetails.raw_comments);
        }
        std::tuple<uint, uint> target = this->get_shortest_queue_and_size();
        uint target_queue = std::get<0>(target);
        uint target_size = std::get<1>(target);
        this->to_process_queues[target_queue]->enqueue(to_process);
    };

    void est_tokens(std::string comment, std::string description, uint *num_tokens) {
        // no idea what happens when process 0 is running and estimate is called. Possible thread issue?
        this->processes[0].tokenize(comment, description, num_tokens);
    }

    double est_processing_time(uint num_tokens) {
        if(this->tps == 0) {
            return (num_tokens / -1);
        } else {
            return (num_tokens / this->tps);
        }
    }

    void checkin(FromCheckin *fc) {
        // *fc->in_progress_video_details
        for(auto i = this->data.begin(); i != this->data.end(); i++) {
            fc->in_progress_video_details.push_back(i->second);
        }
    }

    void run() {
        // this->handler_thread = std::thread(&WorkerProcessHandler::handleProcesses, this);
        // this->handler_thread.detach();
        std::thread processing_thread = std::thread(&WorkerProcessHandler::handleProcesses, this);
        processing_thread.detach();
    };
};


#endif