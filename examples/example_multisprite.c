const char sprite1[1] = {0};

#define MS_NB_SPRITES_DEF 1
const char *ms_grptr[MS_NB_SPRITES_DEF] = {sprite1};
const char *ms_coluptr[MS_NB_SPRITES_DEF] = {sprite1};
const char ms_height[MS_NB_SPRITES_DEF] = {8};

#include "multisprite.h"

void main()
{
    multisprite_init();
    multisprite_kernel_prep();
    multisprite_kernel();
}

