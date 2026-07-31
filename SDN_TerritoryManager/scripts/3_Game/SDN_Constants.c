/*
  Arquivo: SDN_Constants.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/3_Game/SDN_Constants.c
*/

class SDN_TerritoryPerm 
{
	// Declaramos as flags primárias primeiro para evitar erros de compilação no Enforce Script
	static int ADMIN 			= 1; 	// 0000 0001
	static int DEPLOY 			= 2; 	// 0000 0010
	static int BUILD 			= 4; 	// 0000 0100
	static int DISMANTLE 		= 8; 	// 0000 1000
	static int LOWERFLAG 		= 16; 	// 0001 0000
	static int ADDMEMBER 		= 32; 	// 0010 0000
	static int REMOVEMEMBER 	= 64; 	// 0100 0000
	static int DEPLOYSPECIAL 	= 128; 	// 1000 0000 - (Whitelist de itens planejada)
	static int REMOVECARCOVER 	= 256; 	// 0001 0000 0000  

	// Permissões compostas (agora compilam com segurança)
	static int DEFAULTOWNER 	= 1;
	static int DEFAULTMEMBER 	= DEPLOY + LOWERFLAG;
	static int PUBLIC 			= LOWERFLAG + BUILD;
	static int MODERATOR 		= ADDMEMBER + REMOVEMEMBER + DEPLOY + BUILD + DISMANTLE + LOWERFLAG; // NOVO CARGO
}

class SDN_TerritoryIcons 
{
	static string NoBuildZone		= "SDN_TerritoryManager/images/Icones/NoBuildZone.edds";
	static string DistanceConflict	= "SDN_TerritoryManager/images/Icones/DistanceConflict.edds";
	static string WithinTerritory	= "SDN_TerritoryManager/images/Icones/WithinTerritory.edds";
	static string RequireTerritory	= "SDN_TerritoryManager/images/Icones/RequireTerritory.edds";
	static string DeSpawnWarning	= "SDN_TerritoryManager/images/Icones/DeSpawnWarning.edds";
	static string BuildPartWarning	= "SDN_TerritoryManager/images/Icones/BuildPartWarning.edds";
	static string MaxBuildParts		= "SDN_TerritoryManager/images/Icones/MaxBuildParts.edds";
	static string MaxStorageParts	= "SDN_TerritoryManager/images/Icones/MaxStorageParts.edds";
	static string MaxBasesReached	= "SDN_TerritoryManager/images/Icones/MaxBasesReached.edds";
	static string OneCabinPerBase	= "SDN_TerritoryManager/images/Icones/OneCabinPerBase.edds";
	static string TerritoryInteractError = "SDN_TerritoryManager/images/Icones/TerritoryCmdErrorNotInBase.edds";
	static string BaseFull			= "SDN_TerritoryManager/images/Icones/MaxMembers.edds";
	static string TerritoryCmdCreateLengthError = "SDN_TerritoryManager/images/Icones/TerritoryCmdCreateLengthError.edds";
	static string TerritoryCmdCreateAlphaError = "SDN_TerritoryManager/images/Icones/TerritoryCmdCreateAlphaError.edds";
	static string TerritoryCmdCreateSuccess = "SDN_TerritoryManager/images/Icones/TerritoryCmdCreateSuccess.edds";
	static string TerritoryCmdCreateTooFar = "SDN_TerritoryManager/images/Icones/TerritoryCmdCreateTooFar.edds";
	static string TerritoryCmdErrorNotInBase = "SDN_TerritoryManager/images/Icones/TerritoryCmdErrorNotInBase.edds";
	static string TerritoryCmdErrorNotOwner = "SDN_TerritoryManager/images/Icones/TerritoryCmdErrorNotOwner.edds";
	static string TerritoryCmdNoLimits = "SDN_TerritoryManager/images/Icones/TerritoryCmdNoLimits.edds";
	static string TerritoryCmdBaseStatus = "SDN_TerritoryManager/images/Icones/TerritoryCmdBaseStatus.edds";
	
	static string PunishmentActive	= "SDN_TerritoryManager/images/Icones/PunishmentActive.edds";
	static string PunishmentTimeLeft = "SDN_TerritoryManager/images/Icones/PunishmentTimeLeft.edds";
	static string MaxMembers		= "SDN_TerritoryManager/images/Icones/MaxMembers.edds";
}

modded class GameConstants 
{
	// Prefixo SDN adicionado para evitar conflito com engine vanilla ou outros mods
	const float SDN_REFRESHER_RADIUS = 80; // metros
}