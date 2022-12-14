// MIT License

// Copyright (c) 2022 unmanned

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "sequencer.h"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Track //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void track_init(track_t* restrict o)
{
    o->current  = 0;
    o->mode     = 0;
    o->steps    = STEPS;
    o->revolutions = 0;
    o->trigger = 0;
    o->freerun = false;
    o->euclidean = false;
    for(int i = 0; i < 5; ++i)
    {
    	o->permute[i] = 0;
    	o->sift[i] = 0;
    }
    for(int i = 0; i < STEPS; ++i)
    {
        o->data[i].degree   = 0;
        o->data[i].octave   = 3;
        o->data[i].velocity = 0x7F;
        o->data[i].value    = 8;
        o->data[i].offset   = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Loops //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop_forward(track_t* restrict o)
{
    ++o->current;
    if(o->current >= o->steps) 
    {
        o->current = 0;
        ++o->revolutions;
    }
}

void loop_backward(track_t* restrict o)
{
    --o->current;
    if(o->current < 0)
    {
        o->current = o->steps - 1;
        ++o->revolutions;
    }
}

void loop_pingpong(track_t* restrict o)
{
    static bool f;
    if(f)
    {
        ++o->current;
        if(o->current >= o->steps) 
        {
            o->current = o->steps - 2;
            f = false;
            ++o->revolutions;
        }
    }
    else
    {
        --o->current;
        if(o->current < 0) 
        {
            f = true;
            o->current = 1;
            ++o->revolutions;
        }
    }
}

void loop_random(track_t* restrict o)
{
    static int r;
    o->current = rand_in_range(0, o->steps - 1);
    ++r;
    if(r>o->steps)
    {
        r = 0;
        ++o->revolutions;
    }
}

void (*loop_sequence[])(track_t* restrict) = 
{
    loop_forward,
    loop_backward,
    loop_pingpong,
    loop_random
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

note get_note(track_t* restrict o)
{
    return o->data[o->current];
}

void insert_bits(track_t* restrict o, uint16_t bits)
{
    o->trigger = bits;
}

void reset_timestamp(sequencer* restrict o, int track, int bpm)
{
    if(bpm > 999) bpm = 999;
    else if(bpm < 1) bpm = 1;
    o->o[track].bpm  = bpm;
    o->o[track].beat = 60000/o->o[track].bpm;
    o->o[track].step = o->o[track].beat/4; // /4
    o->o[track].atom = o->o[track].step/32; // /32
    if(o->o[track].atom <= 0) o->o[track].atom = 1;
}

void sequencer_init(sequencer* restrict o, int bpm)
{
    o->state = STOP;
    o->brush = 1;
    for(int i = 0; i < TRACKS; ++i)
    {
        track_init(&o->o[i]);
        o->o[i].channel = i;
        automata_init(&o->automata[i]);
        reset_timestamp(o, i, bpm);
    }
}

void sequencer_rand(sequencer* restrict o, int track)
{
    // uint16_t beat = rand_in_range(1, 0xFFFF);
    // insert_bits(&o->o[track], beat);
    o->o[track].scale.data = rand_in_range(1, 0xFFF);
    o->o[track].scale.root = rand_in_range(0,    11);
    set_scale(&o->o[track].scale);
    for(int i = 0; i < STEPS; ++i)
    {
        o->o[track].data[i].value    = rand_in_range(1, 64);
        o->o[track].data[i].offset   = 0;//rand_in_range(0,  0x7F);
        o->o[track].data[i].degree   = rand_in_range(0, 11);
        o->o[track].data[i].octave   = rand_in_range(0, 8);
        o->o[track].data[i].velocity = rand_in_range(0, 0x7F);
        note_from_degree(&o->o[track].scale, &o->o[track].data[i]);
    }
}

void recount_all(sequencer* restrict o, int track)
{
    for(int i = 0; i < STEPS; ++i)
    {
        note_from_degree(&o->o[track].scale, &o->o[track].data[i] );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Permutations ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sag_degree(sequencer* restrict o, int track, uint16_t data)
{
    int p = STEPS - 1;
    for(int i = STEPS - 1; i >= 0; --i)
    {
        if((data>>i)&1)
        {
            swap(&o->o[track].data[i].degree, &o->o[track].data[p].degree);
            --p;
        }
    }
}

void sag_octave(sequencer* restrict o, int track, uint16_t data)
{
    int p = STEPS - 1;
    for(int i = STEPS - 1; i >= 0; --i)
    {
        if((data>>i)&1)
        {
            swap(&o->o[track].data[i].octave, &o->o[track].data[p].octave);
            --p;
        }
    }
}

void sag_velocity(sequencer* restrict o, int track, uint16_t data)
{
    int p = STEPS - 1;
    for(int i = STEPS - 1; i >= 0; --i)
    {
        if((data>>i)&1)
        {
            swap(&o->o[track].data[i].velocity, &o->o[track].data[p].velocity);
            --p;
        }
    }
}

void sag_duration(sequencer* restrict o, int track, uint16_t data)
{
    int p = STEPS - 1;
    for(int i = STEPS - 1; i >= 0; --i)
    {
        if((data>>i)&1)
        {
            swap(&o->o[track].data[i].value, &o->o[track].data[p].value);
            --p;
        }
    }
}

void sag_offset(sequencer* restrict o, int track, uint16_t data)
{
    int p = STEPS - 1;
    for(int i = STEPS - 1; i >= 0; --i)
    {
        if((data>>i)&1)
        {
            swap(&o->o[track].data[i].offset, &o->o[track].data[p].offset);
            --p;
        }
    }
}

void sag_sieve(sequencer* restrict o, int track, uint16_t data)
{
    int p = 7;
    uint16_t f = xor16to8(data);
    for(int i = 7; i >= 0; --i)
    {
        if((f>>i)&1)
        {
            swap(&o->o[track].sieve[i], &o->o[track].sieve[p]);
            --p;
        }
    }
}

void (*mutate[])(sequencer* restrict, int, uint16_t) = 
{
    sag_degree,
    sag_octave,
    sag_velocity,
    sag_duration,
    sag_offset,
    sag_sieve
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sieves /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void regenerate_sieve(sequencer* restrict o, int track, uint16_t data)
{
	int period[STEPS];
	o->o[track].gaps = sieve(period, STEPS, data);
	o->o[track].median = 0;
    for(int i = 0; i < o->o[track].gaps; ++i) o->o[track].median += period[i];
    o->o[track].median /= o->o[track].gaps;
	for(int i = 0; i < 8; ++i)
	{
		o->o[track].sieve[i] = period[i%o->o[track].gaps];
	}
}


void sift_degree(sequencer* restrict o, int track)
{
    int c = 0;
    for(int i = 0; i < STEPS; ++i)
    {
        o->o[track].data[i].degree += o->o[track].sieve[c];
        o->o[track].data[i].degree %= 12;
        if(++c > 7) c = 0;
    }
}

void sift_octave(sequencer* restrict o, int track)
{
    int c = 0;
    for(int i = 0; i < STEPS; ++i)
    {
        o->o[track].data[i].octave += o->o[track].sieve[c];
        o->o[track].data[i].octave %= 10;
        if(++c > 7) c = 0;
    }
}

void sift_velocity(sequencer* restrict o, int track)
{
    int c = 0;
    for(int i = 0; i < STEPS; ++i)
    {
        o->o[track].data[i].velocity += (o->o[track].sieve[c] - o->o[track].median);
		fit_velocity(o, track, i);
        if(++c > 7) c = 0;
    }
}

void sift_duration(sequencer* restrict o, int track)
{
    int c = 0;
    for(int i = 0; i < STEPS; ++i)
    {
        o->o[track].data[i].value += (o->o[track].sieve[c] - o->o[track].median);
     	fit_duration(o, track, i);
        if(++c > 7) c = 0;
    }
}

void sift_offset(sequencer* restrict o, int track)
{
    int c = 0;
    for(int i = 0; i < STEPS; ++i)
    {
        o->o[track].data[i].offset += (o->o[track].sieve[c] - o->o[track].median);
        fit_offset(o, track, i);
        if(++c > 7) c = 0;
    }
}


void (*sift[])(sequencer* restrict, int) =
{
	sift_degree,
	sift_octave,
	sift_velocity,
	sift_duration,
	sift_offset
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Parameter setters //////////////////////////////////////////////////////////////////////////////////////////////////////////
// Velocity ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void velocity_rise (sequencer* restrict o, int track, int center, int range, int value, int incr)
{
	for(int i = 0; i < range; ++i)
	{
		int pos = center - i;
		if(pos < 0) pos += 16;
		o->o[track].data[pos].velocity += (i > value ? 0 : incr);
		fit_velocity(o, track, pos);
	}
}

void velocity_fall (sequencer* restrict o, int track, int center, int range, int value, int incr)
{
	for(int i = 0; i < range; ++i)
	{
		int pos = center + i;
		if(pos > 15) pos -= 16;
		o->o[track].data[pos].velocity += (i > value ? 0 : incr);
		fit_velocity(o, track, pos);
	}
}

void velocity_wave (sequencer* restrict o, int track, int center, int range, int value, int incr)
{
	o->o[track].data[center].velocity +=  incr;
	fit_velocity(o, track, center);
	for(int i = 1; i < range/2; ++i)
	{
		int rise = center - i;
		if(rise < 0) rise += 16;
		int fall = center + i;
		if(fall > 15) fall -= 16;
		int v = (i > value/2 ? 0 : incr);
		o->o[track].data[rise].velocity += v;
		o->o[track].data[fall].velocity += v;
		fit_velocity(o, track, rise);
		fit_velocity(o, track, fall);
	}
}

// Duration ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void duration_rise (sequencer* restrict o, int track, int center, int range, int value, int incr)
{
	for(int i = 0; i < range; ++i)
	{
		int pos = center - i;
		if(pos < 0) pos += 16;
		o->o[track].data[pos].value += (i > value ? 0 : incr);
		fit_duration(o, track, pos);
	}
}

void duration_fall (sequencer* restrict o, int track, int center, int range, int value, int incr)
{
	for(int i = 0; i < range; ++i)
	{
		int pos = center + i;
		if(pos > 15) pos -= 16;
		o->o[track].data[pos].value += (i > value ? 0 : incr);
		fit_duration(o, track, pos);
	}
}

void duration_wave (sequencer* restrict o, int track, int center, int range, int value, int incr)
{
	o->o[track].data[center].value +=  incr;
	fit_duration(o, track, center);
	for(int i = 1; i < range/2; ++i)
	{
		int rise = center - i;
		if(rise < 0) rise += 16;
		int fall = center + i;
		if(fall > 15) fall -= 16;
		int v = (i > value/2 ? 0 : incr);
		o->o[track].data[rise].value += v;
		o->o[track].data[fall].value += v;
		fit_duration(o, track, rise);
		fit_duration(o, track, fall);
	}
}

// Offset /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void offset_rise (sequencer* restrict o, int track, int center, int range, int value, int incr)
{
	for(int i = 0; i < range; ++i)
	{
		int pos = center - i;
		if(pos < 0) pos += 16;
		o->o[track].data[pos].offset += (i > value ? 0 : incr);
		fit_offset(o, track, pos);
	}
}

void offset_fall (sequencer* restrict o, int track, int center, int range, int value, int incr)
{
	for(int i = 0; i < range; ++i)
	{
		int pos = center + i;
		if(pos > 15) pos -= 16;
		o->o[track].data[pos].value += (i > value ? 0 : incr);
		fit_offset(o, track, pos);
	}
}

void offset_wave (sequencer* restrict o, int track, int center, int range, int value, int incr)
{
	o->o[track].data[center].offset +=  incr;
	fit_offset(o, track, center);
	for(int i = 1; i < range/2; ++i)
	{
		int rise = center - i;
		if(rise < 0) rise += 16;
		int fall = center + i;
		if(fall > 15) fall -= 16;
		int v = (i > value/2 ? 0 : incr);
		o->o[track].data[rise].offset += v;
		o->o[track].data[fall].offset += v;
		fit_offset(o, track, rise);
		fit_offset(o, track, fall);
	}
}


void velocity_rect (sequencer* restrict o, int track, int center, int range, int value, int incr)
{
	(void)value;
	for(int i = (center - range/2); i <= (center + range/2); ++i)
	{
		int pos = i;
		if(pos < 0) pos += 16;
		else if(pos > 15) pos -= 16;
		o->o[track].data[pos].velocity += incr;
		fit_velocity(o, track, pos);
	}
}

void (*set_section[])(sequencer* restrict, int, int, int, int, int) = 
{
	velocity_rise,
	velocity_fall,
	velocity_wave,
	duration_rise,
	duration_fall,
	duration_wave,
	offset_rise,
	offset_fall,
	offset_wave

};



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
