/*
  Arquivo: SDN_TerritoryConfig.c
  Caminho: SDN_MODS/SDN_TerritoryManager/scripts/3_Game/SDN_TerritoryConfig.c
*/

// ==========================================
// CLASSES AUXILIARES DE MENSAGENS (JSON)
// ==========================================
class SDN_ToastTitles
{
	string RadarTitle = "Radar de Território";
	string AlertTitle = "Aviso do Sistema";
	string ErrorTitle = "Erro de Território";
	string MemberTitle = "Gestão de Membros";
}

class SDN_RadarToasts
{
	string EnteredTerritory = "Você entrou em: %1";
	string LeftTerritory = "Você saiu de: %1";
	string TerritoryRenamed = "A base agora chama-se: %1";
	string UnnamedFallback = "Território Não Nomeado";
}

class SDN_AlertMessages
{
	string NoBuildZone = "Não podes construir aqui. Zona restrita!";
	string DistanceConflict = "Muito perto de outra base! Mínimo: %1m";
	string WithinTerritory = "Não podes construir no território inimigo!";
	string RequireTerritory = "Você precisa de um território para isto!";
	string DeSpawnWarning = "Atenção: %2 sumirá em %1 se construído aqui.";
	string BuildPartWarning = "Atenção: A peça sumirá em %1.";
	string MaxBuildParts = "Limite máximo de %1 peças da base atingido!";
	string MaxStorageParts = "Limite máximo de %1 armazenamentos atingido!";
	string OneCabinPerBase = "Você só pode construir 1 cabana por base!";
	string PunishmentActive = "Castigo ativo! Não podes criar bases.";
	string MaxBasesReached = "Já tens o limite de %1 bases permitidas.";
	string PunishmentTimeLeft = "Estás sob castigo! Faltam %1.";
	string MaxMembers = "Esta base já atingiu o limite de membros!";
	
	// Server Management Commands / Base
	string TerritoryCmdErrorNotInBase = "Erro: Você precisa estar dentro de um território para usar este comando.";
	string TerritoryCmdErrorNotOwner = "Erro: Apenas o Dono do território pode ver o limite de peças.";
	string TerritoryCmdNoLimits = "Este servidor não possui limite de peças ativado.";
	string TerritoryCmdBaseStatus = "Total: %1  ->  Placed: %2  ->  Remaining: %3";
	
	// Create Command
	string TerritoryCmdCreateLengthError = "Erro: O nome da base deve ter entre 3 e 20 caracteres!";
	string TerritoryCmdCreateAlphaError = "Erro: Apenas letras, números e espaços permitidos.";
	string TerritoryCmdCreateSuccess = "Sucesso! A base agora chama-se: %1";
	string TerritoryCmdCreateTooFar = "Erro: Aproxime-se da sua bandeira sem nome para usar o comando.";
	
	// Interact Events
	string TerritoryInteractInviteEnabled = "Convites ativados! Peça ao jogador para interagir com a bandeira.";
	string TerritoryInteractInviteRevoked = "Convites revogados. Ninguém mais pode entrar.";
	string TerritoryInteractBaseReset = "Território resetado com sucesso (Admin).";
}

class SDN_ChatOnlyMessages
{
	string BaseAbandoned = "Abandonaste a base! Castigo de %1.";
	string BaseTransferredToYou = "O território '%1' foi transferido para você!";
	string BaseTransferredFromYou = "O território foi transferido de %1 para %2.";
	string BaseAbandonedGlobalAlert = "Atenção: Base abandonada na localização X: %1 | Y: %2";
	string MemberKicked = "Expulsaste %1 da base! Ele recebeu castigo.";
	string PromotedModerator = "Você promoveu %1 a Moderador!";
	string DemotedMember = "Você despromoveu %1 para Membro Normal.";
}

class SDN_MessageSettings
{
	int NotificationMode = 2; // 0 = Off, 1 = Toast, 2 = Toast + Chat, 3 = Only Chat
	int AntiSpamCooldownMs = 3000;
	
	ref SDN_ToastTitles ToastTitles = new SDN_ToastTitles();
	ref SDN_RadarToasts RadarToasts = new SDN_RadarToasts();
	ref SDN_AlertMessages Alerts = new SDN_AlertMessages();
	ref SDN_ChatOnlyMessages ChatOnly = new SDN_ChatOnlyMessages();
}

// ==========================================
// CLASSE AUXILIAR: ZONAS RESTRITAS (NO BUILD ZONES)
// ==========================================
class SDN_NoBuildZone
{
	string Name = "";
	float X;
	float Z;
	float R; // Raio da zona em metros
	bool DrawOnMap = false;
	
	void SDN_NoBuildZone(float x, float z, float r, string name = "", bool drawOnMap = false)
	{
		X = x;
		Z = z;
		R = r;
		Name = name;
		DrawOnMap = drawOnMap;
	}
	
