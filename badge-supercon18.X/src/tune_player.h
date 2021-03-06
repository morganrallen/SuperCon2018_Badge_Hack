#ifndef		__TUNE_PLAYER_H
#define		__TUNE_PLAYER_H

void play_mario_tune(void);
void play_music_array(const unsigned char *, unsigned char, const unsigned int *);

const unsigned char mario_array_limits[9] = { 48, 52, 52, 56, 52, 56, 64, 64, 64 };
const unsigned int mario_tempos[4] = {149, 198, 298, 447 };

const unsigned char mario_main0[48] = 
	{
		50,66,76,0,
		50,66,76,0,
		0,0,0,0,
		50,66,76,0,
		0,0,0,0,
		50,66,72,0,
		50,66,76,0,
		0,0,0,0,
		67,71,79,0,
		0,0,0,3,
		0,55,67,0,
		0,0,0,3
	};

const unsigned char mario_main1[52] = 
	{
		48,0,0,0,
		0,0,0,0,
		56,68,75,0,
		0,0,0,2,
		58,65,74,0,
		0,0,0,2,
		60,64,72,0,
		0,0,0,2,
		55,0,0,0,
		55,0,0,0,
		0,0,0,0,
		48,0,0,0,
		0,0,0,0
	};

const unsigned char mario_main2[52] = 
	{
		48,0,0,0,
		0,0,0,0,
		56,68,75,0,
		0,0,0,2,
		56,65,74,0,
		0,0,0,2,
		60,64,72,0,
		0,0,0,2,
		55,0,0,0,
		55,0,0,0,
		0,0,0,0,
		48,0,0,0,
		0,0,0,0
	};



const unsigned char mario_main3[56] = 
	{
		44,68,72,0,
		0,68,72,0,
		0,0,0,0,
		51,68,72,0,
		0,0,0,0,
		0,68,72,0,
		56,70,74,0,
		0,67,76,0,
		55,0,0,0,
		0,0,0,2,
		48,0,0,0,
		0,0,0,2,
		43,0,0,0,
		0,0,0,0
	};

const unsigned char mario_clip0[52] =
	{
		55,64,72,0,
		0,0,0,2,
		52,60,67,0,
		0,0,0,2,
		48,55,64,0,
		0,0,0,2,
		53,60,69,0,
		0,0,0,0,
		55,62,71,0,
		0,0,0,0,
		54,61,70,0,
		53,60,69,0,
		0,0,0,0
	};

const unsigned char mario_clip1[56] =
	{
		52,60,67,1,
		60,67,76,1,
		64,71,79,1,
		65,72,81,0,
		0,0,0,0,
		62,69,77,0,
		64,71,79,0,
		0,0,0,0,
		60,69,76,0,
		0,0,0,0,
		57,64,72,0,
		59,65,74,0,
		55,62,71,0,
		0,0,0,2
	};

const unsigned char mario_clip2[64] =
	{
		48,0,0,0,
		0,0,0,0,
		0,76,79,0,
		55,75,78,0,
		0,74,77,0,
		0,71,75,0,
		60,0,0,0,
		0,72,76,0,
		53,0,0,0,
		0,64,68,0,
		0,65,69,0,
		60,67,72,0,
		60,0,0,0,
		0,60,69,0,
		53,64,72,0,
		0,65,74,0
	};

const unsigned char mario_clip3[64] =
	{
		48,0,0,0,
		0,0,0,0,
		0,76,79,0,
		52,75,78,0,
		0,74,77,0,
		0,71,75,0,
		55,0,0,0,
		60,72,76,0,
		0,0,0,0,
		77,79,84,0,
		0,0,0,0,
		77,79,84,0,
		77,79,84,0,
		0,0,0,0,
		55,0,0,0,
		0,0,0,0
	};

const unsigned char mario_clip4[64] =
	{
		44,68,72,0,
		0,68,72,0,
		0,0,0,0,
		51,68,72,0,
		0,0,0,0,
		0,68,72,0,
		56,70,74,0,
		0,0,0,0,
		55,67,76,0,
		0,64,72,0,
		0,0,0,0,
		48,64,69,0,
		0,60,67,0,
		0,0,0,0,
		43,0,0,0,
		0,0,0,0
	};

#endif

