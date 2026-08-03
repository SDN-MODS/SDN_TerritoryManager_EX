/*
  Arquivo: SDN_MessageManager.c
  Caminho: SDN_MODS/SDN_TerritoryManager/scripts/4_World/SDN_MessageManager.c
*/

class SDN_MessageManager
{
	static string SDN_FormatMinutesToText(int totalMinutes)
	{
		int days = totalMinutes / 1440;
		int hours = (totalMinutes % 1440) / 60;
		int minutes = totalMinutes % 60;
		
		string result = "";
		if (days > 0)
		{
			result += days.ToString() + " dia";
			if (days > 1) result += "s";
		}
		
		if (hours > 0)
		{
			if (result != "") result += " e ";
			result += hours.ToString() + " hora";
			if (hours > 1) result += "s";
		}
		
		if (minutes > 0 || result == "") // se deu menos que 1 hora
		{
			if (result != "") result += " e ";
			result += minutes.ToString() + " min";
		}
		
		return result;
	}

	// Usamos pointer string ("PlayerBase<hash>") para permitir cooldown no Client sem Identity
	static ref map<string, int> m_SDN_AntiSpamCooldowns = new map<string, int>();

	static bool IsSpam(PlayerBase player)
	{
		if (!player) return true;
		
		string pID = player.ToString();
		if (player.GetIdentity()) pID = player.GetIdentity().GetPlainId();

		int curTime = GetGame().GetTime();
		if (m_SDN_AntiSpamCooldowns.Contains(pID))
		{
			int lastTime = m_SDN_AntiSpamCooldowns.Get(pID);
			int cooldown = SDN_TerritoryConfig.Get().MessageSettings.AntiSpamCooldownMs;
			
			if (curTime < lastTime + cooldown)
			{
				return true;
			}
		}
		
		// Otimização: Limpa dicionários gigantes se crescer muito
		if (m_SDN_AntiSpamCooldowns.Count() > 200) m_SDN_AntiSpamCooldowns.Clear();

		m_SDN_AntiSpamCooldowns.Set(pID, curTime);
		return false;
	}

