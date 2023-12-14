#pragma once

#include "player.hpp"
#include "raylib.h"
#include <string>
#include "functional"

namespace hnoker
{
    using callback = std::function<void()>;
    struct Gui
    {
        Gui() {}

        void draw_gui();
        void load_theme();

        std::string song_name    = "SONG NAME";
        std::string button_pause = "pause";
        std::string button_next  = "next";
        std::string button_prev  = "prev";

        const std::function<void()>* pause_callback = nullptr;
        const std::function<void()>* next_callback = nullptr;
        const std::function<void()>* prev_callback = nullptr;

        int dps = 0;
        float spd = 0.1f;

        float progress_bar = 0.5f;
        bool music_playing = true;

    private:
        std::string backgroundText = "";
        std::string ProgressBar006Text = "";
        std::string Label006Text = "                                                                                   HNOKER Player";
        std::string GroupBox007Text = "CLIENTS";
        std::string GroupBox008Text = "SONG QUEUE";
        std::string Line010Text = "";

        bool button_pause_press = false;
        bool button_next_press = false;
        bool button_prev_press = false;

        std::vector<std::string> gui_client_list = {};
        std::vector<std::string> gui_song_queue = {};

        Rectangle layoutRecs[10] =
        {
            { 0, 0, 768, 480 },    // background
            { 24, 100, 480, 20 },  // song name
            { 192, 192, 48, 24 },  // pause button
            { 264, 192, 48, 24 },  // next button
            { 120, 192, 48, 24 },  // prev button
            { 48, 144, 432, 12 },  // status bar
            { 0, 0, 768, 24 },     // hnoker text
            { 528, 88, 192, 344 }, // client list
            { 24, 336, 456, 136 }, // song queue
            { 24, 232, 456, 16 },  // splitter
        };
    };
}
