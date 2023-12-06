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
        std::unique_lock<std::mutex> queue_lock(queue_mutex);
        song_id = song_queue.front();
        song_queue.pop_front();
    }

    void MusicPlayer::add_to_queue(int song_id)
    {
        {
            std::unique_lock<std::mutex> queue_lock(queue_mutex);
            song_queue.emplace_back(song_id);
        }
        queue_wait.notify_all();
    }

    void MusicPlayer::set_elapsed(int new_elapsed)
    {
        elapsed = new_elapsed;
    }

    void MusicPlayer::toggle_pause()
    {
        paused = !paused;
        pause_wait.notify_all();
    }
}
