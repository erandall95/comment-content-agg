# Comment Content Aggregator
![screenshot](Screenshot.png)

Filter comments though an LLM to find users that are suggesting, requesting or otherwise generating, ideas for future content based on the description of a youtube video.

Enter the URL to a youtube video and optionally enter a description for the video to begin parsing comments that may be ideas for future content. If no description is entered then the description given to youtube is used.

Running on an M1 macbook resutls in ~30-160 tokens processed per second, so a comment take 4-11 seconds to determine content relevance.
