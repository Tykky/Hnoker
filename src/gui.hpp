#pragma once

#include "raylib.h"
#include <string>
#include "functional"

namespace hnoker
{
    using callback = std::function<void()>;
    struct Gui
    {
        Gui()
        {
            InitWindow(768, 480, "hnoker");
            SetTargetFPS(420);
            load_theme();
        }

        void draw_gui();

        std::string song_name    = "SONG NAME";

        const std::function<void()>* start_callback = nullptr;
        const std::function<void()>* stop_callback  = nullptr;
        const std::function<void()>* skip_callback  = nullptr;

        float progress_bar = 0.5f;
        bool music_playing = true;

        std::vector<std::string> gui_client_list = {};
        std::vector<std::string> gui_song_queue = {};

    private:
        void load_theme();

        std::string button_start = "start";
        std::string button_stop  = "stop";
        std::string button_skip  = "skip";

        std::string backgroundText = "";
        std::string ProgressBar006Text = "";
        std::string Label006Text = "                                                                                   HNOKER Player";
        std::string GroupBox007Text = "CLIENTS";
        std::string GroupBox008Text = "SONG QUEUE";
        std::string Line010Text = "";

        bool button_start_press = false;
        bool button_stop_press = false;
        bool button_skip_press = false;


        Rectangle layoutRecs[10] =
        {
            { 0, 0, 768, 480 },    // background
            { 24, 100, 480, 20 },  // song name
            { 192, 192, 48, 24 },  // start button
            { 264, 192, 48, 24 },  // stop button
            { 120, 192, 48, 24 },  // skip button
            { 48, 144, 432, 12 },  // status bar
            { 0, 0, 768, 24 },     // hnoker text
            { 528, 88, 192, 344 }, // client list
            { 24, 336, 456, 136 }, // song queue
            { 24, 232, 456, 16 },  // splitter
        };
    };
}
