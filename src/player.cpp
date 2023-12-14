#include "player.hpp"


namespace player {
    using namespace std::literals::chrono_literals;
    using std::chrono::time_point;
    using std::chrono::steady_clock;

    struct Song {
        const char artist[32];
        const char name[32];
        int duration;
    };

    const std::unordered_map<int, Song> songs {
        {1, {"Toilet Sounds", "Bowel Movement Boys", 15}},
        {2, {"Regurgitating a Hairball", "Neighbor's Cat", 20}},
    };

    void MusicPlayer::pause() {
        std::unique_lock<std::mutex> pause_lock(pause_mutex);
        pause_wait.wait(pause_lock, [&](){ return !paused; });
    }

    void MusicPlayer::wait_if_queue_empty() {
        INFO("Ran out of music to play, waiting for new song")
        while (true)
        {
            {
                std::unique_lock<std::mutex> queue_lock(queue_mutex);
                if (!song_queue.empty() || stopper.stop_requested())
                {
                    return;
                }
            }
            std::this_thread::sleep_for(0.2s);
            INFO("Still waiting...")
        }
    }

    void MusicPlayer::start_player()
    {
        std::string song_name = "";

        std::function<void()> stop_callback = [this]() 
        {
            paused = true;
        };

        std::function<void()> start_callback = [this]()
        {
            paused = false;
        };

        std::function<void()> skip_callback = [this]()
        {
            skip();
        };

        g.start_callback = &start_callback;
        g.stop_callback = &stop_callback;
        g.skip_callback = &skip_callback;

        song_queue.push_back(1);
        song_queue.push_back(2);
        song_queue.push_back(1);
        song_queue.push_back(2);

        while (true)
        {
            if (stopper.stop_requested())
            {
                //break;
            }

            if (!paused)
            {
                end_frame = clock.now();
                delta_t = std::chrono::duration<float, std::ratio<1, 1>>(end_frame - start_frame).count();
                start_frame = clock.now();

                if (delta_t > 0 && delta_t < 1)
                    elapsed += delta_t;

                bool broke = false;
                const auto& [artist, name, duration] = songs.at(song_id);

                if (elapsed > duration)
                    next_song();

                song_name = std::format("{} - {} : {:.1f}", artist, name, elapsed.load());
                g.song_name = song_name;
                g.progress_bar = elapsed / duration;

                g.gui_song_queue.clear();
                for (int si : song_queue)
                {
                    const auto& [a, n, d] = songs.at(si);
                    g.gui_song_queue.push_back(std::format("{} - {} : {}", a, n, d));
                }

            }

            g.draw_gui();
        }
    }

    MusicPlayer::MusicPlayer(int initial_song) : song_id(initial_song) { };

    void MusicPlayer::next_song()
    {
        wait_if_queue_empty();
        INFO("New song detected, acquiring lock to play it")
        std::lock_guard<std::mutex> queue_lock(queue_mutex);
        song_id = song_queue.front();
        song_queue.pop_front();
        elapsed = 0.0f;
        INFO("Lock obtained and skipped to new song")
    }

    void MusicPlayer::skip()
    {
        INFO("Obtaining lock to skip")
        std::lock_guard<std::mutex> queue_lock(queue_mutex);
        INFO("Lock obtained, skipping song if queue isn't empty")
        if (!song_queue.empty())
        {
            song_id = song_queue.front();
            song_queue.pop_front();
            elapsed = 0.0f;
            INFO("Skipped")
            return;
        }
        INFO("Queue was empty, did not skip")
    }

    void MusicPlayer::add_to_queue(int song_id)
    {
        INFO("Obtaining lock to add id {} to queue", song_id)
        {
            std::unique_lock<std::mutex> queue_lock(queue_mutex);
            if (song_queue.size() < MAXIMUM_QUEUE_SIZE)
            {
                song_queue.emplace_back(song_id);
            }
        }
        INFO("Added {} to queue", song_id)
        //queue_wait.notify_all();
    }

    void MusicPlayer::set_elapsed(int new_elapsed)
    {
        elapsed = new_elapsed;
        INFO("Changed elapsed time to {}", new_elapsed);
    }

    void MusicPlayer::toggle_pause()
    {
        paused = !paused;
        pause_wait.notify_all();
    }

    const SendStatus MusicPlayer::get_status() {
        std::lock_guard<std::mutex> queue_lock(queue_mutex);
        SendStatus status
        {
            song_id,
            elapsed,
            paused,
            song_queue,
        };

        return status;
    }
}
