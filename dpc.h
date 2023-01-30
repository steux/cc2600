// This file defines register for the DPC (David Patrick Crane) Atari 2600 coprocessor
// It is distributed as a companion to cc2600.

//------------------------------------------------------------------------------
//

#ifndef __DPC__
#define __DPC__

unsigned char * const RANDOM      = 0x1000; // Random number generator
unsigned char * const SOUND_VALUE = 0x1004; // Sound value
                                            //
unsigned char * const DF0DATA     = 0x1008; // Data fetcher 0
unsigned char * const DF1DATA     = 0x1009; // Data fetcher 1
unsigned char * const DF2DATA     = 0x100A; // Data fetcher 2
unsigned char * const DF3DATA     = 0x100B; // Data fetcher 3
unsigned char * const DF4DATA     = 0x100C; // Data fetcher 4
unsigned char * const DF5DATA     = 0x100D; // Data fetcher 5
unsigned char * const DF6DATA     = 0x100E; // Data fetcher 6
unsigned char * const DF7DATA     = 0x100F; // Data fetcher 7
                                            //
unsigned char * const DF0DATAW    = 0x1010; // Data fetcher 0 (windowed)
unsigned char * const DF1DATAW    = 0x1011; // Data fetcher 1 (windowed)
unsigned char * const DF2DATAW    = 0x1012; // Data fetcher 2 (windowed)
unsigned char * const DF3DATAW    = 0x1013; // Data fetcher 3 (windowed)
unsigned char * const DF4DATAW    = 0x1014; // Data fetcher 4 (windowed)
unsigned char * const DF5DATAW    = 0x1015; // Data fetcher 5 (windowed)
unsigned char * const DF6DATAW    = 0x1016; // Data fetcher 6 (windowed)
unsigned char * const DF7DATAW    = 0x1016; // Data fetcher 7 (windowed)
                                            //
unsigned char * const DF0FLAG     = 0x1038; // Fetcher 0 mask
unsigned char * const DF1FLAG     = 0x1039; // Fetcher 1 mask
unsigned char * const DF2FLAG     = 0x103A; // Fetcher 2 mask
unsigned char * const DF3FLAG     = 0x103B; // Fetcher 3 mask
unsigned char * const DF4FLAG     = 0x103C; // Fetcher 4 mask
unsigned char * const DF5FLAG     = 0x103D; // Fetcher 5 mask
unsigned char * const DF6FLAG     = 0x103E; // Fetcher 6 mask
unsigned char * const DF7FLAG     = 0x103F; // Fetcher 7 mask
                                            //
unsigned char * const DF0TOP      = 0x1040; // Fetcher 0 start count
unsigned char * const DF1TOP      = 0x1041; // Fetcher 1 start count
unsigned char * const DF2TOP      = 0x1042; // Fetcher 2 start count
unsigned char * const DF3TOP      = 0x1043; // Fetcher 3 start count
unsigned char * const DF4TOP      = 0x1044; // Fetcher 4 start count
unsigned char * const DF5TOP      = 0x1045; // Fetcher 5 start count
unsigned char * const DF6TOP      = 0x1046; // Fetcher 6 start count
unsigned char * const DF7TOP      = 0x1047; // Fetcher 7 start count
                                            //
unsigned char * const DF0BOT      = 0x1048; // Fetcher 0 end count
unsigned char * const DF1BOT      = 0x1049; // Fetcher 1 end count
unsigned char * const DF2BOT      = 0x104A; // Fetcher 2 end count
unsigned char * const DF3BOT      = 0x104B; // Fetcher 3 end count
unsigned char * const DF4BOT      = 0x104C; // Fetcher 4 end count
unsigned char * const DF5BOT      = 0x104D; // Fetcher 5 end count
unsigned char * const DF6BOT      = 0x104E; // Fetcher 6 end count
unsigned char * const DF7BOT      = 0x104F; // Fetcher 7 end count
                                            //
unsigned char * const DF0LOW      = 0x1050; // Fetcher 0 pointer low
unsigned char * const DF1LOW      = 0x1051; // Fetcher 1 pointer low
unsigned char * const DF2LOW      = 0x1052; // Fetcher 2 pointer low
unsigned char * const DF3LOW      = 0x1053; // Fetcher 3 pointer low
unsigned char * const DF4LOW      = 0x1054; // Fetcher 4 pointer low
unsigned char * const DF5LOW      = 0x1055; // Fetcher 5 pointer low
unsigned char * const DF6LOW      = 0x1056; // Fetcher 6 pointer low
unsigned char * const DF7LOW      = 0x1057; // Fetcher 7 pointer low
                                            //
unsigned char * const DF0HI       = 0x1058; // Fetcher 0 pointer high
unsigned char * const DF1HI       = 0x1059; // Fetcher 1 pointer high
unsigned char * const DF2HI       = 0x105A; // Fetcher 2 pointer high
unsigned char * const DF3HI       = 0x105B; // Fetcher 3 pointer high
unsigned char * const DF4HI       = 0x105C; // Fetcher 4 pointer high
unsigned char * const DF5HI       = 0x105D; // Fetcher 5 pointer high
unsigned char * const DF6HI       = 0x105E; // Fetcher 6 pointer high
unsigned char * const DF7HI       = 0x105F; // Fetcher 7 pointer high
                                            //
unsigned char * const RAND_RESET  = 0x1070; // Random number generator reset
                                            //
#endif __DPC__
