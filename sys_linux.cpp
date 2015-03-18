
#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#include <io.h>
#include "conio.h"
#else
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#endif

#include <signal.h>

#include "quakedef.h"

#include "util/common.hpp"
#include "util/bitops.hpp"

using namespace cloture;
using namespace util;
using namespace common;
using namespace system;


// =======================================================================
// General routines
// =======================================================================
void Sys_Shutdown ()
{
#ifdef FNDELAY
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif
	fflush(stdout);
}

void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char string[MAX_INPUTLINE];

// change stdin to non blocking
#ifdef FNDELAY
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif

	va_start (argptr,error);
	dpvsnprintf (string, sizeof (string), error, argptr);
	va_end (argptr);

	Con_Printf ("Quake Error: %s\n", string);

	Host_Shutdown ();
	exit (1);
}

static int outfd = 1;
void Sys_PrintToTerminal(const char *text)
{
	if(outfd < 0)
		return;
#ifdef FNDELAY
	// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
	// this is because both go to /dev/tty by default!
	{
		int origflags = fcntl (outfd, F_GETFL, 0);
		fcntl (outfd, F_SETFL, origflags & ~FNDELAY);
#endif
#ifdef WIN32
#define write _write
#endif
		while(*text)
		{
			fs_offset_t written = (fs_offset_t)write(outfd, text, (int)strlen(text));
			if(written <= 0)
				break; // sorry, I cannot do anything about this error - without an output
			text += written;
		}
#ifdef FNDELAY
		fcntl (outfd, F_SETFL, origflags);
	}
#endif
	//fprintf(stdout, "%s", text);
}

char *Sys_ConsoleInput()
{
	//if (cls.state == ca_dedicated)
	{
		static char text[MAX_INPUTLINE];
		static unsigned int len = 0;
#ifdef WIN32
		int c;

		// read a line out
		while (_kbhit ())
		{
			c = _getch ();
			if (c == '\r')
			{
				text[len] = '\0';
				_putch ('\n');
				len = 0;
				return text;
			}
			if (c == '\b')
			{
				if (len)
				{
					_putch (c);
					_putch (' ');
					_putch (c);
					len--;
				}
				continue;
			}
			if (len < sizeof (text) - 1)
			{
				_putch (c);
				text[len] = c;
				len++;
			}
		}
#else
		fd_set fdset;
		struct timeval timeout;
		FD_ZERO(&fdset);
		FD_SET(0, &fdset); // stdin
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if (select (1, &fdset, nullptr, nullptr, &timeout) != -1 && FD_ISSET(0, &fdset))
		{
			len = read (0, text, sizeof(text) - 1);
			if (len >= 1)
			{
				// rip off the \n and terminate
				// div0: WHY? console code can deal with \n just fine
				// this caused problems with pasting stuff into a terminal window
				// so, not ripping off the \n, but STILL keeping a NUL terminator
				text[len] = 0;
				return text;
			}
		}
#endif
	}
	return nullptr;
}

char *Sys_GetClipboardData ()
{
	return nullptr;
}

void Sys_InitConsole ()
{
}


int main (int argc, char **argv)
{
	signal(SIGFPE, SIG_IGN);

	com_argc = argc;
	com_argv = (const char **)argv;
	Sys_ProvideSelfFD();

	// COMMANDLINEOPTION: sdl: -noterminal disables console output on stdout
	if(COM_CheckParm("-noterminal"))
		outfd = -1;
	// COMMANDLINEOPTION: sdl: -stderr moves console output to stderr
	else if(COM_CheckParm("-stderr"))
		outfd = 2;
	else
		outfd = 1;

#ifdef FNDELAY
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif

	Host_Main();

	return 0;
}

bool sys_supportsdlgetticks = false;

unsigned int Sys_SDL_GetTicks ()
{
	Sys_Error("Called Sys_SDL_GetTicks on non-SDL target");
	return 0;
}

void Sys_SDL_Delay (unsigned int milliseconds)
{
	Sys_Error("Called Sys_SDL_Delay on non-SDL target");
}

#include <sys/mman.h>
bool system::memprotect(void* mem, MemTraits traits, size32 sz)
{
	//assert(mem != nullptr);
	//assert(sz != 0);
	if(unlikely(mem == nullptr))
	{
		system::error(
			"system::memprotect: mem was NULL. sz = 0x%X, traits = 0x%X.\n",
			static_cast<uint32>(sz),
			static_cast<uint32>(traits)
		);
		return false;
	}

	if(unlikely(sz == 0))
	{
		system::error(
				"system::memprotect: sz was 0. traits = 0x%X.\n",
				static_cast<uint32>(traits)
				);
		return false;
	}
	int prot = 0;
	switch(traits)
	{
	case MemTraits::readWriteExecute:
		prot = PROT_READ | PROT_EXEC | PROT_WRITE;
		break;
	case MemTraits::readWrite:
		prot = PROT_READ | PROT_WRITE;
		break;
	case MemTraits::readExecute:
		prot = PROT_READ | PROT_EXEC;
		break;
	case MemTraits::writeExecute:
		prot = PROT_WRITE | PROT_EXEC;
		break;
	case MemTraits::read:
		prot = PROT_READ;
		break;
	case MemTraits::write:
		prot = PROT_WRITE;
		break;
	case MemTraits::execute:
		prot = PROT_EXEC;
		break;
	}
	if(unlikely(mprotect(mem, sz, prot) == -1))
	{
		system::error(
			"system::memprotect: mprotect returned -1! sz = 0x%X, traits = 0x%X.\n",
			static_cast<uint32>(sz),
			static_cast<uint32>(traits)
		);
		return false;
	}
	return true;
}
