/*
  Arquivo: SDN_MissionGameplay.c
  Caminho: SDN_MODS/SDN_TerritoryManager/scripts/5_Mission/mission/SDN_MissionGameplay.c
*/

modded class MissionGameplay extends MissionBase
{	
	protected ref SDN_TerritoryConfig m_SDN_Config;

	protected string m_SDN_CurrentTerritoryID = "";
	protected string m_SDN_CurrentTerritoryName = "";
	
	protected int m_SDN_ConfigRetries = 0;

	void MissionGameplay() 
	{
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_RPCModSettings", this, SingeplayerExecutionType.Both);
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_PlaySound", this, SingeplayerExecutionType.Client);
	}

	void SDN_PlaySound(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Client)
		{
			Param2<string, bool> data;
			if (!ctx.Read(data)) return;
			
			SDN_MessageManager.SDN_PlayLocalSound(data.param2);
		}
	}
	
	override void OnMissionStart()
	{
		super.OnMissionStart();
		GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPCModSettings", new Param1<SDN_TerritoryConfig>(null), true, null);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SDN_TerritoryRadarTick, 3000, true);
	}

	void SDN_TerritoryRadarTick()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !player.IsAlive()) return;

		// ==========================================
		// SISTEMA DE EMERGÊNCIA (FAILSAFE) DA CONFIGURAÇÃO
		// ==========================================
		if (!m_SDN_Config)
		{
			m_SDN_ConfigRetries++;
			GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPCModSettings", new Param1<SDN_TerritoryConfig>(null), true, null);
			
			if (m_SDN_ConfigRetries < 5) return;
			else 
			{
				Print("[SDN_TerritoryManager] Aviso: Usando configurações de fallback local (30m).");
				m_SDN_Config = new SDN_TerritoryConfig();
				m_SDN_Config.TerritoryRadius = 30.0;
			}
		}

		vector playerPos = player.GetPosition();
		
		float radius = m_SDN_Config.TerritoryRadius;
		float radiusSq = radius * radius;

		TerritoryFlag nearestFlag = null;
		float closestDistSq = radiusSq + 1.0;

		if (TerritoryFlag.m_SDN_AllFlags)
		{
			foreach (TerritoryFlag flag : TerritoryFlag.m_SDN_AllFlags)
			{
				if (!flag) continue;
				float distSq = vector.DistanceSq(playerPos, flag.GetPosition());
				
				if (distSq <= radiusSq && distSq < closestDistSq)
				{
					closestDistSq = distSq;
					nearestFlag = flag;
				}
			}
		}

		string nearestID = "";
		string nearestName = "";

		if (nearestFlag)
		{
			// ==========================================
			// A "TRAVA ABSOLUTA" (FANTASMA DE LOGIN)
			// 1. Se a bandeira nunca sacou os dados da internet na vida, força o pedido e aborta!
			// ==========================================
			if (!nearestFlag.SDN_HasSyncedOnce())
			{
				if (!nearestFlag.SDN_IsRequestingSync())
				{
					nearestFlag.SDN_SyncTerritory();
				}
				return; // Fica calado, nem mostra notificação de entrada!
			}

			// 2. Se ela já tem dados mas está a fazer um refresh no momento, aguarda!
			if (nearestFlag.SDN_IsRequestingSync()) return;

			// ==========================================
			// OPÇÃO A (FURTIVA) - Ignorar Bases Incompletas
			// ==========================================
			// Se a bandeira não tem dono (órfã) ou ainda não tem nome, o radar ignora-a!
			if (nearestFlag.SDN_CanReceiveNewOwner() || nearestFlag.SDN_GetTerritoryName() == "")
			{
				nearestID = "";
				nearestName = "";
			}
			else
			{
				nearestID = nearestFlag.SDN_GetTerritoryID();
				nearestName = nearestFlag.SDN_GetTerritoryName();
			}
		}

		if (nearestID != m_SDN_CurrentTerritoryID)
		{
			if (m_SDN_CurrentTerritoryID != "")
			{
				string leftName = m_SDN_CurrentTerritoryName;
				if (leftName == "") leftName = m_SDN_Config.MessageSettings.RadarToasts.UnnamedFallback;
				
				string msgOut = m_SDN_Config.MessageSettings.RadarToasts.LeftTerritory;
				msgOut.Replace("%1", leftName);
				GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_ShowToast", new Param2<string, string>(msgOut, "LeftTerritory"), true, null);
			}

			if (nearestID != "")
			{
				string enterName = nearestName;
				if (enterName == "") enterName = m_SDN_Config.MessageSettings.RadarToasts.UnnamedFallback;
				
				string msgIn = m_SDN_Config.MessageSettings.RadarToasts.EnteredTerritory;
				msgIn.Replace("%1", enterName);
				GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_ShowToast", new Param2<string, string>(msgIn, "EnteredTerritory"), true, null);
			}

			m_SDN_CurrentTerritoryID = nearestID;
			m_SDN_CurrentTerritoryName = nearestName;
		}
		else if (nearestID != "" && nearestName != m_SDN_CurrentTerritoryName)
		{
			m_SDN_CurrentTerritoryName = nearestName;
			
			if (nearestName != "")
			{
				string msgRen = m_SDN_Config.MessageSettings.RadarToasts.TerritoryRenamed;
				msgRen.Replace("%1", nearestName);
				GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_ShowToast", new Param2<string, string>(msgRen, "TerritoryRenamed"), true, null);
			}
		}
	}

	void SDN_RPCModSettings(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) 
	{
		Param1<SDN_TerritoryConfig> data; 
		if (!ctx.Read(data)) return;

		m_SDN_Config = data.param1;
		SDN_TerritoryConfig.Set(m_SDN_Config);

		// ==========================================
		// INTEGRAÇÃO COM BASICMAP (Zonas Restritas)
		// ==========================================
		#ifdef BASICMAP
		if (m_SDN_Config && m_SDN_Config.NoBuildZones)
		{
			BasicMap().ClearMarkers("SDN_NoBuildZones");
			bool hasZonesToDraw = false;

			for (int i = 0; i < m_SDN_Config.NoBuildZones.Count(); i++)
			{
				SDN_NoBuildZone zone = m_SDN_Config.NoBuildZones.Get(i);
				
				if (zone && zone.DrawOnMap)
				{
					vector zonePos = Vector(zone.X, 0, zone.Z);
					BasicMapCircleMarker tmpMarker = new BasicMapCircleMarker(zone.Name, zonePos, SDN_TerritoryIcons.NoBuildZone, {189, 38, 78}, 150);
					tmpMarker.SetRadius(zone.R);
					tmpMarker.SetShowCenterMarker(true);
					tmpMarker.SetHideIntersects(true);
					tmpMarker.SetCanEdit(false);
					BasicMap().AddMarker("SDN_NoBuildZones", tmpMarker);
					hasZonesToDraw = true;
				}
			}
		}
		#endif
	}

	SDN_TerritoryConfig SDN_GetConfig()
	{
		return m_SDN_Config;
	}
	
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		UIScriptedMenu menu = NULL;
		menu = super.CreateScriptedMenu(id);
		if (menu) return menu;
		
		if (id == 748392)
		{
			menu = new SDN_TerritoryMenu();
			menu.SetID(id);
		}
		
		return menu;
	}
}