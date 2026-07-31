/*
  Arquivo: SDN_TerritoryMembers.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/3_Game/SDN_TerritoryMembers.c
  Objetivo: Gere a memória RAM dos membros de um território (Nomes, GUIDs e Permissões/Cargos)
*/

class SDN_TerritoryMembers
{
	ref map<string, int> m_Members;
	ref map<string, string> m_MemberNames; // Mapeia GUID -> Nome real para a UI e JSON
	
	ref TStringArray m_OnlinePlayers; // Array serializado via RPC
	string m_OwnerName = ""; // Nome do dono guardado na estrutura para sync fácil
		
	void SDN_TerritoryMembers()
	{
		m_Members = new map<string, int>;
		m_MemberNames = new map<string, string>;
		m_OnlinePlayers = new TStringArray;
	}
	
	bool CheckId(string guid)
	{
		if (!m_Members) m_Members = new map<string, int>;
		return m_Members.Contains(guid);
	}
	
	bool Check(string guid, int permission)
	{
		if (CheckId(guid))
		{
			int perms = m_Members.Get(guid);
			if (perms)
			{
				int CalcedPerms = perms & permission;
				if (CalcedPerms == permission || perms == SDN_TerritoryPerm.DEFAULTOWNER) 
				{
					return true;
				}
			}
		} 
		return false;
	}
	
	// Adiciona um membro com nome e nível de permissão (Cargo)
	bool AddMember(string guid, string name = "Sobrevivente", int permission = -1)
	{
		// BLINDADO: Se não for especificada uma permissão, usa a padrão fixa
		if (permission == -1) 
		{
			permission = SDN_TerritoryPerm.DEFAULTMEMBER;
		}

		if (!CheckId(guid))
		{
			m_Members.Insert(guid, permission);
			m_MemberNames.Insert(guid, name); // Guarda o nome na memória
			return true;
		}
		return false;
	}
	
	bool RemoveMember(string guid)
	{
		if (CheckId(guid))
		{
			m_Members.Remove(guid);
			if (m_MemberNames && m_MemberNames.Contains(guid)) 
			{
				m_MemberNames.Remove(guid);
			}
			return true;
		}
		return false;
	}

	// Recupera o nome amigável para a interface (Menu Scroll)
	string GetName(string guid)
	{
		if (m_MemberNames && m_MemberNames.Contains(guid))
		{
			return m_MemberNames.Get(guid);
		}
		return "Desconhecido";
	}

	// NOVO: Recupera o cargo/permissão do jogador
	int GetPermission(string guid)
	{
		if (m_Members && m_Members.Contains(guid)) 
		{
			return m_Members.Get(guid);
		}
		return 0; // 0 significa sem permissões
	}

	// NOVO: Define um novo cargo/permissão para o jogador (Promover/Despromover)
	void SetPermission(string guid, int perm)
	{
		if (m_Members && m_Members.Contains(guid)) 
		{
			m_Members.Set(guid, perm);
		}
	}
	
	// Retorna a lista de todos os GUIDs dos membros
	array<string> GetMemberArray()
	{
		array<string> members = new array<string>;
		if (!m_Members) return members;

		for (int i = 0; i < m_Members.Count(); i++)
		{
			members.Insert(m_Members.GetKey(i));
		}
		return members;
	}
}