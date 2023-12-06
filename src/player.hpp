#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <format>
#include <iostream>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <utility>

namespace player {
    using std::chrono::time_point;
    using std::chrono::steady_clock;

    struct Song;

    class MusicPlayer {
    private:
        int song_id;
        steady_clock clock;
        std::jthread thread;
        std::deque<int> song_queue;
        std::mutex queue_mutex;
        std::atomic_int elapsed;
        std::atomic_bool paused;
        std::mutex pause_mutex;
        std::mutex wait_mutex;
        std::condition_variable pause_wait;
        std::condition_variable queue_wait;
        time_point<steady_clock> next_second;

        void pause();
        void wait_if_queue_empty();
        void start_thread();

    public:
        std::stop_source stopper;

        MusicPlayer(int initial_song);
        void next_song();
        void add_to_queue(int song_id);
        void set_elapsed(int new_elapsed);
        void toggle_pause();
    };
}
