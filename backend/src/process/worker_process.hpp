#ifndef WORKER_PROCESS_HPP
#define WORKER_PROCESS_HPP

#include <chrono>
#include <vector>
#include <string>

#include "../llama.cpp/llama.h" //https://github.com/ggerganov/llama.cpp
#include "../llama.cpp/common/common.h"
#include "../concurrentqueue/concurrentqueue.h" // https://github.com/cameron314/concurrentqueue
#include "../concurrentqueue/blockingconcurrentqueue.h"

#include "../dto/threads/from_process.hpp"
#include "../dto/threads/to_process.hpp"
#include "../helper_classes/processed_comment.hpp"
#include "../helper_classes/video_details.hpp"

class WorkerProcess {
    const int CONTEX_SIZE = 2048;
    private:
    // std::thread processing_thread;
    // https://huggingface.co/models?pipeline_tag=question-answering&sort=downloads
    const std::string model_path = "models/wizardlm-13b-v1.1-superhot-8k.gguf.q4_0.bin";
    llama_model * model;
    llama_context_params ctx_params;

    moodycamel::BlockingConcurrentQueue<ToProcess> &in_queue;
    moodycamel::ConcurrentQueue<FromProcess> &out_queue;

    std::string prompt_builder(std::string comment, std::string video_description) {
        // std::string prompt = "prompt: The comment '"+ comment + "' on a video about " + video_description + " is or is not explicitly a suggestion, recommendation or request for future videos? Start the response with 'is' or 'is not' accordingly.\n response:";
        // std::string prompt = "prompt: The comment '"+ comment + "' on a video about " + video_description + " is or is not explicitly a suggestion, recommendation or request for a new video in the future?\n response:";
        std::string prompt = "prompt: The comment `"+ comment + "` on a video about `" + video_description + "` is or is not directly a suggestion, recommendation or request for a new video in the future? Start the response with `is` or `is not` and state why.\n response:"; //good
        // std::string prompt = "prompt: The comment '"+ comment + "' on a video about " + video_description + " is or is not directly a suggestion, recommendation or request for a new video in the future? Start the response with 'Is' or 'Not'.\n response:";
        // std::string prompt = "prompt: The comment '"+ comment + "' on a video about " + video_description + " is or is not directly a suggestion, recommendation or request for a new video in the future? Respond in this format: 'Is/Is not - reasoning'.\n response:";
        return prompt;
    };