	// Disparado do Cliente ou do Servidor
	static void SendAlert(PlayerBase player, string actionName, string msg, bool isError = false)
	{
		if (!player) return;
		
		int mode = SDN_TerritoryConfig.Get().MessageSettings.NotificationMode;
		if (mode == 0) return;
		
		// BARREIRA ANTI-SPAM GLOBAL (Serve para Client limitando holograma e Server)
		if (IsSpam(player)) return;

		// Chat local sempre funciona (Client ou Server)
		if (mode == 2 || mode == 3)
		{
			if (GetGame().IsClient()) player.MessageStatus("[SDN_BASE] " + msg);
			else if (GetGame().IsServer() && player.GetIdentity()) player.MessageStatus("[SDN_BASE] " + msg);
		}
		
		if (mode == 1 || mode == 2)
		{
			string title = SDN_TerritoryConfig.Get().MessageSettings.ToastTitles.AlertTitle;
			if (isError) title = SDN_TerritoryConfig.Get().MessageSettings.ToastTitles.ErrorTitle;
			
			string iconPath = "SDN_TerritoryManager/images/Icones/" + actionName + ".edds";
			if (!FileExist(iconPath)) 
			{
				if (actionName == "TerritoryInteractError" || actionName == "TerritoryCmdErrorNotInBase" || actionName == "TerritoryCmdErrorNotOwner" || actionName == "TerritoryCmdCreateTooFar") iconPath = "SDN_TerritoryManager/images/Icones/TerritoryCmdErrorNotInBase.edds";
				else if (actionName == "NoBuildZone") iconPath = "SDN_TerritoryManager/images/Icones/NoBuildZone.edds";
				else if (actionName == "DistanceConflict") iconPath = "SDN_TerritoryManager/images/Icones/DistanceConflict.edds";
				else if (actionName == "WithinTerritory") iconPath = "SDN_TerritoryManager/images/Icones/WithinTerritory.edds";
				else if (actionName == "RequireTerritory") iconPath = "SDN_TerritoryManager/images/Icones/RequireTerritory.edds";
				else if (actionName == "DeSpawnWarning") iconPath = "SDN_TerritoryManager/images/Icones/DeSpawnWarning.edds";
				else if (actionName == "BuildPartWarning") iconPath = "SDN_TerritoryManager/images/Icones/BuildPartWarning.edds";
				else if (actionName == "MaxBuildParts") iconPath = "SDN_TerritoryManager/images/Icones/MaxBuildParts.edds";
				else if (actionName == "MaxStorageParts") iconPath = "SDN_TerritoryManager/images/Icones/MaxStorageParts.edds";
				else if (actionName == "MaxBasesReached") iconPath = "SDN_TerritoryManager/images/Icones/MaxBasesReached.edds";
				else if (actionName == "OneCabinPerBase") iconPath = "SDN_TerritoryManager/images/Icones/OneCabinPerBase.edds";
				else if (actionName == "BaseFull" || actionName == "MaxMembers") iconPath = "SDN_TerritoryManager/images/Icones/MaxMembers.edds";
				else if (actionName == "TerritoryCmdCreateLengthError") iconPath = "SDN_TerritoryManager/images/Icones/TerritoryCmdCreateLengthError.edds";
				else if (actionName == "TerritoryCmdCreateAlphaError") iconPath = "SDN_TerritoryManager/images/Icones/TerritoryCmdCreateAlphaError.edds";
				else if (actionName == "TerritoryCmdCreateSuccess") iconPath = "SDN_TerritoryManager/images/Icones/TerritoryCmdCreateSuccess.edds";
				else if (actionName == "TerritoryCmdNoLimits") iconPath = "SDN_TerritoryManager/images/Icones/TerritoryCmdNoLimits.edds";
				else if (actionName == "TerritoryCmdBaseStatus") iconPath = "SDN_TerritoryManager/images/Icones/TerritoryCmdBaseStatus.edds";
				else if (actionName == "PunishmentActive") iconPath = "SDN_TerritoryManager/images/Icones/PunishmentActive.edds";
				else if (actionName == "PunishmentTimeLeft") iconPath = "SDN_TerritoryManager/images/Icones/PunishmentTimeLeft.edds";
				else iconPath = "SDN_TerritoryManager/images/Icones/TerritoryCmdErrorNotInBase.edds"; // Fallback final
			}
			
			if (GetGame().IsServer() && player.GetIdentity())
			{
				NotificationSystem.SendNotificationToPlayerIdentityExtended(player.GetIdentity(), 5.0, title, msg, iconPath);
				
				// Server toca som via RPC no cliente
				if (!SDN_TerritoryConfig.Get() || SDN_TerritoryConfig.Get().EnableNotificationSounds == 1) { Param2<string, bool> sndData = new Param2<string, bool>(actionName, isError); GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_PlaySound", sndData, true, player.GetIdentity()); }
			}
			else if (GetGame().IsClient())
			{
				// Cliente Toca Localmente
				SDN_PlayLocalSound(isError);
				
				// Envia pedido pro server gerar o Toast
				Param3<string, string, string> rpcData = new Param3<string, string, string>(title, msg, iconPath);
				GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_ReqShowToast", rpcData, true, null);
			}
		}
	}

	// Novo Método para Mensagens de Membros
	static void SendMemberEvent(PlayerBase player, string actionName, string msg)
	{
		if (!player) return;

		int mode = SDN_TerritoryConfig.Get().MessageSettings.NotificationMode;
		if (mode == 0) return;
		
		if (IsSpam(player)) return;

		if (mode == 2 || mode == 3)
		{
			if (GetGame().IsClient()) player.MessageStatus("[SDN_BASE] " + msg);
			else if (GetGame().IsServer() && player.GetIdentity()) player.MessageStatus("[SDN_BASE] " + msg);
		}

		if (mode == 1 || mode == 2)
		{
			string title = SDN_TerritoryConfig.Get().MessageSettings.ToastTitles.MemberTitle;
			string iconPath = "SDN_TerritoryManager/images/Icones/" + actionName + ".edds";
			if (!FileExist(iconPath)) 
			{
				if (actionName == "BaseAbandoned" || actionName == "BaseAbandonedGlobalAlert") iconPath = "SDN_TerritoryManager/images/Icones/BaseAbandoned.edds";
				else if (actionName == "TransferTerritory") iconPath = "SDN_TerritoryManager/images/Icones/TerritoryCmdCreateSuccess.edds";
				else if (actionName == "PromoteMember") iconPath = "SDN_TerritoryManager/images/Icones/PromotedModerator.edds";
				else if (actionName == "DemoteMember") iconPath = "SDN_TerritoryManager/images/Icones/DemotedMember.edds";
				else if (actionName == "KickMember") iconPath = "SDN_TerritoryManager/images/Icones/MemberKicked.edds";
				else if (actionName == "LeaveTerritory") iconPath = "SDN_TerritoryManager/images/Icones/BaseAbandoned.edds";
				else if (actionName == "AdminReset") iconPath = "SDN_TerritoryManager/images/Icones/TerritoryInteractBaseReset.edds";
				else if (actionName == "ToggleInvites") iconPath = "SDN_TerritoryManager/images/Icones/TerritoryInteractInviteEnabled.edds";
				else iconPath = "SDN_TerritoryManager/images/Icones/TerritoryCmdErrorNotInBase.edds"; // Fallback final
			}
			
			if (GetGame().IsServer() && player.GetIdentity())
			{
				NotificationSystem.SendNotificationToPlayerIdentityExtended(player.GetIdentity(), 5.0, title, msg, iconPath);
				
				if (!SDN_TerritoryConfig.Get() || SDN_TerritoryConfig.Get().EnableNotificationSounds == 1) { Param2<string, bool> sndData = new Param2<string, bool>(actionName, false); GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_PlaySound", sndData, true, player.GetIdentity()); }
			}
			else if (GetGame().IsClient())
			{
				SDN_PlayLocalSound(false);
				Param3<string, string, string> rpcData = new Param3<string, string, string>(title, msg, iconPath);
				GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_ReqShowToast", rpcData, true, null);
			}
		}
	}

	static void SDN_PlayLocalSound(bool isError)
	{
		if (SDN_TerritoryConfig.Get() && SDN_TerritoryConfig.Get().EnableNotificationSounds == 0) return;
		if (!GetGame().IsClient() && !GetGame().IsMultiplayer()) return;
		
		string soundSet = "SDN_Notification_Normal_SoundSet";
		if (isError) soundSet = "SDN_Notification_Error_SoundSet";
		
		EffectSound sound = SEffectManager.PlaySound(soundSet, GetGame().GetPlayer().GetPosition());
		if (sound)
		{
			sound.SetAutodestroy(true);
		}
	}
}
