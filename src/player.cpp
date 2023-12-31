#include "player.hpp"
#include "networking.hpp"


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
        INFO("Ran out of music to play, waiting for new song");
        const unsigned int max_wait = 10;
        unsigned int wait = 0;
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
            wait++;
        }
    }

    static bool send_msg_to_coordinator(const std::string_view leader_ip, const Message& msg)
    {
        hnoker::network net;

        std::array<char, 1024> rb;
        std::array<char, 1024> wb;

        if (leader_ip == "")
        {
            INFO("Message was not sent to leader due unknown leader ip!");
            return false;
        }

        std::function write_op = [&msg](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
        {
            hnoker::write_message_to_buffer(write_buf, msg);
            return false;
        };

        constexpr unsigned int max_tries = 2;
        bool message_success = true;

        hnoker::timeout_handler th = [&, try_count = 0]() mutable
        {
            while (++try_count <= max_tries)
            {
                message_success = false;
                INFO("Timed out while sending a message to the leader!, tries {}/{} remaining!", try_count, max_tries);
                hnoker::write_message_to_buffer(wb, msg);
            }
        };

        net.async_connect_server(leader_ip, LISTENER_SERVER_PORT, rb, wb, write_op, th);
        net.run();

        return message_success;
    }

    static std::string find_coordinator(ClientList& cl)
    {
        Client coordinator;
        coordinator.ip = "";
        coordinator.bully_id = 0;
        for (auto& c : cl.clients)
            if (c.bully_id > coordinator.bully_id)
                coordinator = c;
        return coordinator.ip;
    }
    
    void MusicPlayer::start_player(ClientList& cl)
    {
        std::string song_name = "";

        Message stop_msg = { MessageType::CONTROL_MUSIC };
        Message start_msg = { MessageType::CONTROL_MUSIC };
        Message skip_msg = { MessageType::CONTROL_MUSIC };
        stop_msg.cm.op = ControlOperation::STOP;
        start_msg.cm.op = ControlOperation::START;
        skip_msg.cm.op = ControlOperation::SKIP;


        std::function<void()> stop_callback = [&,this]() 
        {
            send_msg_to_coordinator(find_coordinator(cl), stop_msg);
        };

        std::function<void()> start_callback = [&,this]()
        {
            send_msg_to_coordinator(find_coordinator(cl), start_msg);
        };

        std::function<void()> skip_callback = [&,this]()
        {
            send_msg_to_coordinator(find_coordinator(cl), skip_msg);
        };

        g.start_callback = &start_callback;
        g.stop_callback = &stop_callback;
        g.skip_callback = &skip_callback;

        song_queue.push_back(1);
        song_queue.push_back(2);
        song_queue.push_back(1);
        song_queue.push_back(2);
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

                g.gui_client_list.clear();
                for (Client c : cl.clients)
                {
                    std::uint16_t leader = cl.bully_id;
                    std::uint16_t client_id = c.bully_id;
                    g.gui_client_list.push_back(std::format("{} {}", c.ip, client_id == leader ? "- BULLY" : ""));
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

    void MusicPlayer::set_status(const SendStatus& status)
    {
        song_id = status.current_song_id;
        elapsed = status.elapsed_time;
        paused = status.paused;
        song_queue = status.queue;
    }
}
