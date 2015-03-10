#pragma once

//#define GAMENAME "id1"
#ifndef	BUILD_CLOTURE_GAME
	constexpr char* GAMENAME = "id1";
#else
	constexpr char* GAMENAME = "cl1";
#endif

#define MAX_NUM_ARGVS	50


#define	MAX_INPUTLINE			16384 ///< maximum length of console commandline, QuakeC strings, and many other text processing buffers
#define	CON_TEXTSIZE			1048576 ///< max scrollback buffer characters in console
#define	CON_MAXLINES			16384 ///< max scrollback buffer lines in console
#define	HIST_TEXTSIZE			262144 ///< max command history buffer characters in console
#define	HIST_MAXLINES			4096 ///< max command history buffer lines in console

#define	CMDBUFSIZE				655360 ///< maximum script size that can be loaded by the exec command (8192 in Quake)

#define	NET_MAXMESSAGE			65536 ///< max reliable packet size (sent as multiple fragments of MAX_PACKETFRAGMENT)
#define	MAX_PACKETFRAGMENT		1024 ///< max length of packet fragment
#define	MAX_EDICTS				32768 ///< max number of objects in game world at once (32768 protocol limit)
#define	MAX_MODELS				8192 ///< max number of models loaded at once (including during level transitions)
#define	MAX_SOUNDS				4096 ///< max number of sounds loaded at once
#define	MAX_LIGHTSTYLES			256 ///< max flickering light styles in level (note: affects savegame format)
#define	MAX_STYLESTRING			64 ///< max length of flicker pattern for light style
#define	MAX_SCOREBOARD			255 ///< max number of players in game at once (255 protocol limit)
#define	MAX_SCOREBOARDNAME		128 ///< max length of player name in game
#define	MAX_USERINFO_STRING		1280 ///< max length of infostring for PROTOCOL_QUAKEWORLD (196 in QuakeWorld)
#define	MAX_SERVERINFO_STRING	1280 ///< max length of server infostring for PROTOCOL_QUAKEWORLD (512 in QuakeWorld)
#define	MAX_LOCALINFO_STRING	32768 ///< max length of server-local infostring for PROTOCOL_QUAKEWORLD (32768 in QuakeWorld)
#define	CL_MAX_USERCMDS			128 ///< max number of predicted input packets in queue
#define	CVAR_HASHSIZE			65536 ///< number of hash buckets for accelerating cvar name lookups
#define	M_MAX_EDICTS			32768 ///< max objects in menu vm
#define	MAX_DEMOS				8 ///< max demos provided to demos command
#define	MAX_DEMONAME			16 ///< max demo name length for demos command
#define	MAX_SAVEGAMES			12 ///< max savegames listed in savegame menu
#define	SAVEGAME_COMMENT_LENGTH	39 ///< max comment length of savegame in menu
#define	MAX_CLIENTNETWORKEYES	16 ///< max number of locations that can be added to pvs when culling network entities (must be at least 2 for prediction)
#define	MAX_LEVELNETWORKEYES	512 ///< max number of locations that can be added to pvs when culling network entities (must be at least 2 for prediction)
#define	MAX_OCCLUSION_QUERIES	4096 ///< max number of GL_ARB_occlusion_query objects that can be used in one frame

#define CRYPTO_HOSTKEY_HASHSIZE 8192 ///< number of hash buckets for accelerating host key lookups
#define MAX_NETWM_ICON 352822 // 16x16, 22x22, 24x24, 32x32, 48x48, 64x64, 128x128, 256x256, 512x512

#define	MAX_WATERPLANES			16 ///< max number of water planes visible (each one causes additional view renders)
#define	MAX_CUBEMAPS			1024 ///< max number of cubemap textures loaded for light filters
#define	MAX_EXPLOSIONS			64 ///< max number of explosion shell effects active at once (not particle related)
#define	MAX_DLIGHTS				256 ///< max number of dynamic lights (rocket flashes, etc) in scene at once
#define	MAX_CACHED_PICS			1024 ///< max number of 2D pics loaded at once
#define	CACHEPICHASHSIZE		256 ///< number of hash buckets for accelerating 2D pic name lookups
#define	MAX_PARTICLEEFFECTNAME	4096 ///< maximum number of unique names of particle effects (for particleeffectnum)
#define	MAX_PARTICLEEFFECTINFO	8192 ///< maximum number of unique particle effects (each name may associate with several of these)
#define	MAX_PARTICLETEXTURES	256 ///< maximum number of unique particle textures in the particle font
#define	MAXCLVIDEOS				65 ///< maximum number of video streams being played back at once (1 is reserved for the playvideo command)
#define	MAX_DYNAMIC_TEXTURE_COUNT	64 ///< maximum number of dynamic textures (web browsers, playvideo, etc)
#define	MAX_MAP_LEAFS			65536 ///< maximum number of BSP leafs in world (8192 in Quake)


