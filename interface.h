#pragma once
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "cell/midi_uart.h"
#include "pico-ssd1306/ssd1306.h"

sequencer esq;
uint16_t point[_tracks];
volatile int f = 0;

void ssd1306_line(ssd1306_t* oled, uint8_t x, uint8_t y, uint8_t length, bool vertical)
{
    if(vertical) for(int i = y; i < length + y; i++) ssd1306_PSET(oled, x, i);
    else for(int i = x; i < length + x; i++) ssd1306_PSET(oled, i, y);
}

void ssd1306_progress_bar(ssd1306_t* oled, uint16_t value, uint16_t x, uint16_t y, uint16_t max, uint8_t length, uint8_t width, bool vertical)
{
    if(vertical)
    {
        uint16_t v = roundf((float)(value * length) / (float)max);
        for(int i = (y + length); i > (y + length - v); i-=2 ) ssd1306_line(oled, x, i, width, false);
    }
}

bool note_glyph[24] =
{
    0,0,1,1,1,0,
    0,1,1,1,1,1,
    1,1,1,1,1,0,
    0,1,1,1,0,0
};

bool circle_glyph_8x8[] =
{
    0,0,1,1,1,1,0,0,
    0,1,0,0,0,0,1,0,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    0,1,0,0,0,0,1,0,
    0,0,1,1,1,1,0,0,
};

bool circle_glyph_f_8x8[] =
{
    0,0,1,1,1,1,0,0,
    0,1,0,0,0,0,1,0,
    1,0,0,1,1,0,0,1,
    1,0,1,1,1,1,0,1,
    1,0,1,1,1,1,0,1,
    1,0,0,1,1,0,0,1,
    0,1,0,0,0,0,1,0,
    0,0,1,1,1,1,0,0
};

bool circle_glyph_r_8x8[] =
{
    0,0,1,1,1,1,0,0,
    0,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    0,1,1,1,1,1,1,0,
    0,0,1,1,1,1,0,0
};

void ssd1306_glyph(ssd1306_t* oled, bool* data, uint8_t w, uint8_t h, uint8_t x, uint8_t y)
{
    for(int i = 0; i < h; i++)
    {
        for(int j = 0; j < w; j++)
        {
            if(data[j + w * i]) ssd1306_PSET(oled, x + j, y + i);
        }
    }
}

