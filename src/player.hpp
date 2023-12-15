#pragma once

#include "logging.hpp"
#include "message_types.hpp"
#include "gui.hpp"

#include <array>
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

#include "Gui.hpp"

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
        std::atomic<float> elapsed;
        std::atomic_bool paused;
        std::mutex pause_mutex;
        std::mutex wait_mutex;
        std::condition_variable pause_wait;
        std::condition_variable queue_wait;

        hnoker::Gui g;

        time_point<steady_clock> start_frame;
        time_point<steady_clock> end_frame;
        float delta_t = 0.0f;

        void pause();
        void wait_if_queue_empty();

    public:
        std::stop_source stopper;

        MusicPlayer(int initial_song);
        void start_player(ClientList& list);
        void next_song();
        void skip();
        void add_to_queue(int song_id);
        void set_elapsed(int new_elapsed);
        void toggle_pause();
        const SendStatus get_status();
        void set_status(const SendStatus& status);
    };
}
