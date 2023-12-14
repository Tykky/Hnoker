#pragma once

#include "player.hpp"
#include "raylib.h"
#include <string>
#include "functional"

namespace hnoker
{
    struct Gui
    {
        void draw_gui();
        void load_theme();

        std::string song_name    = "SONG NAME";
        std::string button_pause = "pause";
        std::string button_next  = "next";
        std::string button_prev  = "prev";

        std::function<void()> pause_callback = [](){};
        std::function<void()> next_callback = [](){};
        std::function<void()> prev_callback = [](){};

        float progress_bar = 0.0f;

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