// 0 to NUM_AMBIENTS - 1 = water, etc
// NUM_AMBIENTS to NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS - 1 = normal entity sounds
// NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS to total_channels = static sounds
#define	MAX_DYNAMIC_CHANNELS	512
#define	MAX_CHANNELS			1028
#define	MODLIST_TOTALSIZE		256
#define	MAX_FAVORITESERVERS		256
#define	MAX_DECALSYSTEM_QUEUE	1024
#define	PAINTBUFFER_SIZE		2048
#define	MAX_BINDMAPS			8
#define	MAX_PARTICLES_INITIAL	8192 ///< initial allocation for cl.particles
#define	MAX_PARTICLES			1048576 ///< upper limit on cl.particles size
#define	MAX_DECALS_INITIAL		8192 ///< initial allocation for cl.decals
#define	MAX_DECALS				1048576 ///< upper limit on cl.decals size
#define	MAX_ENITIES_INITIAL		256 ///< initial size of cl.entities
#define	MAX_STATICENTITIES		1024 ///< limit on size of cl.static_entities
#define	MAX_EFFECTS				256 ///< limit on size of cl.effects
#define	MAX_BEAMS				256 ///< limit on size of cl.beams
#define	MAX_TEMPENTITIES		4096 ///< max number of temporary models visible per frame (certain sprite effects, certain types of CSQC entities also use this)
#define SERVERLIST_TOTALSIZE		2048 ///< max servers in the server list
#define SERVERLIST_ANDMASKCOUNT		16 ///< max items in server list AND mask
#define SERVERLIST_ORMASKCOUNT		16 ///< max items in server list OR mask


#define	MAX_QPATH		128			///< max length of a quake game pathname
#ifdef PATH_MAX
#define	MAX_OSPATH		PATH_MAX
#elif MAX_PATH
#define	MAX_OSPATH		MAX_PATH
#else
#define	MAX_OSPATH		1024		///< max length of a filesystem pathname
#endif

#define	ON_EPSILON		0.1			///< point on plane side epsilon

#define	NET_MINRATE		1000 ///< limits "rate" and "sv_maxrate" cvars

//
// stats are integers communicated to the client by the server
//
#define	MAX_CL_STATS		256
#define	STAT_HEALTH			0

#define	STAT_WEAPON			2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		///< bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		///< bumped by svc_killedmonster
#define STAT_ITEMS			15 ///< FTE, DP
#define STAT_VIEWHEIGHT		16 ///< FTE, DP

#define STAT_VIEWZOOM		21 ///< DP
#define STAT_MOVEVARS_AIRACCEL_QW_STRETCHFACTOR 220 ///< DP
#define STAT_MOVEVARS_AIRCONTROL_PENALTY					221 ///< DP
#define STAT_MOVEVARS_AIRSPEEDLIMIT_NONQW 222 ///< DP
#define STAT_MOVEVARS_AIRSTRAFEACCEL_QW 223 ///< DP
#define STAT_MOVEVARS_AIRCONTROL_POWER					224 ///< DP
#define STAT_MOVEFLAGS                              225 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_AIRFORWARDACCEL	226 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_ACCEL				227 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_TOPSPEED			228 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_TURNACCEL			229 ///< DP
#define STAT_MOVEVARS_WARSOWBUNNY_BACKTOSIDERATIO	230 ///< DP
#define STAT_MOVEVARS_AIRSTOPACCELERATE				231 ///< DP
#define STAT_MOVEVARS_AIRSTRAFEACCELERATE			232 ///< DP
#define STAT_MOVEVARS_MAXAIRSTRAFESPEED				233 ///< DP
#define STAT_MOVEVARS_AIRCONTROL					234 ///< DP
#define STAT_FRAGLIMIT								235 ///< DP
#define STAT_TIMELIMIT								236 ///< DP
#define STAT_MOVEVARS_WALLFRICTION					237 ///< DP
#define STAT_MOVEVARS_FRICTION						238 ///< DP
#define STAT_MOVEVARS_WATERFRICTION					239 ///< DP
#define STAT_MOVEVARS_TICRATE						240 ///< DP
#define STAT_MOVEVARS_TIMESCALE						241 ///< DP
#define STAT_MOVEVARS_GRAVITY						242 ///< DP
#define STAT_MOVEVARS_STOPSPEED						243 ///< DP
#define STAT_MOVEVARS_MAXSPEED						244 ///< DP
#define STAT_MOVEVARS_SPECTATORMAXSPEED				245 ///< DP
#define STAT_MOVEVARS_ACCELERATE					246 ///< DP
#define STAT_MOVEVARS_AIRACCELERATE					247 ///< DP
#define STAT_MOVEVARS_WATERACCELERATE				248 ///< DP
#define STAT_MOVEVARS_ENTGRAVITY					249 ///< DP
#define STAT_MOVEVARS_JUMPVELOCITY					250 ///< DP
#define STAT_MOVEVARS_EDGEFRICTION					251 ///< DP
#define STAT_MOVEVARS_MAXAIRSPEED					252 ///< DP
#define STAT_MOVEVARS_STEPHEIGHT					253 ///< DP
#define STAT_MOVEVARS_AIRACCEL_QW					254 ///< DP
#define STAT_MOVEVARS_AIRACCEL_SIDEWAYS_FRICTION	255 ///< DP

