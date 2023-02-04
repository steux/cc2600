// dpcplus.h - Display Processor Chip Plus Definitions
// Chris Walton, Fred Quimby, Darrell Spice 2010
// Converted to C by Bruno Steux 2023
// Version 1.00

#ifndef __DPCPLUS__
#define __DPCPLUS__

//****************************************
// DPC+ Read Registers
//****************************************
//
//----------------------------------------
// Random Numbers
//----------------------------------------
// DPC+ provides a 32 bit LFSR (Linear feedback shift register)
// which is used as a random number generator.  Each individual byte of the
// random number will return values from 0-255.  The random numbers will follow
// an exact sequence, so it's best to clock them at least once per frame even if 
// you don't need the value (this allows the amount of time it takes the user to
// start the game to select a random starting point in the sequence)
//----------------------------------------
unsigned char * const RANDOM0NEXT   = 0x1000; // clock next 32 bit number and returns byte 0
unsigned char * const RANDOM0PRIOR  = 0x1001; // clock prior 32 bit number and returns byte 0
unsigned char * const RANDOM1       = 0x1002; // returns byte 1 of random number w/out clock
unsigned char * const RANDOM2       = 0x1003; // returns byte 2 of random number w/out clock
unsigned char * const RANDOM3       = 0x1004; // returns byte 3 of random number w/out clock

//----------------------------------------
// Music Fetcher
//----------------------------------------
// When generating music, this value must be read every single scanline and
// stored into AUDV0.
//----------------------------------------
unsigned char * const AMPLITUDE     = 0x1005;

//----------------------------------------
// Data Fetcher
//----------------------------------------
// There are 8 Data Fetchers which are used to access data stored in the Display
// Data bank.  Before using, you must point the Data Fetcher at the data to read
// via DFxLOW and DFxHI.  After each read the Data Fetcher will update to point
// to the next byte of data to return.
//----------------------------------------
unsigned char * const DF0DATA       = 0x1008;
unsigned char * const DF1DATA       = 0x1009;
unsigned char * const DF2DATA       = 0x100A;
unsigned char * const DF3DATA       = 0x100B;
unsigned char * const DF4DATA       = 0x100C;
unsigned char * const DF5DATA       = 0x100D;
unsigned char * const DF6DATA       = 0x100E;
unsigned char * const DF7DATA       = 0x100F;

//----------------------------------------
// Data Fetcher, Windowed
//----------------------------------------
// The 8 Data Fetchers can also be read in a "windowed" mode, which is most
// commonly used to update sprites.  To use windowed mode, point the Data
// Fetcher the same as above, but then also set the Top and Bottom of the
// Window using DFxTOP and DFxBOT.  When reading via the DFxDATAW registers, a 0
// value will be returned for anything that's outside of the window.
//----------------------------------------
unsigned char * const DF0DATAW      = 0x1010;
unsigned char * const DF1DATAW      = 0x1011;
unsigned char * const DF2DATAW      = 0x1012;
unsigned char * const DF3DATAW      = 0x1013;
unsigned char * const DF4DATAW      = 0x1014;
unsigned char * const DF5DATAW      = 0x1015;
unsigned char * const DF6DATAW      = 0x1016;
unsigned char * const DF7DATAW      = 0x1017;

//----------------------------------------
// Fractional Data Fetcher
//----------------------------------------
// Another 8 Data Fetchers exist which work differently than the first 8.
// These allow you to fractionally increment the Data Fetcher so a single
// value can be read a set number of times before advancing to the next value.
// This is commonly used to draw asymmetrical playfields without needing to 
// use 1200 bytes of data (200 scanlines * 6 playfield updates).
// Before using, you must point the Fractional Data Fetcher at the data to read
// via DFxFRACLOW and DFxFRACHI.  You must also set the increment value via
// DFxFRACINC.
//----------------------------------------
unsigned char * const DF0FRACDATA   = 0x1018;
unsigned char * const DF1FRACDATA   = 0x1019;
unsigned char * const DF2FRACDATA   = 0x101A;
unsigned char * const DF3FRACDATA   = 0x101B;
unsigned char * const DF4FRACDATA   = 0x101C;
unsigned char * const DF5FRACDATA   = 0x101D;
unsigned char * const DF6FRACDATA   = 0x101E;
unsigned char * const DF7FRACDATA   = 0x101F;

//----------------------------------------
// Data Fetcher Window Flag
//----------------------------------------
// The Data Fetcher Window Flag allows you to dual-purpose the first four
// Data Fetchers.  The Window is not required when a Data Fetcher is used to
// update a sprite's color.  The Flag will return $FF if it's within the window,
// or 0 if it's not - this value can be used to control the display of the ball
// and missiles. The Data Fetcher will NOT increment when reading the flag.
//----------------------------------------
unsigned char * const DF0FLAG       = 0x1020;
unsigned char * const DF1FLAG       = 0x1021;
unsigned char * const DF2FLAG       = 0x1022;
unsigned char * const DF3FLAG       = 0x1023;

