/*
  Arquivo: SDN_TerritoryDatabase.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/3_Game/SDN_TerritoryDatabase.c
  Objetivo: O "Maestro" que gere a leitura, escrita e backups do ficheiro JSON na Memória RAM.
*/

class SDN_TerritoryDatabase
{
	protected static ref SDN_TerritoryDatabase m_Instance;
	ref SDN_TerritoryDatabaseRoot m_Data;

	// CORREÇÃO: Usando barras duplas para compatibilidade total e forçada com o Windows Server
	protected string m_RootPath   = "$profile:SDN_MODS";
	protected string m_ModPath    = "$profile:SDN_MODS\\SDN_TerritoryManager";
	protected string m_DataPath   = "$profile:SDN_MODS\\SDN_TerritoryManager\\SDN_TerritoryManager_Data";
	
	protected string m_FilePath   = "$profile:SDN_MODS\\SDN_TerritoryManager\\SDN_TerritoryManager_Data\\SDN_TerritoriesData.json";
	protected string m_BackupPath = "$profile:SDN_MODS\\SDN_TerritoryManager\\SDN_TerritoryManager_Data\\SDN_TerritoriesData.bak";

	protected bool m_IsDirty = false;

	static SDN_TerritoryDatabase Get()
	{
		if (!m_Instance)
		{
			m_Instance = new SDN_TerritoryDatabase();
			m_Instance.Load();
		}
		return m_Instance;
	}

	void Load()
	{
		if (!FileExist(m_RootPath)) MakeDirectory(m_RootPath);
		if (!FileExist(m_ModPath)) MakeDirectory(m_ModPath);
		if (!FileExist(m_DataPath)) MakeDirectory(m_DataPath);

		m_Data = new SDN_TerritoryDatabaseRoot();

		if (FileExist(m_FilePath))
		{
			JsonFileLoader<SDN_TerritoryDatabaseRoot>.JsonLoadFile(m_FilePath, m_Data);
			
			// === A CORREÇÃO DE OURO ESTÁ AQUI ===
			if (!m_Data) m_Data = new SDN_TerritoryDatabaseRoot();
			if (!m_Data.Territories) m_Data.Territories = new array<ref SDN_TerritoryData>();
			if (!m_Data.PlayerCooldowns) m_Data.PlayerCooldowns = new array<ref SDN_PlayerCooldown>(); // Proteção extra

			Print("[SDN_TerritoryDatabase] Master JSON carregado com sucesso. Territórios registados: " + m_Data.Territories.Count());
		}
		else
		{
			// Previne o mesmo erro ao criar o ficheiro do zero
			if (!m_Data.Territories) m_Data.Territories = new array<ref SDN_TerritoryData>();
			Save();
			Print("[SDN_TerritoryDatabase] Novo ficheiro Master JSON criado em: " + m_DataPath);
		}
	}

	void Save()
	{
		if (FileExist(m_FilePath))
		{
			JsonFileLoader<SDN_TerritoryDatabaseRoot>.JsonSaveFile(m_BackupPath, m_Data);
		}
		
		JsonFileLoader<SDN_TerritoryDatabaseRoot>.JsonSaveFile(m_FilePath, m_Data);
		m_IsDirty = false; 
	}

	void MarkDirty()
	{
		m_IsDirty = true;
	}

	void SaveIfDirty()
	{
		if (m_IsDirty)
		{
			Save();
		}
	}

	SDN_TerritoryData GetTerritory(string territoryID)
	{
		if (!m_Data || !m_Data.Territories) return null;

		foreach (SDN_TerritoryData terr : m_Data.Territories)
		{
			if (terr.TerritoryID == territoryID)
			{
				return terr;
			}
		}
		return null;
	}

	void AddOrUpdateTerritory(SDN_TerritoryData newTerritory)
	{
		// === A CORREÇÃO DE OURO PARTE 2 ===
		if (!m_Data) m_Data = new SDN_TerritoryDatabaseRoot();
		if (!m_Data.Territories) m_Data.Territories = new array<ref SDN_TerritoryData>();
		if (!m_Data.PlayerCooldowns) m_Data.PlayerCooldowns = new array<ref SDN_PlayerCooldown>(); // Proteção extra

		for (int i = 0; i < m_Data.Territories.Count(); i++)
		{
			if (m_Data.Territories.Get(i).TerritoryID == newTerritory.TerritoryID)
			{
				m_Data.Territories.Remove(i);
				break;
			}
		}

		m_Data.Territories.Insert(newTerritory);
		Save(); // Gravação forçada
	}

	void RemoveTerritory(string territoryID)
	{
		if (!m_Data || !m_Data.Territories) return;

		for (int i = 0; i < m_Data.Territories.Count(); i++)
		{
			if (m_Data.Territories.Get(i).TerritoryID == territoryID)
			{
				m_Data.Territories.Remove(i);
				Save(); // Gravação forçada ao destruir base
				return;
			}
		}
	}
	
	// ==========================================
	// NOVAS FUNÇÕES: GESTÃO DE REGRAS E TEMPO REAL (UNIX)
	// ==========================================
	
	// Converte a data nativa do Motor numa linha do tempo Universal (Segundos)
	int SDN_GetUnixTimestamp()
	{
		int year, month, day, hour, minute, second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);

		int days = day - 1;
		int y = year;
		if (month <= 2) y -= 1;
		int m = month;
		if (m <= 2) m += 12;
		days += (153 * m - 457) / 5;
		days += 365 * y + y / 4 - y / 100 + y / 400;
		days -= 719468;
		return (days * 86400) + (hour * 3600) + (minute * 60) + second;
	}

	void SetCooldown(string guid, float minutes)
	{
		if (minutes <= 0) return;
		if (!m_Data) m_Data = new SDN_TerritoryDatabaseRoot();
		if (!m_Data.PlayerCooldowns) m_Data.PlayerCooldowns = new array<ref SDN_PlayerCooldown>();

		int currentTimestamp = SDN_GetUnixTimestamp();
		int expiration = currentTimestamp + (minutes * 60); // Converte minutos em segundos

		for (int i = 0; i < m_Data.PlayerCooldowns.Count(); i++)
		{
			if (m_Data.PlayerCooldowns.Get(i).PlayerGUID == guid)
			{
				m_Data.PlayerCooldowns.Get(i).ExpirationTimestamp = expiration;
				Save();
				return;
			}
		}
		m_Data.PlayerCooldowns.Insert(new SDN_PlayerCooldown(guid, expiration));
		Save();
	}

	int GetCooldownRemaining(string guid)
	{
		if (!m_Data || !m_Data.PlayerCooldowns) return 0;
		int currentTimestamp = SDN_GetUnixTimestamp();

		for (int i = 0; i < m_Data.PlayerCooldowns.Count(); i++)
		{
			if (m_Data.PlayerCooldowns.Get(i).PlayerGUID == guid)
			{
				int exp = m_Data.PlayerCooldowns.Get(i).ExpirationTimestamp;
				if (exp > currentTimestamp) return exp - currentTimestamp;
				else
				{
					// Se o tempo já passou, remove o jogador da lista negra automaticamente!
					m_Data.PlayerCooldowns.Remove(i);
					Save();
					return 0;
				}
			}
		}
		return 0;
	}

	int GetActiveTerritoryCount(string guid)
	{
		if (!m_Data || !m_Data.Territories) return 0;
		int count = 0;
		foreach (SDN_TerritoryData terr : m_Data.Territories)
		{
			if (terr.OwnerGUID == guid || terr.HasMember(guid)) count++;
		}
		return count;
	}
}