	bool Check(vector checkPos)
	{
		if (checkPos)
		{
			vector ZeroedHeightPos = Vector(checkPos[0], 0, checkPos[2]);
			vector ZeroedZonePos = Vector(X, 0, Z);
			
			if (vector.Distance(ZeroedHeightPos, ZeroedZonePos) <= R)
			{
				return true;
			}
		}
		return false;
	}
}

// ==========================================
// CLASSE PRINCIPAL: CONFIGURAÇÃO DO MOD
// ==========================================
class SDN_TerritoryConfig
{
	[NonSerialized()]
	protected static string m_SDN_DirPATH = "$profile:SDN_MODS\\SDN_TerritoryManager";
	
	[NonSerialized()]
	protected static string m_SDN_ConfigPATH = "$profile:SDN_MODS\\SDN_TerritoryManager\\Config.json";

	[NonSerialized()]
	private static ref SDN_TerritoryConfig m_SDN_Instance;

	string ConfigVersion = "1.1";
	
	float TerritoryRadius = 30.0; 
	float TentRadius = 0;
	float TentCESpawnLifeTime = 5600;
	float BuildBonusSledgeDamage = 300;
	int FlagRefreshFrequency = 432000;
	bool RequireTerritory = true; 
	
	// NOVO: Regras de Ouro de População e Punição
	int MaxMembersPerTerritory = 10;
	int MaxTerritoriesPerPlayer = 1;
	float LeaveTerritoryCooldownMinutes = 1440.0;
	int DeleteAbandonedBaseCooldownMinutes = 1440;
	int AlertAbandonedBaseIntervalMinutes = 30;
	
	// NOVO: Distância Mínima entre Bases (Evita Mega-Cidades)
	float MinDistanceBetweenTerritories = 150.0; // Distância em metros
	
	// NOVO: Limite máximo de cadeados por território (-1 para infinito)
	int MaxCodeLocksPerTerritory = 3;
	
	// NOVO: Modo de construção automática da bandeira
	int AutoBuildFlagpoleMode = 3; // 0=Vanilla, 1=Base+Suporte, 2=+Mastro, 3=+Bandeira
	
	// NOVO: Lifetime fora de territórios
	int LifeTimeStorageVanilla = 3888000;
	int EnableNotificationSounds = 1;
	int PreventEnemyDismantle = 1;
	int TeleportEnemyOnLogin = 0;
	
	// CORREÇÃO: As variáveis que tinham desaparecido voltaram ao seu lugar!
	int BaseBuildPartsMax = 5; 
	ref TStringArray BaseBuildParts; 
	
	int BaseStoragePartsMax;
	ref TStringArray BaseStorageParts;
	
	ref TStringArray ServerAdmins; 
	ref TStringArray WhiteList; 
	ref map<string, int> KitLifeTimes; 
	ref array<ref SDN_NoBuildZone> NoBuildZones; 

	ref SDN_MessageSettings MessageSettings;

	void SDN_TerritoryConfig() 
	{
		ServerAdmins = new TStringArray;
		WhiteList = new TStringArray;
		KitLifeTimes = new map<string, int>;
		NoBuildZones = new array<ref SDN_NoBuildZone>;
		BaseBuildParts = new TStringArray; 
		BaseStorageParts = new TStringArray;
		EnableNotificationSounds = 1;
		PreventEnemyDismantle = 1;
		TeleportEnemyOnLogin = 0;
	}
	
	static SDN_TerritoryConfig Get()
	{
		if (!m_SDN_Instance)
		{
			m_SDN_Instance = SDN_TerritoryConfig.Load();
		}
		return m_SDN_Instance;
	}

	static void Set(SDN_TerritoryConfig instance)
	{
		m_SDN_Instance = instance;
	}

	string GetKitDeSpawnWarning(int LifeTime, int paramFlagRefreshFrequency = 0)
	{
		int Hours = LifeTime / 3600;
		if (paramFlagRefreshFrequency == 0) paramFlagRefreshFrequency = FlagRefreshFrequency;
		if (LifeTime < paramFlagRefreshFrequency) return ""; 

		int Days = Math.Floor(Hours / 24);
		int remander = Days % 7;
		
		if (remander >= 2) return Days.ToString() + " Dias";

		int Weeks = Math.Floor(Days / 7);
		return Weeks.ToString() + " Semanas";
	}
	
	int GetKitLifeTime(string item)
	{
		item.ToLower();
		int lt = KitLifeTimes.Get(item);
		
		if (lt == 0)
		{
			foreach (string key, int lifetime : KitLifeTimes)
			{
				key.ToLower(); 
				if (item.Contains(key)) return lifetime;
			}
		} 
		else return lt;
		
		return 0; 
	}