//****************************************
// SECTION 2 - DPC+ Write Registers
//****************************************
//
//----------------------------------------
// Fractional Data Fetcher, Low Pointer
//----------------------------------------
// These are used in conjunction with DFxFRACHI to point a Fractional Data
// Fetcher to the data to read.  For usage, see "Fractional Data Fetcher"
// in SECTION 1.
//----------------------------------------
unsigned char * const DF0FRACLOW    = 0x1028;
unsigned char * const DF1FRACLOW    = 0x1029;
unsigned char * const DF2FRACLOW    = 0x102A;
unsigned char * const DF3FRACLOW    = 0x102B;
unsigned char * const DF4FRACLOW    = 0x102C;
unsigned char * const DF5FRACLOW    = 0x102D;
unsigned char * const DF6FRACLOW    = 0x102E;
unsigned char * const DF7FRACLOW    = 0x102F;

//----------------------------------------
// Fractional Data Fetcher, High Pointer
//----------------------------------------
// These are used in conjunction with DFxFRACLOW to point a Fractional Data
// Fetcher to the data to read.  For usage, see "Fractional Data Fetcher"
// in SECTION 1.
//
// NOTE: for only the lower 4 bits are used.
//----------------------------------------
unsigned char * const DF0FRACHI     = 0x1030;
unsigned char * const DF1FRACHI     = 0x1031;
unsigned char * const DF2FRACHI     = 0x1032;
unsigned char * const DF3FRACHI     = 0x1033;
unsigned char * const DF4FRACHI     = 0x1034;
unsigned char * const DF5FRACHI     = 0x1035; // 
unsigned char * const DF6FRACHI     = 0x1036; // 
unsigned char * const DF7FRACHI     = 0x1037; // 

//----------------------------------------
// Fractional Data Fetcher, Increment
//----------------------------------------
// These are used to set the increment amount for the Fractional Data Fetcher.
// To increment pointer after every Xth read use int(256/X)
// For usage, see "Fractional Data Fetcher" in SECTION 1.
//----------------------------------------
unsigned char * const DF0FRACINC    = 0x1038;
unsigned char * const DF1FRACINC    = 0x1039;
unsigned char * const DF2FRACINC    = 0x103A;
unsigned char * const DF3FRACINC    = 0x103B;
unsigned char * const DF4FRACINC    = 0x103C;
unsigned char * const DF5FRACINC    = 0x103D;
unsigned char * const DF6FRACINC    = 0x103E;
unsigned char * const DF7FRACINC    = 0x103F;

//----------------------------------------
// Data Fetcher, Window Top
//----------------------------------------
// These are used with unsigned char * const DFxBOT to define the Data Fetcher Window
// For usage, see "Data Fetcher, Windowed" in SECTION 1.
//----------------------------------------
unsigned char * const DF0TOP        = 0x1040;
unsigned char * const DF1TOP        = 0x1041;
unsigned char * const DF2TOP        = 0x1042;
unsigned char * const DF3TOP        = 0x1043;
unsigned char * const DF4TOP        = 0x1044;
unsigned char * const DF5TOP        = 0x1045;
unsigned char * const DF6TOP        = 0x1046;
unsigned char * const DF7TOP        = 0x1047;

//----------------------------------------
// Data Fetcher, Window Bottom
//----------------------------------------
// These are used with unsigned char * const DFxTOP to define the Data Fetcher Window
// For usage, see "Data Fetcher, Windowed" in SECTION 1.
//----------------------------------------
unsigned char * const DF0BOT        = 0x1048;
unsigned char * const DF1BOT        = 0x1049;
unsigned char * const DF2BOT        = 0x104A;
unsigned char * const DF3BOT        = 0x104B;
unsigned char * const DF4BOT        = 0x104C;
unsigned char * const DF5BOT        = 0x104D;
unsigned char * const DF6BOT        = 0x104E;
unsigned char * const DF7BOT        = 0x104F;

//----------------------------------------
// Data Fetcher, Low Pointer
//----------------------------------------
// These are used in conjunction with unsigned char * const DFxHI to point a Data Fetcher to the data
// to read.  For usage, see "Data Fetcher" in SECTION 1.
//----------------------------------------
unsigned char * const DF0LOW        = 0x1050;
unsigned char * const DF1LOW        = 0x1051;
unsigned char * const DF2LOW        = 0x1052;
unsigned char * const DF3LOW        = 0x1053;
unsigned char * const DF4LOW        = 0x1054;
unsigned char * const DF5LOW        = 0x1055;
unsigned char * const DF6LOW        = 0x1056;
unsigned char * const DF7LOW        = 0x1057;

