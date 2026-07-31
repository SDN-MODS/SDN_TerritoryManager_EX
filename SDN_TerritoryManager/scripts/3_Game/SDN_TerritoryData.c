/*
  Arquivo: SDN_TerritoryData.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/3_Game/SDN_TerritoryData.c
*/

class SDN_TerritoryMember
{
	string PlayerGUID;
	string PlayerName;
	int PermissionLevel;

	void SDN_TerritoryMember(string guid, string name, int permLevel)
	{
		PlayerGUID = guid;
		PlayerName = name;
		PermissionLevel = permLevel;
	}
}

class SDN_TerritoryData
{
	string TerritoryID; 
	string TerritoryName; // NOVO: Guarda o nome escolhido pelo jogador
	vector Position;    
	
	string OwnerGUID;
	string OwnerName; 
	bool IsOpenForInvites;
	int AbandonedTimestamp; // NOVO: Timestamp do momento que a base foi abandonada

	ref array<ref SDN_TerritoryMember> Members;

	void SDN_TerritoryData()
	{
		TerritoryName = "";
		Members = new array<ref SDN_TerritoryMember>();
		IsOpenForInvites = false;
		AbandonedTimestamp = 0;
	}

	bool HasMember(string guid)
	{
		if (!Members) return false;
		foreach (SDN_TerritoryMember member : Members)
		{
			if (member.PlayerGUID == guid) return true;
		}
		return false;
	}
}

// ==========================================
// NOVO: ESTRUTURA PARA GRAVAR O CASTIGO DE TEMPO
// ==========================================
class SDN_PlayerCooldown
{
	string PlayerGUID;
	int ExpirationTimestamp;

	void SDN_PlayerCooldown(string guid, int expiration)
	{
		PlayerGUID = guid;
		ExpirationTimestamp = expiration;
	}
}

class SDN_TerritoryDatabaseRoot
{
	ref array<ref SDN_TerritoryData> Territories;
	ref array<ref SDN_PlayerCooldown> PlayerCooldowns; // A nossa "Lista Negra" de castigos

	void SDN_TerritoryDatabaseRoot()
	{
		Territories = new array<ref SDN_TerritoryData>();
		PlayerCooldowns = new array<ref SDN_PlayerCooldown>();
	}
}