	static ref SDN_TerritoryConfig Load()
	{
		ref SDN_TerritoryConfig config = new SDN_TerritoryConfig();

		if (!FileExist("$profile:SDN_MODS")) MakeDirectory("$profile:SDN_MODS");
		if (!FileExist(m_SDN_DirPATH)) MakeDirectory(m_SDN_DirPATH);

		if (FileExist(m_SDN_ConfigPATH))
		{
			JsonFileLoader<SDN_TerritoryConfig>.JsonLoadFile(m_SDN_ConfigPATH, config);
			
			if (!config.ServerAdmins) config.ServerAdmins = new TStringArray;
			if (!config.WhiteList) config.WhiteList = new TStringArray;
			if (!config.KitLifeTimes) config.KitLifeTimes = new map<string, int>;
			if (!config.NoBuildZones) config.NoBuildZones = new array<ref SDN_NoBuildZone>;
			if (!config.BaseBuildParts) config.BaseBuildParts = new TStringArray; 
			if (!config.BaseStorageParts) config.BaseStorageParts = new TStringArray; 
			
			if (!config.MessageSettings)
			{
				config.MessageSettings = new SDN_MessageSettings();
				config.Save();
			}
		}
		else
		{
			config.LoadDefaultSettings();
			config.Save();
		}
		return config;
	}

	void Save()
	{
		JsonFileLoader<SDN_TerritoryConfig>.JsonSaveFile(m_SDN_ConfigPATH, this);
	}

	void LoadDefaultSettings()
	{
		ConfigVersion = "1.3";
		TerritoryRadius = 20.0; 
		TentRadius = 0;
		TentCESpawnLifeTime = 5600;
		BuildBonusSledgeDamage = 300;
		FlagRefreshFrequency = 432000;
		RequireTerritory = true;
		
		// Regras de Ouro
		MaxMembersPerTerritory = 10;
		MaxTerritoriesPerPlayer = 1;
		LeaveTerritoryCooldownMinutes = 1440.0;
		DeleteAbandonedBaseCooldownMinutes = 1440;
		AlertAbandonedBaseIntervalMinutes = 30;
		MinDistanceBetweenTerritories = 150.0; 
		MaxCodeLocksPerTerritory = 3; // Padrão de cadeados permitido
		
		// Padrão: 0 (Vanilla) para não assustar novos donos de servidores
		AutoBuildFlagpoleMode = 3; 
		
		LifeTimeStorageVanilla = 3888000;
		EnableNotificationSounds = 1;
		PreventEnemyDismantle = 1;
		TeleportEnemyOnLogin = 0;
		
		WhiteList.Insert("Trap");
		WhiteList.Insert("Paper");
		WhiteList.Insert("Fireplace");
		WhiteList.Insert("WrittenNote");
		WhiteList.Insert("ClaymoreMine");
		WhiteList.Insert("PowerGenerator");
		WhiteList.Insert("Plastic_Explosive");
		WhiteList.Insert("BarrelHoles_"); // PONTO 3: Permitir fogueiras improvisadas em qualquer lugar
		
		KitLifeTimes.Insert("fencekit", 3600);
		KitLifeTimes.Insert("watchtowerkit", 3600);
		KitLifeTimes.Insert("territoryflagkit", 3600);
		KitLifeTimes.Insert("bbp_", 3888000);
		KitLifeTimes.Insert("msp_", 3888000);

		NoBuildZones.Insert(new SDN_NoBuildZone(8024.0, 9283.0, 300.0, "Altar_Trader", true));
		NoBuildZones.Insert(new SDN_NoBuildZone(3700.0, 5980.0, 400.0, "Green_Mountain", true));

		BaseBuildParts.Insert("Fence");
		BaseBuildParts.Insert("Watchtower");

		BaseStorageParts.Insert("WoodenCrate");
		BaseStorageParts.Insert("SeaChest");
		
		// ==========================================
		// PONTO 2: MUDANÇA PARA 'Barrel_' GERAL
		// ==========================================
		BaseStorageParts.Insert("Barrel_"); 
		
		BaseStorageParts.Insert("MediumTent");
		BaseStorageParts.Insert("LargeTent");
		BaseStorageParts.Insert("CarTent");
		BaseStorageParts.Insert("PartyTent");
		BaseStorageParts.Insert("ShelterKit");
		
		MessageSettings = new SDN_MessageSettings();
	}

	bool IsStorageItem(string itemType)
	{
		if (!BaseStorageParts || BaseStoragePartsMax <= 0) return false;
		
		itemType.ToLower();
		foreach (string storageClass : BaseStorageParts)
		{
			string checkClass = storageClass; 
			checkClass.ToLower();
			if (itemType.Contains(checkClass)) return true;
		}
		return false;
	}
}