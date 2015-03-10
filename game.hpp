#pragma once
#ifdef BUILD_CLOTURE_GAME
/*
	Code replacing the QuakeC VM 
	Instead we just use C++
*/
namespace cloture::game
{
	namespace types
	{
		/**
			Program (alias for prvm_prog_t)
			structure defined in progsvm.h
		*/
		using Program 	= prvm_prog_t;
		/**
			Edict (alias for prvm_edict_t)
			structure defined in progsvm.h
		*/
		using Edict 	= prvm_edict_t;
		/**
			Entity (alias for entity_t)
			structure defined in client.h
		*/
		using Entity	= entity_t;
		/**
			Function (alias for func_t)
			defined in pr_comp.h as unsigned int
		*/
		using Function	= func_t; 
		/**
			String (alias for string_t)
			defined in pr_comp.h as int
		*/
		using String 	= string_t; 
	};

	namespace client
	{

		/*	
			called from cl_parse.cpp "CL_SetupWorldModel"	
			replaces CL_VM_Init
		*/
		void init();
	};
	
	namespace server
	{
		/*
			called from sv_main.cpp " SV_SpawnServer"
			replaces SV_VM_Setup
		*/
		void init();
	};
};

namespace cloture::game::shared::api
{
	using namespace cloture::game::types;
	using cloture::util::vector::vector3D;
	
	namespace Edicts 
	{
		/**
			Edicts::spawn
			replaces VM_spawn in prvm_cmds.cpp
		*/
		Edict* spawn(Program* program);
		/**
			Edicts::remove
			replaces VM_remove in prvm_cmds.cpp
		*/
		void remove(Program* program, Edict* edict);
		
		/**
			Edicts::next
			replaces VM_nextent in prvm_cmds.cpp
		*/
		
		Edict* next(Program* program, Edict* edict);
		
		/**
			Edicts::find
			replaces VM_find in prvm_cmds.cpp
		*/
		Edict* find(Program* program, Edict* start, const char* fieldName, const char* match);
	};
};

namespace cloture::game::server::api
{
	using namespace cloture::game::types;
	using cloture::util::vector::vector3D;
	namespace Edicts
	{

		/**
			Edicts::setOrigin
			replaces VM_SV_setorigin in svvm_cmds.cpp
		*/
		void setOrigin(Program* program, Edict* edict, vector3D newOrigin);
		/**
			Edicts::setModel
			replaces VM_SV_setmodel in svvm_cmds.cpp
		*/
		void setModel(Program* program, Edict* edict, const char* modelName);
	};
	namespace Precache
	{
		/**
			Precache::sound
			replaces VM_SV_precache_sound in svvm_cmds.cpp
		*/
		void sound(Program* program, const char* soundName);
		/**
			Precache::model
			replaces VM_SV_precache_model in svvm_cmds.cpp
		*/
		void model(Program* program, const char* modelName);
	};
};
namespace cloture::game::client::api
{
	
};
#endif //BUILD_CLOTURE_GAME
