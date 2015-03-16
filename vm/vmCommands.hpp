#pragma once


namespace cloture		{
namespace engine		{
namespace vm			{

	void Cmd_Init();

	prvm_stringbuffer_t *BufStr_FindCreateReplace (Program prog, int bufindex, int flags, char *format);
	void BufStr_Set(Program prog, prvm_stringbuffer_t *stringbuffer, int strindex, const char *str);
	void BufStr_Del(Program prog, prvm_stringbuffer_t *stringbuffer);
	void BufStr_Flush(Program prog);
	void error (Program prog);
	void objerror (Program prog);
	void print (Program prog);
	void bprint (Program prog);
	void sprint (Program prog);
	void centerprint (Program prog);
	void normalize (Program prog);
	void vlen (Program prog);
	void vectoyaw (Program prog);
	void vectoangles (Program prog);
	float random (Program prog);
	void localsound(Program prog);
	void _break (Program prog);
	void localcmd (Program prog);
	void cvar (Program prog);
	void cvar_string(Program prog);
	void cvar_type (Program prog);
	void cvar_defstring (Program prog);
	void cvar_set (Program prog);

	void spawn (Program prog);
	void remove (Program prog);
	void find (Program prog);
	void findfloat (Program prog);
	void findchain (Program prog);
	void findchainfloat (Program prog);
	void findflags (Program prog);
	void findchainflags (Program prog);
	void precache_file (Program prog);
	void precache_sound (Program prog);
	void coredump (Program prog);

	void stackdump (Program prog);
	void crash(Program prog); // REMOVE IT
	void traceon (Program prog);
	void traceoff (Program prog);
	void eprint (Program prog);
	void nextent (Program prog);

	void changelevel (Program prog);
	void randomvec (Program prog);
	void registercvar (Program prog);

	void Files_Init(Program prog);
	void Files_CloseAll(Program prog);

	void substring(Program prog);
	void stov(Program prog);
	void strzone(Program prog);
	void strunzone(Program prog);

	// KrimZon - DP_QC_ENTITYDATA
	void numentityfields(Program prog);
	void entityfieldname(Program prog);
	void entityfieldtype(Program prog);
	void getentityfieldstring(Program prog);
	void putentityfieldstring(Program prog);

	// DRESK - String Length (not counting color codes)
	void strlennocol(Program prog);
	// DRESK - Decolorized String
	void strdecolorize(Program prog);
	// DRESK - String Uppercase and Lowercase Support
	void strtolower(Program prog);
	void strtoupper(Program prog);

	void clcommand (Program prog);

	void tokenize (Program prog);
	void tokenizebyseparator (Program prog);
	void argv (Program prog);

	void isserver(Program prog);
	void clientcount(Program prog);
	void clientstate(Program prog);
	// not used at the moment -> not included in the common list
	void getostype(Program prog);
	void getmousepos(Program prog);
	void gettime(Program prog);
	void getsoundtime(Program prog);
	void soundlength(Program prog);
	void loadfromdata(Program prog);
	void parseentitydata(Program prog);
	void loadfromfile(Program prog);
	void modulo(Program prog);

	void search_begin(Program prog);
	void search_end(Program prog);
	void search_getsize(Program prog);
	void search_getfilename(Program prog);
	void chr(Program prog);
	void iscachedpic(Program prog);
	void precache_pic(Program prog);
	void freepic(Program prog);
	void drawcharacter(Program prog);
	void drawstring(Program prog);
	void drawcolorcodedstring(Program prog);
	void stringwidth(Program prog);
	void drawpic(Program prog);
	void drawrotpic(Program prog);
	void drawsubpic(Program prog);
	void drawfill(Program prog);
	void drawsetcliparea(Program prog);
	void drawresetcliparea(Program prog);
	void getimagesize(Program prog);

	void findfont(Program prog);
	void loadfont(Program prog);

	void makevectors (Program prog);
	void vectorvectors (Program prog);