    std::string prompt(std::string input, size_t *token_count) {
        llama_context * ctx = llama_new_context_with_model(this->model, this->ctx_params);
        std::string response = "";
        // tokenize the prompt
        std::vector<llama_token> tokens_list;
        tokens_list = llama_tokenize(ctx, input, true);
        *token_count = tokens_list.size(); //store number of tokens
        const int max_context_size = llama_n_ctx(ctx);
        const int max_tokens_list_size = max_context_size - 4;
        if ((int) tokens_list.size() > max_tokens_list_size) {
            fprintf(stderr, "%s: error: prompt too long (%d tokens, max %d)\n", __func__, (int) tokens_list.size(), max_tokens_list_size);
            return "";
        }
        // fprintf(stderr, "\n\n");
        // for (auto id : tokens_list) {
        //     fprintf(stderr, "%s", llama_token_to_piece(ctx, id).c_str());
        // }
        // fflush(stderr);

        // main loop
        // The LLM keeps a contextual cache memory of previous token evaluation.
        // Usually, once this cache is full, it is required to recompute a compressed context based on previous
        // tokens (see "infinite text generation via context swapping" in the main example), but in this minimalist
        // example, we will just stop the loop once this cache is full or once an end of stream is detected.
        int n_gen = std::min(32, max_context_size);
        n_gen = max_context_size;
        while(llama_get_kv_cache_token_count(ctx) < n_gen) {
            // evaluate the transformer
            if (llama_eval(ctx, tokens_list.data(), int(tokens_list.size()), llama_get_kv_cache_token_count(ctx), get_num_physical_cores())) {
                fprintf(stderr, "%s : failed to eval\n", __func__);
                return "";
            }
            tokens_list.clear();
            // sample the next token
            llama_token new_token_id = 0;
            auto logits  = llama_get_logits(ctx);
            auto n_vocab = llama_n_vocab(ctx);
            std::vector<llama_token_data> candidates;
            candidates.reserve(n_vocab);
            for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
                candidates.emplace_back(llama_token_data{ token_id, logits[token_id], 0.0f });
            }
            llama_token_data_array candidates_p = { candidates.data(), candidates.size(), false };
            new_token_id = llama_sample_token_greedy(ctx , &candidates_p);
            // is it an end of stream ?
            if (new_token_id == llama_token_eos(ctx)) {
                // fprintf(stderr, " [end of text]\n");
                break;
            }
            // print the new token :
            // printf("%s", llama_token_to_piece(ctx, new_token_id).c_str());
            // fflush(stdout);
            response.append(llama_token_to_piece(ctx, new_token_id).c_str());
            // push this new token for next evaluation
            tokens_list.push_back(new_token_id);
        }
        llama_free(ctx);
        return response;
    }

    public:
    WorkerProcess(moodycamel::BlockingConcurrentQueue<ToProcess> &in_queue, moodycamel::ConcurrentQueue<FromProcess> &out_queue): in_queue(in_queue), out_queue(out_queue) {
        // this->in_queue = in_queue;
        // this->out_queue = out_queue;
        llama_backend_init(false);
        this->ctx_params = {
            /*.seed                        =*/ LLAMA_DEFAULT_SEED,
            /*.n_ctx                       =*/ this->CONTEX_SIZE,
            /*.n_batch                     =*/ this->CONTEX_SIZE,
            /*.n_gpu_layers                =*/ 1,
            /*.main_gpu                    =*/ 0,
            /*.tensor_split                =*/ nullptr,
            /*.rope_freq_base              =*/ 10000.0f,
            /*.rope_freq_scale             =*/ 1.0f,
            /*.progress_callback           =*/ nullptr,
            /*.progress_callback_user_data =*/ nullptr,
            /*.low_vram                    =*/ false,
            /*.mul_mat_q                   =*/ true,
            /*.f16_kv                      =*/ true,
            /*.logits_all                  =*/ false,
            /*.vocab_only                  =*/ false,
            /*.use_mmap                    =*/ true,
            /*.use_mlock                   =*/ false,
            /*.embedding                   =*/ false,
        };
        this->model = llama_load_model_from_file(this->model_path.c_str(), ctx_params);
        if (model == NULL) {
            fprintf(stderr , "%s: error: unable to load model\n" , __func__);
        }
    }

    void tokenize(std::string comment, std::string description, uint *num_tokens) {
        std::string psuedo_input = this->prompt_builder(comment, description);
        llama_token *tokens = new llama_token[this->CONTEX_SIZE];
        *num_tokens += llama_tokenize_with_model(this->model, psuedo_input.c_str(), psuedo_input.size(), tokens, this->CONTEX_SIZE, true);
    }

    void process() {
        printf("worker processing\n");
        const std::string TARGET = "is not";
        while(1) {
            ToProcess toProcess = {"", "", {}};
            this->in_queue.wait_dequeue(toProcess);
            for(uint i = 0; i < toProcess.raw_comments.size(); i++) {
                ProcessedComment processed_comment = {toProcess.raw_comments[i]};
                std::string input = this->prompt_builder(toProcess.raw_comments[i].rawComment, toProcess.description);
                auto start = std::chrono::high_resolution_clock::now(); // Get the starting time
                std::string resp = this->prompt(input, &processed_comment.tokens);
                auto end = std::chrono::high_resolution_clock::now(); // Get the ending time
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start); // Calculate the duration
                processed_comment.processing_time = duration.count() / 1e6; // Convert microseconds to seconds
                processed_comment.reason = resp; //store the resposne from the LLM
                // determine if it is a content suggestion "is not" should appear early in the response if it is not a content comment
                std::string searchStringLower = resp.substr(0, 12); // Take the first 12 characters
                std::transform(searchStringLower.begin(), searchStringLower.end(), searchStringLower.begin(), ::tolower); // Convert to lowercase
                if (searchStringLower.find(TARGET) == std::string::npos) {
                    processed_comment.is_content_comment = true;
                }
                // done processing this comment, send it to the handler
                this->out_queue.enqueue(FromProcess(toProcess.uuid, processed_comment));
            }
        }
    }

    void run() {
        // this->processing_thread = std::thread(&AI::process, this);
        // this->processing_thread.detach();
        std::thread processing_thread = std::thread(&WorkerProcess::process, this);
        processing_thread.detach();
    }

    void deinit() {
        // this->processing_thread.join();
        llama_free_model(this->model);
        llama_backend_free();
    }
};

#endif