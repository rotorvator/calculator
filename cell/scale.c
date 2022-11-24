#include "scale.h"


void set_scale(scale_t* scale)
{
    scale->data  |= (1<<(11 - scale->root));
    uint8_t s = 0;
    for(int i = scale->root; i < 12 + scale->root; i++)
    {
        if(scale->data & (0x800 >> (i % 12)))
        {
            scale->degree[s] = i - scale->root;
            s++;
        }
    }
    scale->width = s;
}

void note_from_degree(scale_t* scale, note* o)
{
    o->chroma = scale->root + scale->degree[o->degree % scale->width] + 12 * o->octave;
}



const char* chromatic[]    = {" C","#C"," D","#D"," E"," F","#F"," G","#G"," A","#A"," B"};
const char* chromatic_lr[] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
