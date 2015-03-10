/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

typedef struct cl_cdstate_s
{
	bool Valid;
	bool Playing;
	bool PlayLooping;
	unsigned char PlayTrack;
}
cl_cdstate_t;

//extern cl_cdstate_t cd;

extern bool cdValid;
extern bool cdPlaying;
extern bool cdPlayLooping;
extern unsigned char cdPlayTrack;

extern cvar_t cdaudioinitialized;

int CDAudio_Init();
void CDAudio_Open();
void CDAudio_Close();
void CDAudio_Play(int track, bool looping);
void CDAudio_Play_byName (const char *trackname, bool looping, bool tryreal, float startposition);
void CDAudio_Stop();
void CDAudio_Pause();
void CDAudio_Resume();
int CDAudio_Startup();
void CDAudio_Shutdown();
void CDAudio_Update();
float CDAudio_GetPosition();
void CDAudio_StartPlaylist(bool resume);

// Prototypes of the system dependent functions
void CDAudio_SysEject ();
void CDAudio_SysCloseDoor ();
int CDAudio_SysGetAudioDiskInfo ();
float CDAudio_SysGetVolume ();
void CDAudio_SysSetVolume (float volume);
int CDAudio_SysPlay (int track);
int CDAudio_SysStop ();
int CDAudio_SysPause ();
int CDAudio_SysResume ();
int CDAudio_SysUpdate ();
void CDAudio_SysInit ();
int CDAudio_SysStartup ();
void CDAudio_SysShutdown ();
