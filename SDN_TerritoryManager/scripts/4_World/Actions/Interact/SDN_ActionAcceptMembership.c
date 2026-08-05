/*
  Arquivo: SDN_ActionAcceptMembership.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/4_World/Actions/Interact/SDN_ActionAcceptMembership.c
*/

class ActionSDN_AcceptMembership extends ActionInteractBase
{
	protected bool m_SDN_IsMember = false;

	void ActionSDN_AcceptMembership()
	{
		// Comando de animação vanilla para interagir com itens anexados
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_ATTACHITEM;
	}
	
	override string GetText()
	{
		if (m_SDN_IsMember)
		{
			return "Abandonar Território";
		}

		return "Aceitar Convite / Juntar-se";
	}
	
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{	
		TerritoryFlag theFlag = TerritoryFlag.Cast(target.GetObject());
		PlayerIdentity ident = player.GetIdentity();

		if (ident && theFlag)
		{
			string playerGUID = ident.GetPlainId();
			
			// Atualiza o estado da variável para o GetText()
			m_SDN_IsMember = (theFlag.SDN_IsTerritoryMember(playerGUID) && !theFlag.SDN_CanReceiveNewOwner());
			
			// Caso 1: Jogador não é membro e a bandeira está aberta para novos membros (convite ativo)
			if (theFlag.SDN_CanAddMember() && !m_SDN_IsMember)
			{
				// NOVO BLOQUEIO: Limite de membros na base!
				if (theFlag.SDN_IsBaseFull())
				{
					return false; // Nem sequer mostra a opção se a base estiver lotada
				}
				return true;
			} 
			// Caso 2: Jogador já é membro mas NÃO é o dono (quer sair do território)
			else if (m_SDN_IsMember && !theFlag.SDN_IsTerritoryOwner(playerGUID))
			{
				return true;
			}
		}

		return false;
	}
	
	override void OnExecuteServer(ActionData action_data)
	{
		if (action_data && action_data.m_Target && action_data.m_Player)
		{
			TerritoryFlag theFlag = TerritoryFlag.Cast(action_data.m_Target.GetObject());
			PlayerBase thePlayer = PlayerBase.Cast(action_data.m_Player);

			if (theFlag && thePlayer && thePlayer.GetIdentity())
			{
				string guid = thePlayer.GetIdentity().GetPlainId();
				
				// Lógica de Servidor: Adicionar ou Remover membro baseado no estado atual
				if (!theFlag.SDN_IsTerritoryMember(guid))
				{
					// ==========================================
					// NOVOS BLOQUEIOS: CASTIGO E LIMITE DE BASES
					// ==========================================
					bool isAdmin = (SDN_TerritoryConfig.Get().ServerAdmins.Find(guid) != -1);
					if (!isAdmin && SDN_TerritoryDatabase.Get())
					{
						int cd = SDN_TerritoryDatabase.Get().GetCooldownRemaining(guid);
						if (cd > 0)
						{
							int minutesLeft = cd / 60;
							string msgCD = SDN_TerritoryConfig.Get().MessageSettings.Alerts.PunishmentTimeLeft;
							msgCD.Replace("%1", SDN_MessageManager.SDN_FormatMinutesToText(minutesLeft));
							SDN_MessageManager.SendAlert(thePlayer, "PunishmentTimeLeft", msgCD, true);
							return;
						}
						
						int tCount = SDN_TerritoryDatabase.Get().GetActiveTerritoryCount(guid);
						if (tCount >= SDN_TerritoryConfig.Get().MaxTerritoriesPerPlayer)
						{
							string msgMaxBases = SDN_TerritoryConfig.Get().MessageSettings.Alerts.MaxBasesReached;
							msgMaxBases.Replace("%1", SDN_TerritoryConfig.Get().MaxTerritoriesPerPlayer.ToString());
							SDN_MessageManager.SendAlert(thePlayer, "MaxBasesReached", msgMaxBases, true);
							return;
						}
					}
					
					// Verifica novamente o limite do lado do servidor por segurança
					if (theFlag.SDN_IsBaseFull())
					{
						SDN_MessageManager.SendAlert(thePlayer, "MaxMembers", SDN_TerritoryConfig.Get().MessageSettings.Alerts.MaxMembers, true);
						return;
					}

					theFlag.SDN_AddMember(guid, thePlayer.GetIdentity().GetName());
						SDN_Logger.LogInfo("O jogador " + thePlayer.GetIdentity().GetName() + " (" + guid + ") aceitou o convite e entrou na base '" + theFlag.SDN_GetTerritoryName() + "'. [ID: " + theFlag.SDN_GetTerritoryID() + " | Loc: " + theFlag.GetPosition().ToString() + "]");
					Print("[SDN_TerritoryManager] Jogador " + thePlayer.GetIdentity().GetName() + " (" + guid + ") juntou-se ao território.");
					
					// FORÇA O SYNC INSTANTÂNEO DOS DADOS PARA O CLIENTE (Garante que a UI e permissões atualizem na hora)
					theFlag.SDN_SyncTerritory(thePlayer.GetIdentity());
					theFlag.SDN_SyncTerritory();
				}
				else if (!theFlag.SDN_IsTerritoryOwner(guid))
				{
					theFlag.SDN_RemoveMember(guid);
						SDN_Logger.LogInfo("O membro " + thePlayer.GetIdentity().GetName() + " (" + guid + ") saiu voluntariamente (via mastro) da base '" + theFlag.SDN_GetTerritoryName() + "'. [ID: " + theFlag.SDN_GetTerritoryID() + " | Loc: " + theFlag.GetPosition().ToString() + "]");
					
					// APLICAÇÃO DO CASTIGO AO ABANDONAR!
					float cooldownMinutes = SDN_TerritoryConfig.Get().LeaveTerritoryCooldownMinutes;
					if (SDN_TerritoryDatabase.Get()) SDN_TerritoryDatabase.Get().SetCooldown(guid, cooldownMinutes);
					
					string msgLeft = SDN_TerritoryConfig.Get().MessageSettings.ChatOnly.BaseAbandoned;
					msgLeft.Replace("%1", SDN_MessageManager.SDN_FormatMinutesToText(cooldownMinutes));
					SDN_MessageManager.SendMemberEvent(thePlayer, "BaseAbandoned", msgLeft);
					Print("[SDN_TerritoryManager] Jogador " + thePlayer.GetIdentity().GetName() + " abandonou e recebeu cooldown.");
					
					// Sincroniza a remoção para todos
					theFlag.SDN_SyncTerritory(thePlayer.GetIdentity());
					theFlag.SDN_SyncTerritory();
				}
			}
		}
	}
}