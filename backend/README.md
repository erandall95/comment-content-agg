# Building
* `cd src`
* `git clone https://github.com/r-lyeh-archived/sole.git`
* `git clone https://github.com/nlohmann/json.git`
* `git clone https://github.com/cameron314/concurrentqueue`
* `git clone https://github.com/ggerganov/llama.cpp`
* `git clone https://github.com/uNetworking/uWebSockets.git --recurse-submodules`
* `cd into uWebsockets/src && make`
* `brew install mongo-cxx-driver`
* probably something I'm forgetting... Like how this too can be made a script

# Running
 * download a .gguf (or convert) model from huggingface of your choice (I've used wizardlm-1b-v1.1-superhot-8k.gguf.q4_0.bin) and place it in models/
 * `docker run -d -p 20808:27017 --name cca -v mongo-data:/data/db mongo:latest`
 * create a DB named "CCA" and a collection named "video_details_collection"
 * run `./run.sh`

# TODO
So much...
 * new endpoint to server html gui webpage
 * docker compose
 * server gets settings (api key, number process, etc from DB)
 * new endpoint to get server stats
 * if a video was preiously processed, only get new comments
 * CUDA suport
 * cmake builds uWebSockets
 * cmake better support llama build (for cuda, cpu/ metal etc)
 * worker_process_handler should not have inital tps set, should be fingured out upon object creation
 * worker_process_handler will segfault if passed a process count greater than 1 - this may not be fixable, but I think there is a solution
 * to_server DTO isn't being used but should be - similarly, server should handle bad input and other errors
 * ProcessedComment class has RawComment class as variable. In VideoDetails class this results in a lot of string duplication
 * database as own micro service
 * checking endpoint should also give a time estimate
 * impletment a logger or logging service
 * estimate tokens service/fuctionality can tokenize comments so it only has to be done once - this would help with estimates. Or sum tokens of all remaining comments (would have to filter which aren't done) 
 * so much general clean up
 * add feature if video is queued for processing, prevent re-queue
