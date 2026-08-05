/*
  Arquivo: SDN_ActionLowerFlag.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/4_World/Actions/SDN_ActionLowerFlag.c
*/

modded class ActionLowerFlag: ActionContinuousBase
{		
	// Cooldown para evitar spam de mensagens na UI
	static int m_SDN_LastLowerFlagWarningTime = 0;

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (super.ActionCondition(player, target, item))
		{
			TerritoryFlag theFlag = TerritoryFlag.Cast(target.GetObject());
			PlayerBase thePlayer = PlayerBase.Cast(player);

			if (theFlag && thePlayer && thePlayer.GetIdentity())
			{
				// NOVO BLOQUEIO: A ação de descer a bandeira fica oculta até a base receber um nome!
				if (theFlag.SDN_GetTerritoryName() == "") return false;

				// Sincroniza os dados do território caso o cliente ainda não os tenha
				theFlag.SDN_SyncTerritoryRateLimited();

				// Verificação de distância de segurança para interações com a bandeira
				if (vector.Distance(theFlag.GetPosition(), thePlayer.GetPosition()) > UAMaxDistances.BASEBUILDING)
				{
					return false;
				}

				string theGUID = thePlayer.GetIdentity().GetPlainId();

				if (theFlag.SDN_IsTerritoryOwner(theGUID) || theFlag.SDN_CheckPlayerPermission(theGUID, SDN_TerritoryPerm.LOWERFLAG))
				{
					return true;
				}

				return false;
			}
		}
		
		return false;
	}

	override void OnStartClient(ActionData action_data)
	{
		super.OnStartClient(action_data);
		
		TerritoryFlag theFlag = TerritoryFlag.Cast(action_data.m_Target.GetObject());
		if (theFlag && action_data.m_Player && action_data.m_Player.GetIdentity())
		{
			string guid = action_data.m_Player.GetIdentity().GetPlainId();
			if (!theFlag.SDN_IsTerritoryOwner(guid) && !theFlag.SDN_CheckPlayerPermission(guid, SDN_TerritoryPerm.LOWERFLAG))
			{
				string msg = SDN_TerritoryConfig.Get().MessageSettings.Alerts.WithinTerritory;
				SDN_MessageManager.SendAlert(PlayerBase.Cast(action_data.m_Player), "WithinTerritory", msg, true);
			}
		}
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		super.OnFinishProgressServer(action_data);

		TerritoryFlag theFlag = TerritoryFlag.Cast(action_data.m_Target.GetObject());
		if (theFlag && action_data.m_Player && action_data.m_Player.GetIdentity())
		{
			string guid = action_data.m_Player.GetIdentity().GetPlainId();
			SDN_Logger.LogInfo("A bandeira do territorio '" + theFlag.SDN_GetTerritoryName() + "' foi abaixada por " + action_data.m_Player.GetIdentity().GetName() + " (" + guid + "). [ID: " + theFlag.SDN_GetTerritoryID() + " | Loc: " + theFlag.GetPosition().ToString() + "]");
		}
	}
}
