/*
  Arquivo: config.cpp
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/config.cpp
*/

class CfgPatches
{
	class SDN_TerritoryManager
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"JM_CF_Scripts"
		};
	};
};

class CfgMods
{
	class SDN_TerritoryManager
	{
		dir="SDN_TerritoryManager";
		name="SDN_TerritoryManager";
		credits="Deceased";
		author="SafeDoNordeste";
		authorID="0";
		version="0.1";
		extra=0;
		type="mod";
		hideName=1;
		hidePicture=1;
		dependencies[]=
		{
			"Game",
			"World",
			"Mission"
		};
		class defs
		{
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"SDN_TerritoryManager/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"SDN_TerritoryManager/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"SDN_TerritoryManager/scripts/5_Mission"
				};
			};
		};
	};
};

class CfgSoundShaders
{
	class SDN_SoundShader_Base
	{
		range=50;
	};
	class SDN_Notification_Error_SoundShader: SDN_SoundShader_Base
	{
		samples[]=
		{
			
			{
				"SDN_TerritoryManager\sounds\notification_erro",
				1
			}
		};
		volume=1;
	};
	class SDN_Notification_Normal_SoundShader: SDN_SoundShader_Base
	{
		samples[]=
		{
			
			{
				"SDN_TerritoryManager\sounds\Notification1",
				1
			}
		};
		volume=1;
	};
};
class CfgSoundSets
{
	class SDN_SoundSet_Base
	{
		sound3DProcessingType="character3DProcessingType";
		volumeCurve="characterAttenuationCurve";
		spatial=1;
		doppler=0;
		loop=0;
	};
	class SDN_Notification_Error_SoundSet: SDN_SoundSet_Base
	{
		soundShaders[]=
		{
			"SDN_Notification_Error_SoundShader"
		};
		volumeFactor=1;
		spatial=0;
	};
	class SDN_Notification_Normal_SoundSet: SDN_SoundSet_Base
	{
		soundShaders[]=
		{
			"SDN_Notification_Normal_SoundShader"
		};
		volumeFactor=1;
		spatial=0;
	};
};