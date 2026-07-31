/*
  Arquivo: SDN_TerritoryFlag.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/4_World/SDN_TerritoryFlag.c
*/

modded class TerritoryFlag extends BaseBuildingBase
{
	// ==========================================
	// VARIÁVEIS ESTÁTICAS GLOBAIS (CACHE OTIMIZADO)
	// ==========================================
	static ref array<TerritoryFlag> m_SDN_AllFlags = new array<TerritoryFlag>;
	static ref map<string, TerritoryFlag> m_SDN_NamingPlayers = new map<string, TerritoryFlag>;

	static vector m_SDN_LastCheckLocation = vector.Zero;
	static int m_SDN_LastCheckLocationNextTime = 0;
	static bool m_SDN_CachedHasTerritoryPerm = false;

	static TerritoryFlag m_SDN_TargetFlagForMenu = null;

	// ==========================================
	// VARIÁVEIS DE INSTÂNCIA
	// ==========================================
	protected bool m_SDN_CanAddMember 		= false;
	protected bool m_SDN_AwaitingReset 		= false;
	protected string m_SDN_TerritoryOwner 	= "";
	protected bool m_SDN_IsRequestingSync 	= false;
	
	// NOVO: Memória que garante que a bandeira sacou os dados do servidor pelo menos 1 vez na vida
	protected bool m_SDN_HasSyncedOnce      = false;
	
	protected int m_SDN_LastSyncTime 		= 0;
	protected string m_SDN_TerritoryName    = ""; 
	protected string m_SDN_TerritoryOwnerName = "";

	ref TStringArray m_SDN_OnlineMembersCache = new TStringArray();
	
	ref SDN_TerritoryMembers m_SDN_TerritoryMembers = new SDN_TerritoryMembers();
	
	// ==========================================
	// CONSTRUTOR / DESTRUTOR
	// ==========================================
	void TerritoryFlag()
	{
		RegisterNetSyncVariableBool("m_SDN_CanAddMember");
	}

	void ~TerritoryFlag()
	{
		if (m_SDN_IsRequestingSync && GetGame())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.SDN_DoFirstSync);
		}
		
		if (m_SDN_AllFlags && m_SDN_AllFlags.Find(this) != -1)
		{
			m_SDN_AllFlags.RemoveItem(this);
		}
	}
	
	// ==========================================
	// INICIALIZAÇÃO E EVENTOS CORE
	// ==========================================
	override void EEInit()
	{
		super.EEInit();

		if (m_SDN_AllFlags && m_SDN_AllFlags.Find(this) == -1)
		{
			m_SDN_AllFlags.Insert(this);
		}

		if (GetGame().IsClient())
		{
			// CORREÇÃO: Já não dependemos do jogador estar vivo ou instanciado para pedir os dados.
			// A bandeira pede automaticamente 2 segundos após nascer no mapa do cliente!
			m_SDN_IsRequestingSync = true;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SDN_DoFirstSync, 2000, false);
			AnimateFlag(1 - GetRefresherTime01());
		}
	}

	// ==========================================
	// GETTERS E SETTERS BÁSICOS
	// ==========================================
	string SDN_GetTerritoryID()
	{
		vector pos = GetPosition();
		int x = Math.Round(pos[0]);
		int z = Math.Round(pos[2]);
		return "Base_" + x.ToString() + "_" + z.ToString();
	}

	string SDN_GetTerritoryName() { return m_SDN_TerritoryName; }

	void SDN_SetTerritoryName(string name)
	{
		m_SDN_TerritoryName = name;
		SDN_UpdateDatabase();
		SDN_SyncTerritory();
	}

	void SDN_PromoteModerator(string targetGUID)
	{
		if (targetGUID == m_SDN_TerritoryOwner || targetGUID == "") return;
		m_SDN_TerritoryMembers.SetPermission(targetGUID, SDN_TerritoryPerm.MODERATOR); // MODERATOR
		SDN_UpdateDatabase();
		SDN_SyncTerritory();
	}

	void SDN_DemoteModerator(string targetGUID)
	{
		if (targetGUID == m_SDN_TerritoryOwner || targetGUID == "") return;
		m_SDN_TerritoryMembers.SetPermission(targetGUID, SDN_TerritoryPerm.DEFAULTMEMBER); // NORMAL MEMBER
		SDN_UpdateDatabase();
		SDN_SyncTerritory();
	}

	// ==========================================
	// SISTEMA DE ÍNDICE DO MENU SCROLL (UI-FREE)
	// ==========================================
	ref map<string, int> m_SDN_PlayerKickIndices = new map<string, int>();

	int SDN_GetPlayerKickIndex(string playerGUID)
	{
		if (m_SDN_PlayerKickIndices && m_SDN_PlayerKickIndices.Contains(playerGUID)) 
		{
			return m_SDN_PlayerKickIndices.Get(playerGUID);
		}
		return 0;
	}

	void SDN_CyclePlayerKickIndex(string playerGUID)
	{
		int idx = SDN_GetPlayerKickIndex(playerGUID);
		idx++;
		if (idx >= SDN_GetMemberCount()) idx = 0; 
		m_SDN_PlayerKickIndices.Set(playerGUID, idx);
	}
	
	string SDN_GetTargetMemberName(string playerGUID)
	{
		int idx = SDN_GetPlayerKickIndex(playerGUID);
		TStringArray members = SDN_TerritoryMembersList();
		if (members && idx < members.Count()) return m_SDN_TerritoryMembers.GetName(members.Get(idx));
		return "Nenhum";
	}
	
	string SDN_GetTargetMemberGUID(string playerGUID)
	{
		int idx = SDN_GetPlayerKickIndex(playerGUID);
		TStringArray members = SDN_TerritoryMembersList();
		if (members && idx < members.Count()) return members.Get(idx);
		return "";
	}

	// ==========================================
	// GESTÃO DA BASE DE DADOS (JSON)
	// ==========================================
	void SDN_UpdateDatabase()
	{
		if (GetGame().IsServer() && SDN_TerritoryDatabase.Get())
		{
			SDN_TerritoryData tData = SDN_TerritoryDatabase.Get().GetTerritory(SDN_GetTerritoryID());
			if (!tData)
			{
				tData = new SDN_TerritoryData();
				tData.TerritoryID = SDN_GetTerritoryID();
			}
			
			tData.TerritoryName = m_SDN_TerritoryName;
			tData.Position = GetPosition();
			tData.OwnerGUID = m_SDN_TerritoryOwner;
			
			string oName = SDN_GetPlayerNameOnline(m_SDN_TerritoryOwner);
			if (oName != "") 
			{
				tData.OwnerName = oName;
				m_SDN_TerritoryOwnerName = oName;
			}
			else if (m_SDN_TerritoryOwnerName != "")
			{
				tData.OwnerName = m_SDN_TerritoryOwnerName;
			}
			else
			{
				m_SDN_TerritoryOwnerName = tData.OwnerName;
			}

			tData.IsOpenForInvites = m_SDN_CanAddMember;

			array<string> memberIds = m_SDN_TerritoryMembers.GetMemberArray();
			array<ref SDN_TerritoryMember> newList = new array<ref SDN_TerritoryMember>();
			
			foreach (string guid : memberIds)
			{
				string mName = SDN_GetPlayerNameOnline(guid); 
				if (mName == "") mName = m_SDN_TerritoryMembers.GetName(guid); 
				
				if (mName == "Desconhecido" || mName == "")
				{
					if (tData.HasMember(guid)) 
					{
						foreach (SDN_TerritoryMember oldM : tData.Members)
						{
							if (oldM.PlayerGUID == guid) mName = oldM.PlayerName;
						}
					}
				}
				
				if (mName == "") mName = "Sobrevivente_" + guid.Substring(0, 4); 
				
				int perm = m_SDN_TerritoryMembers.GetPermission(guid);
				newList.Insert(new SDN_TerritoryMember(guid, mName, perm));
				
				if (m_SDN_TerritoryMembers.GetName(guid) == "Desconhecido") m_SDN_TerritoryMembers.m_MemberNames.Set(guid, mName);
			}

			tData.Members = newList;
			SDN_TerritoryDatabase.Get().AddOrUpdateTerritory(tData);
		}
	}

	string SDN_GetPlayerNameOnline(string guid)
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach(Man p : players)
		{
			PlayerBase pb = PlayerBase.Cast(p);
			if (pb && pb.GetIdentity() && pb.GetIdentity().GetPlainId() == guid) return pb.GetIdentity().GetName();
		}
		return "";
	}

	void SDN_LoadFromDatabase()
	{
		if (GetGame().IsServer() && SDN_TerritoryDatabase.Get())
		{
			SDN_TerritoryData tData = SDN_TerritoryDatabase.Get().GetTerritory(SDN_GetTerritoryID());
			if (tData)
			{
				m_SDN_TerritoryName = tData.TerritoryName; 
				m_SDN_TerritoryOwner = tData.OwnerGUID;
				m_SDN_TerritoryOwnerName = tData.OwnerName;
				m_SDN_CanAddMember = tData.IsOpenForInvites;
				
				m_SDN_TerritoryMembers = new SDN_TerritoryMembers();
				foreach (SDN_TerritoryMember member : tData.Members)
				{
					int loadPerm = member.PermissionLevel;
					if (loadPerm <= 0) loadPerm = SDN_TerritoryPerm.DEFAULTMEMBER; 
					m_SDN_TerritoryMembers.AddMember(member.PlayerGUID, member.PlayerName, loadPerm);
				}
			}
			else
			{
				SDN_UpdateDatabase();
			}
			SDN_SyncTerritory();
		}
	}

	TStringArray SDN_TerritoryMembersList()
	{
		return TStringArray.Cast(m_SDN_TerritoryMembers.GetMemberArray());
	}

	void SDN_DoFirstSync()
	{
		SDN_SyncTerritory();
	}

	bool SDN_IsTerritoryOwner(string guid)
	{
		if (!m_SDN_TerritoryOwner || m_SDN_TerritoryOwner == "") return false;
		return (m_SDN_TerritoryOwner == guid);
	}
	
	bool SDN_CanReceiveNewOwner() 
	{ 
		// If abandoned (timestamp > 0), no one can claim it
		if (SDN_TerritoryDatabase.Get())
		{
			SDN_TerritoryData tData = SDN_TerritoryDatabase.Get().GetTerritory(SDN_GetTerritoryID());
			if (tData && tData.AbandonedTimestamp > 0) return false;
		}

		return (!m_SDN_TerritoryOwner || m_SDN_TerritoryOwner == ""); 
	}
	
	string SDN_GetTerritoryOwner() { return m_SDN_TerritoryOwner; }
	
	string SDN_GetTerritoryOwnerName() { return m_SDN_TerritoryOwnerName; }

	string SDN_GetMemberName(string guid)
	{
		if (m_SDN_TerritoryMembers) return m_SDN_TerritoryMembers.GetName(guid);
		return "Desconhecido";
	}
	
	int SDN_GetMemberPermission(string guid)
	{
		if (m_SDN_TerritoryMembers) return m_SDN_TerritoryMembers.GetPermission(guid);
		return 0;
	}
	
	int SDN_GetMemberCount() 
	{ 
		TStringArray members = SDN_TerritoryMembersList();
		if (members)
		{
			return members.Count();
		}
		return 0; 
	}
	
	bool SDN_IsBaseFull()
	{
		int maxMembers = SDN_TerritoryConfig.Get().MaxMembersPerTerritory;
		if (maxMembers <= 0) return false;
		
		// The member count list does NOT include the owner natively in m_SDN_TerritoryMembers
		// If config says 3 members, it means Owner + 2 invited members.
		return ((SDN_GetMemberCount() + 1) >= maxMembers);
	}

	bool SDN_CanAddMember() { return m_SDN_CanAddMember; }
	bool SDN_IsTerritoryMember(string guid) { return m_SDN_TerritoryMembers.CheckId(guid); }
	
	// FUNÇÕES PÚBLICAS DO RADAR
	bool SDN_IsRequestingSync() { return m_SDN_IsRequestingSync; }
	bool SDN_HasSyncedOnce() { return m_SDN_HasSyncedOnce; }

	void SDN_ResetMembers()
	{
		m_SDN_TerritoryMembers = new SDN_TerritoryMembers(); 
		SDN_SyncTerritory();
		SDN_UpdateDatabase(); 
	}

	void SDN_AbandonTerritory()
	{
		// Kick all members and reset owner
		m_SDN_TerritoryMembers = new SDN_TerritoryMembers();
		m_SDN_TerritoryOwner = "";
		m_SDN_TerritoryOwnerName = "";
		m_SDN_TerritoryName = "";
		
		// Block claiming (cannot add members)
		m_SDN_CanAddMember = false;

		// Set AbandonedTimestamp in database
		if (SDN_TerritoryDatabase.Get())
		{
			SDN_TerritoryData tData = SDN_TerritoryDatabase.Get().GetTerritory(SDN_GetTerritoryID());
			if (tData)
			{
				tData.AbandonedTimestamp = SDN_TerritoryDatabase.Get().SDN_GetUnixTimestamp();
				SDN_TerritoryDatabase.Get().MarkDirty();
				SDN_TerritoryDatabase.Get().SaveIfDirty();
			}
		}

		// Look for codelocks in radius and delete them
		float radius = SDN_TerritoryConfig.Get().TerritoryRadius;
		array<Object> territoryObjs = new array<Object>;
		array<CargoBase> proxyCargos = new array<CargoBase>;
		GetGame().GetObjectsAtPosition(GetPosition(), radius, territoryObjs, proxyCargos);
		
		foreach (Object tObj : territoryObjs)
		{
			if (tObj.IsInherited(BaseBuildingBase) || tObj.IsInherited(TentBase) || tObj.IsInherited(ItemBase))
			{
				EntityAI baseObj = EntityAI.Cast(tObj);
				if (baseObj)
				{
					EntityAI attachment = baseObj.GetAttachmentByType(CombinationLock);
					if (attachment)
					{
						GetGame().ObjectDelete(attachment);
					}
					
					// Avoid changing array size while iterating, but we only have a few code locks normally.
					// A safer way is to store them and delete after.
					array<EntityAI> locksToDelete = new array<EntityAI>;
					for (int i = 0; i < baseObj.GetInventory().AttachmentCount(); i++)
					{
						EntityAI att = baseObj.GetInventory().GetAttachmentFromIndex(i);
						if (att)
						{
							string attType = att.GetType();
							attType.ToLower();
							if (attType.Contains("codelock"))
							{
								locksToDelete.Insert(att);
							}
						}
					}
					
					foreach(EntityAI lock : locksToDelete)
					{
						GetGame().ObjectDelete(lock);
					}
				}
			}
		}
		
		SDN_SyncTerritory();
		SDN_UpdateDatabase();
	}
	
	void SDN_SetTerritoryOwner(string guid, string name = "")
	{
		m_SDN_TerritoryOwner = guid;
		if (name != "") m_SDN_TerritoryOwnerName = name;
		SDN_SyncTerritory();
		SDN_UpdateDatabase(); 
	}
	
	void SDN_AllowMemberToBeAdded(bool state = true)
	{
		m_SDN_CanAddMember = state;
		if (m_SDN_AwaitingReset && GetGame().IsServer())
		{
			m_SDN_AwaitingReset = false;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.SDN_ResetAllowMemberToBeAdded);
		}
		if (state && GetGame().IsServer())
		{
			m_SDN_AwaitingReset = true;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SDN_ResetAllowMemberToBeAdded, 60 * 1000);
		}
		SetSynchDirty();
		SDN_UpdateDatabase(); 
	}
	
	void SDN_ResetAllowMemberToBeAdded()
	{
		m_SDN_AwaitingReset = false;
		m_SDN_CanAddMember = false;
		SetSynchDirty();
		SDN_UpdateDatabase(); 
	}
	
	void SDN_AddMember(string guid, string name = "Sobrevivente")
	{
		if (guid != m_SDN_TerritoryOwner)
		{
			m_SDN_TerritoryMembers.AddMember(guid, name);
			SDN_AllowMemberToBeAdded(false);
			SDN_SyncTerritory();
			SDN_UpdateDatabase(); 
		}
	}
	
	void SDN_RemoveMember(string guid)
	{
		if (guid != m_SDN_TerritoryOwner)
		{
			m_SDN_TerritoryMembers.RemoveMember(guid);
			SDN_SyncTerritory();
			SDN_UpdateDatabase(); 
		}
	}

	void SDN_ToggleModerator(string targetGUID)
	{
		if (targetGUID == m_SDN_TerritoryOwner || targetGUID == "") return;
		int currentPerm = m_SDN_TerritoryMembers.GetPermission(targetGUID);
		
		if (currentPerm & SDN_TerritoryPerm.REMOVEMEMBER) m_SDN_TerritoryMembers.SetPermission(targetGUID, SDN_TerritoryPerm.DEFAULTMEMBER);
		else m_SDN_TerritoryMembers.SetPermission(targetGUID, SDN_TerritoryPerm.MODERATOR);

		SDN_UpdateDatabase();
		SDN_SyncTerritory();
	}

	void SDN_AddMemberClient(string guid)
	{
		if (SDN_CanAddMember())
		{
			Param2<string, SDN_TerritoryMembers> paramAdd = new Param2<string, SDN_TerritoryMembers>(guid, m_SDN_TerritoryMembers);
			RPCSingleParam(SDN_TerritoryRPCs.SDN_ADD_MEMBER, paramAdd, true, NULL);
		}
	}
	
	override void OnStoreSave(ParamsWriteContext ctx) { super.OnStoreSave(ctx); }
	
	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version)) return false;
		return true;
	}
	
	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (GetGame().IsServer())
		{
			if (m_SDN_AllFlags && m_SDN_AllFlags.Find(this) == -1) m_SDN_AllFlags.Insert(this);
			SDN_LoadFromDatabase();
			
			// Deletion check
			if (SDN_TerritoryDatabase.Get() && SDN_TerritoryConfig.Get())
			{
				SDN_TerritoryData tData = SDN_TerritoryDatabase.Get().GetTerritory(SDN_GetTerritoryID());
				if (tData && tData.AbandonedTimestamp > 0)
				{
					int currentTimestamp = SDN_TerritoryDatabase.Get().SDN_GetUnixTimestamp();
					int deleteCooldownSeconds = SDN_TerritoryConfig.Get().DeleteAbandonedBaseCooldownMinutes * 60; 
					
					if (currentTimestamp - tData.AbandonedTimestamp >= deleteCooldownSeconds)
					{
						// Defer deletion so all objects have time to load into the world
						GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SDN_ExecuteDeletion, 10000, false);
					}
				}
			}
		}
	}

	void SDN_ExecuteDeletion()
	{
		float radius = SDN_TerritoryConfig.Get().TerritoryRadius;
		array<Object> territoryObjs = new array<Object>;
		array<CargoBase> proxyCargos = new array<CargoBase>;
		GetGame().GetObjectsAtPosition(GetPosition(), radius, territoryObjs, proxyCargos);
		
		foreach (Object tObj : territoryObjs)
		{
			if (tObj.IsInherited(BaseBuildingBase) || tObj.IsInherited(TentBase) || tObj.IsInherited(ItemBase))
			{
				if (tObj != this)
				{
					GetGame().ObjectDelete(tObj);
				}
			}
		}
		
		// Finally, delete the flag itself
		GetGame().ObjectDelete(this);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (GetGame().IsServer() && SDN_TerritoryDatabase.Get())
		{
			SDN_TerritoryDatabase.Get().RemoveTerritory(SDN_GetTerritoryID());
		}
	}

	void SDN_SyncTerritoryRateLimited()
	{
		if (GetGame().IsServer()) return;
		int curTime = GetGame().GetTime();
		if (m_SDN_LastSyncTime < curTime)
		{
			m_SDN_LastSyncTime = curTime + 100000;
			SDN_SyncTerritory();
		}
	}
	
	void SDN_SyncTerritory(PlayerIdentity identity = NULL)
	{
		if (GetGame().IsServer()) 
		{
			SetSynchDirty();
			
			if (m_SDN_TerritoryMembers)
			{
				m_SDN_TerritoryMembers.m_OnlinePlayers.Clear();
				array<Man> players = new array<Man>;
				GetGame().GetPlayers(players);
				foreach(Man p : players)
				{
					PlayerBase pb = PlayerBase.Cast(p);
					if (pb && pb.GetIdentity() && pb.GetIdentity().GetPlainId())
					{
						m_SDN_TerritoryMembers.m_OnlinePlayers.Insert(pb.GetIdentity().GetPlainId());
					}
				}
				m_SDN_TerritoryMembers.m_OwnerName = m_SDN_TerritoryOwnerName;
			}

			Param3<string, string, SDN_TerritoryMembers> data = new Param3<string, string, SDN_TerritoryMembers>(m_SDN_TerritoryOwner, m_SDN_TerritoryName, m_SDN_TerritoryMembers);
			RPCSingleParam(SDN_TerritoryRPCs.SDN_SEND_DATA, data, true, identity);
		} 
		else if (GetGame().IsClient())
		{
			m_SDN_IsRequestingSync = true;
			Param1<bool> reqData = new Param1<bool>(true);
			RPCSingleParam(SDN_TerritoryRPCs.SDN_REQUEST_DATA, reqData, true, NULL);
		}
	}
	
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		
		if (rpc_type == SDN_TerritoryRPCs.SDN_SEND_DATA && GetGame().IsClient()) 
		{
			Param3<string, string, SDN_TerritoryMembers> data3;
			if (ctx.Read(data3)) 
			{
				m_SDN_TerritoryOwner = data3.param1;
				m_SDN_TerritoryName = data3.param2; 
				m_SDN_TerritoryMembers = SDN_TerritoryMembers.Cast(data3.param3);
				
				if (m_SDN_TerritoryMembers)
				{
					m_SDN_TerritoryOwnerName = m_SDN_TerritoryMembers.m_OwnerName;
					m_SDN_OnlineMembersCache = m_SDN_TerritoryMembers.m_OnlinePlayers;
				}
				
				// CORREÇÃO: Marca que o pacote finalmente chegou!
				m_SDN_IsRequestingSync = false;
				m_SDN_HasSyncedOnce = true; 
				
				// Menu logic is in 5_Mission which cannot be referenced here in 4_World
				// We can simply close and reopen the menu if it's the right ID
				if (GetGame().GetUIManager() && GetGame().GetUIManager().GetMenu())
				{
					if (GetGame().GetUIManager().GetMenu().GetID() == 748392) // 748392 is SDN_TerritoryMenu id
					{
						GetGame().GetUIManager().GetMenu().Close();
						GetGame().GetUIManager().EnterScriptedMenu(748392, NULL);
					}
				}
			}
			return;
		}

		if (rpc_type == SDN_TerritoryRPCs.SDN_REQUEST_DATA && GetGame().IsServer()) 
		{
			if (sender) SDN_SyncTerritory(sender);
			else SDN_SyncTerritory();
			return;
		}

		if (rpc_type == SDN_TerritoryRPCs.SDN_ADD_MEMBER && GetGame().IsServer())
		{
			Param2<string, SDN_TerritoryMembers> data2;
			if (ctx.Read(data2))
			{
				if (SDN_CanAddMember() && sender)
				{
					if (SDN_TerritoryMembersList().Count() == data2.param2.m_Members.Count() && sender.GetPlainId() == data2.param1) 
					{ 
						SDN_AddMember(data2.param1, sender.GetName());
						SDN_AllowMemberToBeAdded(false);
					}
					SDN_SyncTerritory();
				}
			}
			return;
		}
	}
		
	bool SDN_HasRaisedFlag()
	{
		if (FindAttachmentBySlotName("Material_FPole_Flag"))
		{
			float state = GetAnimationPhase("flag_mast");
			// CORREÇÃO: Removido o sinal de igual (=)
			if (state < 1.0) return true;
		}
		return false;
	}
		
	bool SDN_CheckPlayerPermission(string guid, int permission)
	{
		int publicPerms = 0; // Se você planeja dar acesso público no futuro (ex: beber de um poço), pode colocar as flags aqui.
		int PermsCheck = publicPerms & permission;
		if (PermsCheck == permission) return true;
		
		// Admin Bypass
		if (SDN_TerritoryConfig.Get().ServerAdmins.Find(guid) != -1) return true;
		
		// Owner Bypass - Except if flag is down, we want to block building/deploying
		if (SDN_IsTerritoryOwner(guid))
		{
			if (!SDN_HasRaisedFlag() && !SDN_CanReceiveNewOwner())
			{
				if (permission == SDN_TerritoryPerm.LOWERFLAG || permission == SDN_TerritoryPerm.ADDMEMBER || permission == SDN_TerritoryPerm.REMOVEMEMBER) return true;
				return false;
			}
			return true;
		}
		
		// A Bandeira não está erguida (Ninguém é dono oficial ou não terminou de montar)
		// Portanto, dependendo de como a permissão deve reagir, para construção não é "público".
		// Na lógica anterior retornava 'true' sempre se a bandeira estivesse baixada, o que permitia
		// a inimigos continuarem construindo antes de o dono erguer a bandeira!
		if (!SDN_HasRaisedFlag())
		{
			// Se a bandeira for recém colocada e não tiver dono, qualquer um pode construir o mastro.
			if (SDN_CanReceiveNewOwner()) return true;

		// NOVO COMPORTAMENTO: Se a bandeira não está içada, NINGUÉM pode interagir 
		// (construir, colocar codelock, mexer no armazenamento), para forçar segurança e evitar exploits.
		// O dono precisaria levantar a bandeira primeiro para que o território volte a estar "ativo" para essas coisas.
		// Apenas a permissão LOWERFLAG e ADDMEMBER/REMOVEMEMBER deve funcionar para permitir gerenciar e levantar.
		if (permission == SDN_TerritoryPerm.LOWERFLAG || permission == SDN_TerritoryPerm.ADDMEMBER || permission == SDN_TerritoryPerm.REMOVEMEMBER)
		{
			if (m_SDN_TerritoryMembers) return m_SDN_TerritoryMembers.Check(guid, permission);
		}
		
			return false;
		}

		// Checa na base de dados de membros se este GUID possui a flag específica ativada.
		if (m_SDN_TerritoryMembers)
		{
			return m_SDN_TerritoryMembers.Check(guid, permission); 
		}

		return false;
	}
	
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionSDN_OpenTerritoryMenu);
		AddAction(ActionSDN_AcceptMembership);
		
		// Removidas as actions antigas (NameTerritory, Promote, Kick, Reset)
		// pois a nova Interface de Usuário lida com isso.
		// Mantido AcceptMembership (Opção A aprovada pelo usuário: tecla F rápida).
	}

	static bool SDN_IsInsideTerritory(vector Pos)
	{
		if (Pos == vector.Zero) return false;
		
		float radiusSq = SDN_TerritoryConfig.Get().TerritoryRadius * SDN_TerritoryConfig.Get().TerritoryRadius;
		
		if (m_SDN_AllFlags)
		{
			foreach (TerritoryFlag theFlag : m_SDN_AllFlags)
			{
				if (!theFlag) continue;
				if (vector.DistanceSq(theFlag.GetPosition(), Pos) <= radiusSq)
				{
					return true;
				}
			}
		}
		return false;
	}

	static int m_SDN_CachedPermType = 0;
	static string m_SDN_CachedGUID = "";

	static bool SDN_HasTerritoryPermAtPos(string GUID, int Perm, vector Pos, bool CheckTerritoryOverlap = false)
	{
		if (GUID == "") return false;
		int curTime = GetGame().GetTime();
		
		// O cache armazena a permissão validada E o GUID, evitando que jogadores diferentes partilhem resultados falsos negativos
		if (vector.DistanceSq(m_SDN_LastCheckLocation, Pos) <= 0.0025 && m_SDN_LastCheckLocationNextTime > curTime && m_SDN_CachedPermType == Perm && m_SDN_CachedGUID == GUID) 
		{
			return m_SDN_CachedHasTerritoryPerm;
		}

		m_SDN_LastCheckLocation = Pos;
		m_SDN_LastCheckLocationNextTime = curTime + 500; // 500ms é suficiente para segurar o FPS sem trancar a permissão
		m_SDN_CachedPermType = Perm;
		m_SDN_CachedGUID = GUID;
		
		if (Pos == vector.Zero)
		{
			m_SDN_CachedHasTerritoryPerm = false;
			return m_SDN_CachedHasTerritoryPerm;
		} 
		else 
		{
			float theRadius = SDN_TerritoryConfig.Get().TerritoryRadius;
			if (CheckTerritoryOverlap) theRadius = theRadius * 2;
			float radiusSq = theRadius * theRadius;

			if (m_SDN_AllFlags)
			{
				foreach (TerritoryFlag theFlag : m_SDN_AllFlags)
				{
					if (!theFlag) continue;
					if (vector.DistanceSq(theFlag.GetPosition(), Pos) <= radiusSq)
					{
						theFlag.SDN_SyncTerritoryRateLimited();
						m_SDN_CachedHasTerritoryPerm = theFlag.SDN_CheckPlayerPermission(GUID, Perm) && !CheckTerritoryOverlap;
						return m_SDN_CachedHasTerritoryPerm;
					}
				}
			}
		}

		if (!SDN_TerritoryConfig.Get().RequireTerritory) m_SDN_CachedHasTerritoryPerm = true;
		else m_SDN_CachedHasTerritoryPerm = false;
		return m_SDN_CachedHasTerritoryPerm;
	}

	// ==========================================
	// SISTEMA DE LIMITAÇÃO DE PEÇAS DE BASE
	// ==========================================
	int SDN_GetBuiltPartsCount()
	{
		int count = 0;
		float radius = SDN_TerritoryConfig.Get().TerritoryRadius;
		
		array<Object> objects_in_radius = new array<Object>;
		// Faz um scan rápido em redor da bandeira
		GetGame().GetObjectsAtPosition(GetPosition(), radius, objects_in_radius, null);

		TStringArray limitsList = SDN_TerritoryConfig.Get().BaseBuildParts;
		if (!limitsList || limitsList.Count() == 0) return 0;

		foreach (Object obj : objects_in_radius)
		{
			if (!obj || obj == this) continue; // Ignora nulos e a própria bandeira
			
			// OTIMIZAÇÃO EXTREMA: Só perde tempo a ler nomes se o objeto for uma construção (ignora árvores, armas, etc)
			if (!obj.IsInherited(BaseBuildingBase)) continue;

			string objType = obj.GetType();
			objType.ToLower();

			foreach (string partClass : limitsList)
			{
				string checkClass = partClass;
				checkClass.ToLower();

				// O 'Contains' permite apanhar itens exatos ("fence") ou grupos de mods ("bbp_")
				if (objType.Contains(checkClass))
				{
					count++;
					break; // Peça contada! Salta para o próximo objeto para não contar a dobrar
				}
			}
		}
		return count;
	}

	bool SDN_IsBuildLimitReached()
	{
		int maxParts = SDN_TerritoryConfig.Get().BaseBuildPartsMax;
		if (maxParts <= 0) return false; // Se o valor no JSON for 0, o Admin desligou a limitação
		
		return (SDN_GetBuiltPartsCount() >= maxParts);
	}

	int SDN_GetStorageCount()
	{
		int currentStorage = 0;
		TStringArray storageList = SDN_TerritoryConfig.Get().BaseStorageParts;
		float radius = SDN_TerritoryConfig.Get().TerritoryRadius;

		array<Object> objectsInRadius = new array<Object>;
		GetGame().GetObjectsAtPosition(this.GetPosition(), radius, objectsInRadius, null);

		foreach (Object obj : objectsInRadius)
		{
			if (!obj || obj == this) continue;
			
			ItemBase itemObj = ItemBase.Cast(obj);
			if (!itemObj) continue;
			
			if (itemObj.GetHierarchyRootPlayer()) continue;
			if (itemObj.IsHologram()) continue;

			string objType = itemObj.GetType(); 
			objType.ToLower();

			foreach (string storageClass : storageList)
			{
				string checkClass = storageClass; 
				checkClass.ToLower();

				if (objType.Contains(checkClass))
				{
					if (!itemObj.SDN_IsLegallyPlaced()) break;
					
					currentStorage++;
					break;
				}
			}
		}
		return currentStorage;
	}

	// Nova Função: Conta se o limite de armazenamentos foi atingido
	bool SDN_IsStorageLimitReached()
	{
		int maxStorage = SDN_TerritoryConfig.Get().BaseStoragePartsMax;
		if (maxStorage <= 0) return false;
		return (SDN_GetStorageCount() >= maxStorage);
	}

	int SDN_GetCodeLockCount()
	{
		int currentLocks = 0;
		float radius = SDN_TerritoryConfig.Get().TerritoryRadius;
		array<Object> territoryObjs = new array<Object>;
		GetGame().GetObjectsAtPosition(this.GetPosition(), radius, territoryObjs, null);

		foreach (Object tObj : territoryObjs)
		{
			if (tObj.IsInherited(BaseBuildingBase) || tObj.IsInherited(TentBase) || tObj.IsInherited(ItemBase))
			{
				EntityAI baseObj = EntityAI.Cast(tObj);
				if (baseObj)
				{
					bool hasLock = false;

					if (baseObj.GetAttachmentByType(CombinationLock))
					{
						currentLocks++;
					}

					for (int i = 0; i < baseObj.GetInventory().AttachmentCount(); i++)
					{
						EntityAI attachment = baseObj.GetInventory().GetAttachmentFromIndex(i);
						if (attachment)
						{
							string attType = attachment.GetType();
							attType.ToLower();
							if (attType.Contains("codelock"))
							{
								currentLocks++;
							}
						}
					}
				}
			}
		}
		return currentLocks;
	}
}