//----------------------------------------
// Fast Fetch Mode
//----------------------------------------
// Fast Fetch Mode enables the fastest way to read DPC+ registers.  Normal
// reads use LDA Absolute addressing (LDA unsigned char * const DF0DATA) which takes 4 cycles to
// process.  Fast Fetch Mode intercepts LDA Immediate addressing (LDA #<unsigned char * const DF0DATA)
// which takes only 2 cycles!  Only immediate values < $28 are intercepted
// 
// set Fast Fetch Mode
//       *FASTFETCH = 0;
//
// then use immediate mode to read the registers, takes just 5 cycles to update
// any TIA register
//
// when done, turn off Fast Fetch Mode using any non-zero value
//       *FASTFETCH = 0xFF;
//
// NOTE: if you forget to turn off FASTFETCH mode, then code like this will not
//       work as you expect
//	*COLUPF = 0; returns a RANDOM NUMBER, not 0.
//----------------------------------------
unsigned char * const FASTFETCH     = 0x1058;

//----------------------------------------
// Function Support
//----------------------------------------
// Currently only Function 255 is defined, and it is used to call user
// written ARM routines (or C code compiled for the ARM processor.)
//
// PARAMETER is not used by function 255, it may be used by future functions.
//----------------------------------------
unsigned char * const PARAMETER     = 0x1059;
unsigned char * const CALLFUNCTION  = 0x105A;

//----------------------------------------
// Waveforms
//----------------------------------------
// Waveforms are 32 byte tables that define a waveform.  Waveforms must be 32
// byte aligned, and can only be stored in the 4K Display Data Bank. You MUST
// define an "OFF" waveform,  comprised of all zeros.  The sum of all waveforms
// being played should be <= 15, so typically you'll use a maximum of 5 for any
// given value.
//
// Valid values are 0-127 and point to the 4K Display Data bank.  The formula
// (* & $1fff)/32 as shown below will calculate the value for you
//----------------------------------------
unsigned char * const WAVEFORM0     = 0x105D;
unsigned char * const WAVEFORM1     = 0x105E;
unsigned char * const WAVEFORM2     = 0x105F;

//----------------------------------------
// Data Fetcher Push (stack)
//----------------------------------------
// The Data Fetchers can also be used to update the contents of the 4K
// Display Data bank.  Point the Data Fetcher to the data to change,
// then Push to it.  The Data Fetcher's pointer will be decremented BEFORE
// the data is written.
//----------------------------------------
unsigned char * const DF0PUSH       = 0x1060;
unsigned char * const DF1PUSH       = 0x1061;
unsigned char * const DF2PUSH       = 0x1062;
unsigned char * const DF3PUSH       = 0x1063;
unsigned char * const DF4PUSH       = 0x1064;
unsigned char * const DF5PUSH       = 0x1065;
unsigned char * const DF6PUSH       = 0x1066;
unsigned char * const DF7PUSH       = 0x1067;

//----------------------------------------
// Data Fetcher, High Pointer
//----------------------------------------
// These are used in conjunction with unsigned char * const DFxLOW to point a Data Fetcher to the data
// to read.  For usage, see "Data Fetcher" in SECTION 1.
//----------------------------------------
unsigned char * const DF0HI         = 0x1068;
unsigned char * const DF1HI         = 0x1069;
unsigned char * const DF2HI         = 0x106A;
unsigned char * const DF3HI         = 0x106B;
unsigned char * const DF4HI         = 0x106C;
unsigned char * const DF5HI         = 0x106D; // 
unsigned char * const DF6HI         = 0x106E; // 
unsigned char * const DF7HI         = 0x106F; // 

//----------------------------------------
// Random Number Initialization
//----------------------------------------
// The random number generate defaults to a value that spells out DPC+.
// Store any value to RRESET to set the random number back to DPC+, or you
// can use RWRITE0-3 to change the 32 bit value to anything you desire.
//
// NOTE: do not set all 4 bytes to 0, as that will disable the generator.
//----------------------------------------
unsigned char * const RRESET        = 0x1070;
unsigned char * const RWRITE0       = 0x1071;
unsigned char * const RWRITE1       = 0x1072;
unsigned char * const RWRITE2       = 0x1073;
unsigned char * const RWRITE3       = 0x1074;

//----------------------------------------
// Notes
//----------------------------------------
// These are used to select a value from the frequency table to play.
// The default table, store in DPC_frequencies.h, only defines frequencies
// for 1-88, which cover the keys of a piano.  You are free to add additional
// frequencies from 88-255.  Piano keys are defined at the end of this file
//----------------------------------------
unsigned char * const NOTE0         = 0x1075;
unsigned char * const NOTE1         = 0x1076;
unsigned char * const NOTE2         = 0x1077;

//----------------------------------------
// Data Fetcher Write (queue)
//----------------------------------------
// The Data Fetchers can also be used to update the contents of the 4K
// Display Data bank.  Point the Data Fetcher to the data to change,
// then Write to it  The Data Fetcher's pointer will be incremented AFTER
// the data is written.
//----------------------------------------
unsigned char * const DF0WRITE      = 0x1078;
unsigned char * const DF1WRITE      = 0x1079;
unsigned char * const DF2WRITE      = 0x107A;
unsigned char * const DF3WRITE      = 0x107B;
unsigned char * const DF4WRITE      = 0x107C;
unsigned char * const DF5WRITE      = 0x107D;
unsigned char * const DF6WRITE      = 0x107E;
unsigned char * const DF7WRITE      = 0x107F;

#endif __DPCPLUS__
