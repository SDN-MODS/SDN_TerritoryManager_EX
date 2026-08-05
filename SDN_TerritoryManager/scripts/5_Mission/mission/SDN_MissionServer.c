/*
  Ficheiro: SDN_MissionServer.c
  Caminho: SDN_MODS/SDN_TerritoryManager/scripts/5_Mission/mission/SDN_MissionServer.c
*/

modded class MissionServer extends MissionBase
{
	int m_SDN_LastAbandonedAlertIdx = 0;

	void MissionServer()
	{
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_ShowToast", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_ReqShowToast", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_RPCModSettings", this, SingeplayerExecutionType.Server);
		
		// Registro dos RPCs da GUI
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_RPC_RenameTerritory", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_RPC_ToggleInvites", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_RPC_PromoteMember", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_RPC_DemoteMember", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_RPC_KickMember", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_RPC_LeaveTerritory", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_RPC_TransferTerritory", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("SDN_TerritoryManager", "SDN_RPC_AdminReset", this, SingeplayerExecutionType.Server);
	}

	override void OnInit()
	{
		super.OnInit();
		// Start global alert timer
		if (SDN_TerritoryConfig.Get())
		{
			int interval = SDN_TerritoryConfig.Get().AlertAbandonedBaseIntervalMinutes * 60 * 1000;
			if (interval > 0)
			{
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SDN_AlertAbandonedBases, interval, true);
			}
		}
	}

	void SDN_AlertAbandonedBases()
	{
		if (!TerritoryFlag.m_SDN_AllFlags || TerritoryFlag.m_SDN_AllFlags.Count() == 0) return;
		if (!SDN_TerritoryDatabase.Get()) return;
		
		array<TerritoryFlag> abandonedFlags = new array<TerritoryFlag>();
		foreach (TerritoryFlag flag : TerritoryFlag.m_SDN_AllFlags)
		{
			if (!flag) continue;
			SDN_TerritoryData tData = SDN_TerritoryDatabase.Get().GetTerritory(flag.SDN_GetTerritoryID());
			if (tData && tData.AbandonedTimestamp > 0)
			{
				abandonedFlags.Insert(flag);
			}
		}

		if (abandonedFlags.Count() == 0) 
		{
			m_SDN_LastAbandonedAlertIdx = 0;
			return;
		}

		if (m_SDN_LastAbandonedAlertIdx >= abandonedFlags.Count())
		{
			m_SDN_LastAbandonedAlertIdx = 0;
		}

		TerritoryFlag alertFlag = abandonedFlags.Get(m_SDN_LastAbandonedAlertIdx);
		if (alertFlag)
		{
			vector pos = alertFlag.GetPosition();
			string msg = SDN_TerritoryConfig.Get().MessageSettings.ChatOnly.BaseAbandonedGlobalAlert;
			msg.Replace("%1", pos[0].ToString());
			msg.Replace("%2", pos[2].ToString());

			// Broadcast to all players
			array<Man> players = new array<Man>;
			GetGame().GetPlayers(players);
			foreach (Man player : players)
			{
				PlayerBase pb = PlayerBase.Cast(player);
				if (pb)
				{
					SDN_MessageManager.SendMemberEvent(pb, "BaseAbandonedGlobalAlert", msg);
				}
			}
		}

		m_SDN_LastAbandonedAlertIdx++;
	}


	// ==========================================
	// LÓGICA DE BACKEND DA INTERFACE (GUI)
	// ==========================================
	
	bool SDN_CheckRPCDistance(PlayerBase player, TerritoryFlag flag)
	{
		if (!player || !flag) return false;
		if (vector.DistanceSq(player.GetPosition(), flag.GetPosition()) > 25.0) return false; // Max 5 meters (squared)
		return true;
	}

	void SDN_RPC_RenameTerritory(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param2<TerritoryFlag, string> data;
			if (!ctx.Read(data)) return;
			
			TerritoryFlag flag = data.param1;
			string newName = data.param2;
			string guid = sender.GetPlainId();
			PlayerBase pb = PlayerBase.Cast(sender.GetPlayer());
			
			if (!SDN_CheckRPCDistance(pb, flag)) return;
			
			bool isAdmin = false;
			if (SDN_TerritoryConfig.Get().ServerAdmins)
			{
				isAdmin = SDN_TerritoryConfig.Get().ServerAdmins.Find(guid) != -1;
			}
			if (flag && (flag.SDN_IsTerritoryOwner(guid) || isAdmin || flag.SDN_CanReceiveNewOwner()))
			{
				if (flag.SDN_CanReceiveNewOwner())
				{
				    // Validações antes de reivindicar (migrado da action obsoleta)
						if (!flag.FindAttachmentBySlotName("Material_FPole_Flag"))
					{
						return; // Bandeira ou mastro incompletos, não pode dar claim
					}

				    if (!isAdmin && SDN_TerritoryDatabase.Get())
				    {
				        int cooldown = SDN_TerritoryDatabase.Get().GetCooldownRemaining(guid);
				        if (cooldown > 0)
				        {
				            if (pb)
				            {
				                int minutesLeft = cooldown / 60;
				                string msg = SDN_TerritoryConfig.Get().MessageSettings.Alerts.PunishmentTimeLeft;
				                msg.Replace("%1", SDN_MessageManager.SDN_FormatMinutesToText(minutesLeft));
				                SDN_MessageManager.SendAlert(pb, "TerritoryInteractError", msg, true);
				            }
				            return;
				        }
				        
				        int maxBases = SDN_TerritoryConfig.Get().MaxTerritoriesPerPlayer;
				        int activeBases = SDN_TerritoryDatabase.Get().GetActiveTerritoryCount(guid);
				        if (activeBases >= maxBases)
				        {
				            if (pb)
				            {
				                string msgMax = SDN_TerritoryConfig.Get().MessageSettings.Alerts.MaxBasesReached;
				                msgMax.Replace("%1", maxBases.ToString());
				                SDN_MessageManager.SendAlert(pb, "TerritoryInteractError", msgMax, true);
				            }
				            return;
				        }
				    }
				}

				newName.TrimInPlace(); 
				
				if (newName.Length() < 3 || newName.Length() > 20)
				{
					string msgLen = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdCreateLengthError;
					if (pb) SDN_MessageManager.SendMemberEvent(pb, "TerritoryCmdCreateLengthError", msgLen);
					return;
				}
				
				if (!SDN_IsAlphanumeric(newName))
				{
					string msgAlp = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdCreateAlphaError;
					if (pb) SDN_MessageManager.SendMemberEvent(pb, "TerritoryCmdCreateAlphaError", msgAlp);
					return;
				}
				
				// Apenas conceder a propriedade e resetar os membros DEPOIS de validar o nome
				if (flag.SDN_CanReceiveNewOwner())
				{
					SDN_Logger.LogAdmin("O Administrador " + sender.GetName() + " (" + guid + ") resetou (deletou os dados de) a base " + flag.SDN_GetTerritoryName() + ". [ID: " + flag.SDN_GetTerritoryID() + " | Loc: " + flag.GetPosition().ToString() + "]");
					flag.SDN_ResetMembers();
					flag.SDN_SetTerritoryOwner(guid);
				}
				
				flag.SDN_SetTerritoryName(newName);
					SDN_Logger.LogInfo("O jogador " + sender.GetName() + " (" + guid + ") registrou a base: " + newName + " [ID: " + flag.SDN_GetTerritoryID() + " | Loc: " + flag.GetPosition().ToString() + "]");
				if (pb) {
					string msgSuc = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdCreateSuccess;
					msgSuc.Replace("%1", newName);
					SDN_MessageManager.SendMemberEvent(pb, "TerritoryCmdCreateSuccess", msgSuc);
				}
			}
		}
	}

	void SDN_RPC_ToggleInvites(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param1<TerritoryFlag> data;
			if (!ctx.Read(data)) return;
			
			TerritoryFlag flag = data.param1;
			string guid = sender.GetPlainId();
			PlayerBase pb = PlayerBase.Cast(sender.GetPlayer());
			
			if (!SDN_CheckRPCDistance(pb, flag)) return;
			
				bool isAdmin = false;
			if (SDN_TerritoryConfig.Get().ServerAdmins) isAdmin = SDN_TerritoryConfig.Get().ServerAdmins.Find(guid) != -1;
			if (flag && (flag.SDN_IsTerritoryOwner(guid) || flag.SDN_CheckPlayerPermission(guid, SDN_TerritoryPerm.ADDMEMBER) || isAdmin))
			{
					bool isFull = flag.SDN_IsBaseFull();
					if (!flag.SDN_CanAddMember() && isFull)
					{
						if (pb)
						{
							SDN_MessageManager.SendAlert(pb, "BaseFull", "BASE FULL: Maximum member limit reached.", true);
						}
						return;
					}
				bool newState = !flag.SDN_CanAddMember();
				flag.SDN_AllowMemberToBeAdded(newState);
				
				if (pb)
				{
					if (newState) SDN_MessageManager.SendMemberEvent(pb, "ToggleInvites", SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryInteractInviteEnabled);
					else SDN_MessageManager.SendMemberEvent(pb, "ToggleInvites", SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryInteractInviteRevoked);
				}
			}
		}
	}

	void SDN_RPC_PromoteMember(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param2<TerritoryFlag, string> data;
			if (!ctx.Read(data)) return;
			
			TerritoryFlag flag = data.param1;
			string targetGuid = data.param2;
			string guid = sender.GetPlainId();
			PlayerBase pb = PlayerBase.Cast(sender.GetPlayer());
			
			if (!SDN_CheckRPCDistance(pb, flag)) return;
			
			bool isAdmin = false;
			if (SDN_TerritoryConfig.Get().ServerAdmins) isAdmin = SDN_TerritoryConfig.Get().ServerAdmins.Find(guid) != -1;
			if (flag && (flag.SDN_IsTerritoryOwner(guid) || isAdmin))
			{
				if (flag.SDN_IsTerritoryOwner(targetGuid)) return; // Impede dono de ser modificado
				
				int currentPerm = flag.SDN_GetMemberPermission(targetGuid);
				if (currentPerm & SDN_TerritoryPerm.REMOVEMEMBER) return; // Ja e moderador, faz nada

				flag.SDN_PromoteModerator(targetGuid);
					SDN_Logger.LogInfo("O dono " + sender.GetName() + " promoveu o membro " + flag.SDN_GetMemberName(targetGuid) + " (" + targetGuid + ") para moderador na base " + flag.SDN_GetTerritoryName() + ". [ID: " + flag.SDN_GetTerritoryID() + " | Loc: " + flag.GetPosition().ToString() + "]");
				if (pb)
				{
				    string msg = SDN_TerritoryConfig.Get().MessageSettings.ChatOnly.PromotedModerator;
				    string mName = flag.SDN_GetMemberName(targetGuid);
				    msg.Replace("%1", mName);
				    SDN_MessageManager.SendMemberEvent(pb, "PromoteMember", msg);
				}
			}
		}
	}

	void SDN_RPC_DemoteMember(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param2<TerritoryFlag, string> data;
			if (!ctx.Read(data)) return;
			
			TerritoryFlag flag = data.param1;
			string targetGuid = data.param2;
			string guid = sender.GetPlainId();
			PlayerBase pb = PlayerBase.Cast(sender.GetPlayer());
			
			if (!SDN_CheckRPCDistance(pb, flag)) return;
			
			bool isAdmin = false;
			if (SDN_TerritoryConfig.Get().ServerAdmins) isAdmin = SDN_TerritoryConfig.Get().ServerAdmins.Find(guid) != -1;
			if (flag && (flag.SDN_IsTerritoryOwner(guid) || isAdmin))
			{
				if (flag.SDN_IsTerritoryOwner(targetGuid)) return; // Impede dono de ser modificado
				
				int currentPerm = flag.SDN_GetMemberPermission(targetGuid);
				if (!(currentPerm & SDN_TerritoryPerm.REMOVEMEMBER)) return; // Nao e moderador, faz nada

				flag.SDN_DemoteModerator(targetGuid);
					SDN_Logger.LogInfo("O dono " + sender.GetName() + " rebaixou o moderador " + flag.SDN_GetMemberName(targetGuid) + " (" + targetGuid + ") na base " + flag.SDN_GetTerritoryName() + ". [ID: " + flag.SDN_GetTerritoryID() + " | Loc: " + flag.GetPosition().ToString() + "]");
				if (pb)
				{
				    string msg = SDN_TerritoryConfig.Get().MessageSettings.ChatOnly.DemotedMember;
				    string mName = flag.SDN_GetMemberName(targetGuid);
				    msg.Replace("%1", mName);
				    SDN_MessageManager.SendMemberEvent(pb, "PromoteMember", msg);
				}
			}
		}
	}

	void SDN_RPC_KickMember(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param2<TerritoryFlag, string> data;
			if (!ctx.Read(data)) return;
			
			TerritoryFlag flag = data.param1;
			string targetGuid = data.param2;
			string guid = sender.GetPlainId();
			PlayerBase pb = PlayerBase.Cast(sender.GetPlayer());
			
			if (!SDN_CheckRPCDistance(pb, flag)) return;
			
			bool isAdmin = false;
			if (SDN_TerritoryConfig.Get().ServerAdmins) isAdmin = SDN_TerritoryConfig.Get().ServerAdmins.Find(guid) != -1;
			if (flag && (flag.SDN_IsTerritoryOwner(guid) || flag.SDN_CheckPlayerPermission(guid, SDN_TerritoryPerm.REMOVEMEMBER) || isAdmin))
			{
				if (flag.SDN_IsTerritoryOwner(targetGuid)) return; // Impede moderador de expulsar o Dono!
				
				string mName = flag.SDN_GetMemberName(targetGuid);
				flag.SDN_RemoveMember(targetGuid);
					SDN_Logger.LogInfo("O admin/moderador " + sender.GetName() + " (" + guid + ") expulsou o membro " + mName + " (" + targetGuid + ") da base " + flag.SDN_GetTerritoryName() + ". [ID: " + flag.SDN_GetTerritoryID() + " | Loc: " + flag.GetPosition().ToString() + "]");
				
				// Aplicar cooldown ao membro expulso se configurado
				float cooldown = SDN_TerritoryConfig.Get().LeaveTerritoryCooldownMinutes;
				if (cooldown > 0 && SDN_TerritoryDatabase.Get())
				{
				    SDN_TerritoryDatabase.Get().SetCooldown(targetGuid, cooldown);
				}
				
				if (pb)
				{
				    string msg = SDN_TerritoryConfig.Get().MessageSettings.ChatOnly.MemberKicked;
				    msg.Replace("%1", mName);
				    SDN_MessageManager.SendMemberEvent(pb, "KickMember", msg);
				}
			}
		}
	}
	void SDN_RPC_TransferTerritory(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param2<TerritoryFlag, string> data;
			if (!ctx.Read(data)) return;
			
			TerritoryFlag flag = data.param1;
			string oldOwnerGUID = sender.GetPlainId();
			string newOwnerGUID = data.param2;
			PlayerBase pb = PlayerBase.Cast(sender.GetPlayer());
			
			if (!SDN_CheckRPCDistance(pb, flag)) return;
			
			if (flag && flag.SDN_IsTerritoryOwner(oldOwnerGUID) && newOwnerGUID != "")
			{
				string newOwnerName = flag.SDN_GetMemberName(newOwnerGUID);
				if (newOwnerName == "" || newOwnerName == "Desconhecido") newOwnerName = flag.SDN_GetPlayerNameOnline(newOwnerGUID);
				
				flag.SDN_RemoveMember(newOwnerGUID);
				flag.SDN_SetTerritoryOwner(newOwnerGUID, newOwnerName);
				flag.SDN_AddMember(oldOwnerGUID, sender.GetName());
					flag.SDN_PromoteModerator(oldOwnerGUID);
					SDN_Logger.LogAdmin("O dono " + sender.GetName() + " (" + oldOwnerGUID + ") transferiu a posse da base " + flag.SDN_GetTerritoryName() + " para o jogador " + newOwnerName + " (" + newOwnerGUID + "). [ID: " + flag.SDN_GetTerritoryID() + " | Loc: " + flag.GetPosition().ToString() + "]");

				if (pb)
				{
					string msgFrom = SDN_TerritoryConfig.Get().MessageSettings.ChatOnly.BaseTransferredFromYou;
					msgFrom.Replace("%1", sender.GetName());
					msgFrom.Replace("%2", newOwnerName);
					SDN_MessageManager.SendMemberEvent(pb, "TransferTerritory", msgFrom);
				}

				array<Man> players = new array<Man>;
				GetGame().GetPlayers(players);
				PlayerBase newOwnerPB = null;
				foreach (Man player : players)
				{
					if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == newOwnerGUID)
					{
						newOwnerPB = PlayerBase.Cast(player);
						break;
					}
				}

				if (newOwnerPB)
				{
					string msgTo = SDN_TerritoryConfig.Get().MessageSettings.ChatOnly.BaseTransferredToYou;
					msgTo.Replace("%1", flag.SDN_GetTerritoryName());
					SDN_MessageManager.SendMemberEvent(newOwnerPB, "TransferTerritory", msgTo);
				}

				// Envia o pacote completo de volta para que o cliente atualize os dados UI
				flag.SDN_SyncTerritory(sender);
			}
		}
	}


	void SDN_RPC_LeaveTerritory(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param1<TerritoryFlag> data;
			if (!ctx.Read(data)) return;
			
			TerritoryFlag flag = data.param1;
			string guid = sender.GetPlainId();
			PlayerBase pb = PlayerBase.Cast(sender.GetPlayer());
			
			if (!SDN_CheckRPCDistance(pb, flag)) return;
			
			if (flag && (flag.SDN_IsTerritoryMember(guid) || flag.SDN_IsTerritoryOwner(guid)))
			{
				if (flag.SDN_IsTerritoryOwner(guid))
				{
					flag.SDN_AbandonTerritory();
						SDN_Logger.LogInfo("O dono " + sender.GetName() + " (" + guid + ") abandonou a base " + flag.SDN_GetTerritoryName() + ". [ID: " + flag.SDN_GetTerritoryID() + " | Loc: " + flag.GetPosition().ToString() + "]");
				}
				else
				{
					flag.SDN_RemoveMember(guid);
						SDN_Logger.LogInfo("O membro " + sender.GetName() + " (" + guid + ") saiu voluntariamente da base " + flag.SDN_GetTerritoryName() + ". [ID: " + flag.SDN_GetTerritoryID() + " | Loc: " + flag.GetPosition().ToString() + "]");
				}
				
				float cooldown = SDN_TerritoryConfig.Get().LeaveTerritoryCooldownMinutes;
				if (cooldown > 0 && SDN_TerritoryDatabase.Get())
				{
				    SDN_TerritoryDatabase.Get().SetCooldown(guid, cooldown);
				}
				
				if (pb)
				{
				    string msg = SDN_TerritoryConfig.Get().MessageSettings.ChatOnly.BaseAbandoned;
				    msg.Replace("%1", SDN_MessageManager.SDN_FormatMinutesToText(cooldown));
				    SDN_MessageManager.SendMemberEvent(pb, "LeaveTerritory", msg);
				}
			}
		}
	}

	void SDN_RPC_AdminReset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param1<TerritoryFlag> data;
			if (!ctx.Read(data)) return;
			
			TerritoryFlag flag = data.param1;
			string guid = sender.GetPlainId();
			PlayerBase pb = PlayerBase.Cast(sender.GetPlayer());
			
			if (!SDN_CheckRPCDistance(pb, flag)) return;
			
			bool isAdmin = false;
			if (SDN_TerritoryConfig.Get().ServerAdmins) isAdmin = SDN_TerritoryConfig.Get().ServerAdmins.Find(guid) != -1;
			if (flag && isAdmin)
			{
				SDN_Logger.LogAdmin("O Administrador " + sender.GetName() + " (" + guid + ") resetou (deletou os dados de) a base " + flag.SDN_GetTerritoryName() + ". [ID: " + flag.SDN_GetTerritoryID() + " | Loc: " + flag.GetPosition().ToString() + "]");
					flag.SDN_ResetMembers();
				flag.SDN_SetTerritoryOwner("");
				flag.SDN_SetTerritoryName("");
				
				if (pb)
				{
				    SDN_MessageManager.SendMemberEvent(pb, "AdminReset", SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryInteractBaseReset);
				}
			}
		}
	}

	void SDN_ReqShowToast(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param3<string, string, string> data;
			if (!ctx.Read(data)) return;
			
			// Apenas uma barreira extra de segurança baseada no PlainId para garantir que
			// um cliente malicioso não faça spoofing do RPC enviando 5.000 chamadas num frame.
			int curTime = GetGame().GetTime();
			string pID = sender.GetPlainId();
			if (SDN_MessageManager.m_SDN_AntiSpamCooldowns.Contains(pID))
			{
				int lastTime = SDN_MessageManager.m_SDN_AntiSpamCooldowns.Get(pID);
				if (curTime < lastTime + 200) return; // Proteção de burst de 200ms
			}
			SDN_MessageManager.m_SDN_AntiSpamCooldowns.Set(pID, curTime);

			NotificationSystem.SendNotificationToPlayerIdentityExtended(sender, 5.0, data.param1, data.param2, data.param3);
		}
	}

	void SDN_ShowToast(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param2<string, string> data;
			if (!ctx.Read(data)) return;
			
			int mode = SDN_TerritoryConfig.Get().MessageSettings.NotificationMode;
			if (mode == 0 || mode == 3) return; 

			string title = SDN_TerritoryConfig.Get().MessageSettings.ToastTitles.RadarTitle;
			string iconPath = "SDN_TerritoryManager/images/Icones/" + data.param2 + ".edds";
			
			NotificationSystem.SendNotificationToPlayerIdentityExtended(sender, 5.0, title, data.param1, iconPath);
			
			// Server toca som via RPC no cliente
			Param2<string, bool> sndData = new Param2<string, bool>(data.param2, false);
			GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_PlaySound", sndData, true, sender);
		}
	}

	void SDN_RPCModSettings(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target) 
	{
		if (sender)
		{
			auto params = new Param1<SDN_TerritoryConfig>(SDN_TerritoryConfig.Get());
			GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPCModSettings", params, true, sender);
		}
	}

	// ==========================================
	// SISTEMA DE INTERCEÇÃO DE CHAT
	// ==========================================
	override void OnEvent(EventType eventTypeId, Param params)
	{
		super.OnEvent(eventTypeId, params);

		if (eventTypeId == ChatMessageEventTypeID)
		{
			ChatMessageEventParams chatParams = ChatMessageEventParams.Cast(params);
			if (chatParams)
			{
				string senderName = chatParams.param2;
				string message = chatParams.param3;
				
				// 1. Comando de Criar Nome (!create)
				if (message.IndexOf("!create ") == 0)
				{
					string territoryName = message.Substring(8, message.Length() - 8);
					PlayerBase playerCreate = SDN_GetPlayerByFlexName(senderName);
					if (playerCreate)
					{
						SDN_ProcessCommandCreate(playerCreate, territoryName);
					}
				}
				
				// 2. NOVO COMANDO: Verificar Status da Base (!SDN_BASE)
				message.ToLower(); // Transforma a mensagem em minúsculas para ignorar erros de digitação do jogador
				if (message == "!sdn_base")
				{
					PlayerBase playerBase = SDN_GetPlayerByFlexName(senderName);
					if (playerBase)
					{
						SDN_ProcessCommandBaseStatus(playerBase);
					}
				}
			}
		}
	}

	// ==========================================
	// LÓGICA DO COMANDO: !SDN_BASE
	// ==========================================
	void SDN_ProcessCommandBaseStatus(PlayerBase player)
	{
		if (!player || !player.GetIdentity()) return;
		
		string guid = player.GetIdentity().GetPlainId();
		vector playerPos = player.GetPosition();
		float radius = SDN_TerritoryConfig.Get().TerritoryRadius;
		
		// 1. Procura a bandeira mais próxima do jogador
		TerritoryFlag targetFlag = null;
		float closestDistSq = radius * radius;

		if (TerritoryFlag.m_SDN_AllFlags)
		{
			foreach (TerritoryFlag flag : TerritoryFlag.m_SDN_AllFlags)
			{
				if (!flag) continue;
				float distSq = vector.DistanceSq(playerPos, flag.GetPosition());
				if (distSq <= closestDistSq)
				{
					targetFlag = flag;
					closestDistSq = distSq;
				}
			}
		}

		// 2. Verifica se encontrou uma bandeira
		if (!targetFlag)
		{
			string msgNotInBase = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdErrorNotInBase;
			SDN_MessageManager.SendAlert(player, "TerritoryCmdErrorNotInBase", msgNotInBase, true);
			return;
		}

		// 3. Segurança: Verifica se é o dono da base
		if (!targetFlag.SDN_IsTerritoryOwner(guid))
		{
			string msgNotOwner = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdErrorNotOwner;
			SDN_MessageManager.SendAlert(player, "TerritoryCmdErrorNotOwner", msgNotOwner, true);
			return;
		}

		// 4. Inicia a Matemática
		int maxParts = SDN_TerritoryConfig.Get().BaseBuildPartsMax;
		if (maxParts <= 0)
		{
			string msgNoLim = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdNoLimits;
			SDN_MessageManager.SendAlert(player, "TerritoryCmdNoLimits", msgNoLim, false);
			return;
		}

		int currentParts = 0;
		TStringArray limitsList = SDN_TerritoryConfig.Get().BaseBuildParts;
		
		// Otimização: Varre o raio para contar apenas as peças configuradas no JSON
		array<Object> objectsInRadius = new array<Object>;
		GetGame().GetObjectsAtPosition(targetFlag.GetPosition(), radius, objectsInRadius, null);

		foreach (Object obj : objectsInRadius)
		{
			if (!obj) continue;

			string objType = obj.GetType();
			objType.ToLower();

			foreach (string partClass : limitsList)
			{
				string checkClass = partClass;
				checkClass.ToLower();

				if (objType.Contains(checkClass))
				{
					currentParts++;
					break;
				}
			}
		}

		// Calcula os restantes
		int remainingParts = maxParts - currentParts;
		if (remainingParts < 0) remainingParts = 0;

		// 5. Formatação do Texto e Retorno Visual
		string statusFormat = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdBaseStatus;
		string statusText = string.Format(statusFormat, maxParts, currentParts, remainingParts);
		
		// Utilizamos o Alert padrão mas forçando visual Toast sem o ErrorTitle (apenas info)
		SDN_MessageManager.SendAlert(player, "TerritoryCmdBaseStatus", statusText, false);
	}

	// ==========================================
	// LÓGICA DO COMANDO: !CREATE
	// ==========================================
	void SDN_ProcessCommandCreate(PlayerBase player, string text)
	{
		if (!player || !player.GetIdentity()) return;
		
		string guid = player.GetIdentity().GetPlainId();
		text.Trim(); 
		
		if (text.Length() < 3 || text.Length() > 20)
		{
			string msgLen = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdCreateLengthError;
			SDN_MessageManager.SendMemberEvent(player, "TerritoryCmdCreateLengthError", msgLen);
			return;
		}
		
		if (!SDN_IsAlphanumeric(text))
		{
			string msgAlp = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdCreateAlphaError;
			SDN_MessageManager.SendMemberEvent(player, "TerritoryCmdCreateAlphaError", msgAlp);
			return;
		}
		
		TerritoryFlag targetFlag = null;
		float closestDistSq = 25.0; 
		vector playerPos = player.GetPosition();

		if (TerritoryFlag.m_SDN_AllFlags)
		{
			foreach (TerritoryFlag flag : TerritoryFlag.m_SDN_AllFlags)
			{
				if (flag && flag.SDN_IsTerritoryOwner(guid) && flag.SDN_GetTerritoryName() == "")
				{
					float distSq = vector.DistanceSq(playerPos, flag.GetPosition());
					if (distSq <= closestDistSq)
					{
						targetFlag = flag;
						closestDistSq = distSq;
					}
				}
			}
		}

		if (targetFlag)
		{
			targetFlag.SDN_SetTerritoryName(text);
				SDN_Logger.LogInfo("O jogador " + player.GetIdentity().GetName() + " (" + guid + ") registrou a base via CHAT CMD: " + text + " [ID: " + targetFlag.SDN_GetTerritoryID() + " | Loc: " + targetFlag.GetPosition().ToString() + "]");
			string msgSuc = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdCreateSuccess;
			msgSuc.Replace("%1", text);
			SDN_MessageManager.SendMemberEvent(player, "TerritoryCmdCreateSuccess", msgSuc);
		}
		else
		{
			string msgFar = SDN_TerritoryConfig.Get().MessageSettings.Alerts.TerritoryCmdCreateTooFar;
			SDN_MessageManager.SendMemberEvent(player, "TerritoryCmdCreateTooFar", msgFar);
		}
	}

	bool SDN_IsAlphanumeric(string text)
	{
		string allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
		for (int i = 0; i < text.Length(); i++)
		{
			string character = text.Substring(i, 1);
			if (!allowed.Contains(character)) return false;
		}
		return true;
	}

	PlayerBase SDN_GetPlayerByFlexName(string senderName)
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		
		PlayerBase bestMatch = null;
		int longestMatch = 0;

		foreach(Man p : players)
		{
			PlayerBase pb = PlayerBase.Cast(p);
			if (pb && pb.GetIdentity())
			{
				string pName = pb.GetIdentity().GetName();
				
				if (senderName.Contains(pName))
				{
					if (pName.Length() > longestMatch)
					{
						longestMatch = pName.Length();
						bestMatch = pb;
					}
				}
			}
		}
		return bestMatch;
	}
}