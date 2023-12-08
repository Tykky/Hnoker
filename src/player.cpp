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
        next_second = clock.now() + 1s;
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

    void MusicPlayer::start_thread()
    {
        thread = std::jthread([&]()
        {
            std::cout << "\n";
            while (true)
            {
                next_second = clock.now() + 1s;
                const int song_at_start = song_id;
                bool broke = false;

                const auto& [artist, name, duration] = songs.at(song_at_start);

                for (elapsed = 0; elapsed <= duration; ++elapsed)
                {
                    std::cout << std::format("\033[F{} - {:<32}\n{:02}:{:02}", artist, name, elapsed / 60, elapsed % 60);

                    std::this_thread::sleep_until(next_second);
                    next_second += 1s;

                    if (paused)
                    {
                        pause();
                    }

                    if (stopper.stop_requested())
                    {
                        return;
                    }

                    if (song_id != song_at_start)
                    {
                        broke = true;
                        break;
                    }
                }

                if (!broke)
                {
                    next_song();
                }
            }
        });
    }

    MusicPlayer::MusicPlayer(int initial_song) : song_id(initial_song) { start_thread(); };

    void MusicPlayer::next_song()
    {
        wait_if_queue_empty();
        INFO("New song detected, acquiring lock to play it")
        std::lock_guard<std::mutex> queue_lock(queue_mutex);
        song_id = song_queue.front();
        song_queue.pop_front();
        elapsed = -1;
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
            elapsed = -1;
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
        std::array<int, MAXIMUM_QUEUE_SIZE> queue_array{};
        std::uint8_t size;

        {
            auto array_it = queue_array.begin();
            std::lock_guard<std::mutex> queue_lock(queue_mutex);
            size = (std::uint8_t) song_queue.size();
            std::copy(song_queue.cbegin(), song_queue.cend(), queue_array.begin());
        }

        return SendStatus {
            song_id,
            elapsed,
            paused,
            size,
            queue_array,
        };
    }
}