	void keynumtostring (Program prog);
	void getkeybind (Program prog);
	void findkeysforcommand (Program prog);
	void stringtokeynum (Program prog);
	void setkeybind (Program prog);
	void getbindmaps (Program prog);
	void setbindmaps (Program prog);

	void cin_open(Program prog);
	void cin_close(Program prog);
	void cin_setstate(Program prog);
	void cin_getstate(Program prog);
	void cin_restart(Program prog);

	void gecko_create(Program prog);
	void gecko_destroy(Program prog);
	void gecko_navigate(Program prog);
	void gecko_keyevent(Program prog);
	void gecko_movemouse(Program prog);
	void gecko_resize(Program prog);
	void gecko_get_texture_extent(Program prog);

	void drawline (Program prog);

	void bitshift (Program prog);

	void altstr_count(Program prog);
	void altstr_prepare(Program prog);
	void altstr_get(Program prog);
	void altstr_set(Program prog);
	void altstr_ins(Program prog);

	void buf_create(Program prog);
	void buf_del (Program prog);
	void buf_getsize (Program prog);
	void buf_copy (Program prog);
	void buf_sort (Program prog);
	void buf_implode (Program prog);
	void bufstr_get (Program prog);
	void bufstr_set (Program prog);
	void bufstr_add (Program prog);
	void bufstr_free (Program prog);

	void buf_loadfile(Program prog);
	void buf_writefile(Program prog);
	void bufstr_find(Program prog);
	void matchpattern(Program prog);

	void changeyaw (Program prog);
	void changepitch (Program prog);

	void uncolorstring (Program prog);

	void strstrofs (Program prog);
	void str2chr (Program prog);
	void chr2str (Program prog);
	void strconv (Program prog);
	void strpad (Program prog);
	void infoadd (Program prog);
	void infoget (Program prog);
	void strncasecmp (Program prog);
	void registercvar (Program prog);
	void wasfreed (Program prog);

	void strreplace (Program prog);
	void strireplace (Program prog);

	void crc16(Program prog);
	void digest_hex(Program prog);

	void SetTraceGlobals(Program prog, const trace_t *trace);
	void ClearTraceGlobals(Program prog);

	void uri_escape (Program prog);
	void uri_unescape (Program prog);
	void whichpack (Program prog);

	void etof (Program prog);
	void uri_get (Program prog);
	void netaddress_resolve (Program prog);

	void tokenize_console (Program prog);
	void argv_start_index (Program prog);
	void argv_end_index (Program prog);

	void buf_cvarlist(Program prog);
	void cvar_description(Program prog);

	void isfunction(Program prog);
	void callfunction(Program prog);


	void getsurfacenumpoints(Program prog);
	void getsurfacepoint(Program prog);
	void getsurfacepointattribute(Program prog);
	void getsurfacenormal(Program prog);
	void getsurfacetexture(Program prog);
	void getsurfacenearpoint(Program prog);
	void getsurfaceclippedpoint(Program prog);
	void getsurfacenumtriangles(Program prog);
	void getsurfacetriangle(Program prog);


	namespace physics
	{
		void enable(Program prog);
		void addforce(Program prog);
		void addtorque(Program prog);
	}//namespace physics

	namespace client
	{
		void isdemo (Program prog);
		void videoplaying (Program prog);
		void getextresponse (Program prog);
	/*
	 * originally from clvm_cmds.h
	 */
		void setmodel ();
		void precache_model ();
		void setorigin ();
		void setattachment();
		void gettagindex();
		void gettaginfo();

		namespace renderer
		{
			void AddDynamicLight ();
			void ClearScene ();
			void AddEntities ();
			void AddEntity ();
			void SetView ();
			void RenderScene ();
			void LoadWorldModel ();

			void PolygonBegin ();
			void PolygonVertex ();
			void PolygonEnd ();
		}//namespace renderer

	}//namespace client

	namespace server
	{
		void getextresponse (Program prog);
	}//namespace server

}//namespace vm
}//namespace engine
}//namespace cloture