// moveflags values
#define MOVEFLAG_VALID 0x80000000
#define MOVEFLAG_Q2AIRACCELERATE 0x00000001
#define MOVEFLAG_NOGRAVITYONGROUND 0x00000002
#define MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE 0x00000004

// stock defines

#define	IT_SHOTGUN				1
#define	IT_SUPER_SHOTGUN		2
#define	IT_NAILGUN				4
#define	IT_SUPER_NAILGUN		8
#define	IT_GRENADE_LAUNCHER		16
#define	IT_ROCKET_LAUNCHER		32
#define	IT_LIGHTNING			64
#define IT_SUPER_LIGHTNING      128
#define IT_SHELLS               256
#define IT_NAILS                512
#define IT_ROCKETS              1024
#define IT_CELLS                2048
#define IT_AXE                  4096
#define IT_ARMOR1               8192
#define IT_ARMOR2               16384
#define IT_ARMOR3               32768
#define IT_SUPERHEALTH          65536
#define IT_KEY1                 131072
#define IT_KEY2                 262144
#define	IT_INVISIBILITY			524288
#define	IT_INVULNERABILITY		1048576
#define	IT_SUIT					2097152
#define	IT_QUAD					4194304
#define IT_SIGIL1               (1<<28)
#define IT_SIGIL2               (1<<29)
#define IT_SIGIL3               (1<<30)
#define IT_SIGIL4               (1<<31)

//===========================================
// AK nexuiz changed and added defines

#define NEX_IT_UZI              1
#define NEX_IT_SHOTGUN          2
#define NEX_IT_GRENADE_LAUNCHER 4
#define NEX_IT_ELECTRO          8
#define NEX_IT_CRYLINK          16
#define NEX_IT_NEX              32
#define NEX_IT_HAGAR            64
#define NEX_IT_ROCKET_LAUNCHER  128
#define NEX_IT_SHELLS           256
#define NEX_IT_BULLETS          512
#define NEX_IT_ROCKETS          1024
#define NEX_IT_CELLS            2048
#define NEX_IT_LASER            4094
#define NEX_IT_STRENGTH         8192
#define NEX_IT_INVINCIBLE       16384
#define NEX_IT_SPEED            32768
#define NEX_IT_SLOWMO           65536

//===========================================
//rogue changed and added defines

#define RIT_SHELLS              128
#define RIT_NAILS               256
#define RIT_ROCKETS             512
#define RIT_CELLS               1024
#define RIT_AXE                 2048
#define RIT_LAVA_NAILGUN        4096
#define RIT_LAVA_SUPER_NAILGUN  8192
#define RIT_MULTI_GRENADE       16384
#define RIT_MULTI_ROCKET        32768
#define RIT_PLASMA_GUN          65536
#define RIT_ARMOR1              8388608
#define RIT_ARMOR2              16777216
#define RIT_ARMOR3              33554432
#define RIT_LAVA_NAILS          67108864
#define RIT_PLASMA_AMMO         134217728
#define RIT_MULTI_ROCKETS       268435456
#define RIT_SHIELD              536870912
#define RIT_ANTIGRAV            1073741824
#define RIT_SUPERHEALTH         2147483648

//MED 01/04/97 added hipnotic defines
//===========================================
//hipnotic added defines
#define HIT_PROXIMITY_GUN_BIT 16
#define HIT_MJOLNIR_BIT       7
#define HIT_LASER_CANNON_BIT  23
#define HIT_PROXIMITY_GUN   (1<<HIT_PROXIMITY_GUN_BIT)
#define HIT_MJOLNIR         (1<<HIT_MJOLNIR_BIT)
#define HIT_LASER_CANNON    (1<<HIT_LASER_CANNON_BIT)
#define HIT_WETSUIT         (1<<(23+2))
#define HIT_EMPATHY_SHIELDS (1<<(23+3))

//===========================================
