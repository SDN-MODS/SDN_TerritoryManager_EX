/*
  Arquivo: SDN_ActionDismantlePart.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/4_World/Actions/SDN_ActionDismantlePart.c
*/

modded class ActionDismantlePart : ActionContinuousBase
{
	// ==========================================
	// VARIÁVEIS DE CONTROLE (CLIENT-SIDE CACHE)
	// ==========================================
	// Cooldown de segurança para evitar spam de mensagens na tela do jogador
	static int m_SDN_LastDismantleWarningTime = 0;

	// ==========================================
	// VALIDAÇÃO DA AÇÃO (CLIENTE E SERVIDOR)
	// ==========================================
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!super.ActionCondition(player, target, item))
		{
			return false;
		}

		ItemBase theTarget;
		
		// Verifica se o alvo é o próprio objeto ou seu 'parent' (ex: uma parede de cerca)
		if (Class.CastTo(theTarget, target.GetObject()) || Class.CastTo(theTarget, target.GetParent()))
		{
			PlayerBase thePlayer = PlayerBase.Cast(player);
			
			if (theTarget && thePlayer) 
			{
				string theGUID = "";
				
				if (thePlayer.GetIdentity()) 
				{
					theGUID = thePlayer.GetIdentity().GetPlainId();
				}

				// SDN_ModArchitect Note: 
				// O bloqueio 'if (IsServer) return true' foi removido para garantir 
				// que o servidor valide matematicamente o desmonte, impedindo hackers.
				
				// Validação de permissão SDN_TerritoryPerm.DISMANTLE
				if (!TerritoryFlag.SDN_HasTerritoryPermAtPos(theGUID, SDN_TerritoryPerm.DISMANTLE, theTarget.GetPosition()))
				{
					if (!SDN_TerritoryConfig.Get() || SDN_TerritoryConfig.Get().PreventEnemyDismantle == 1)
					{
						return false;
					}
				}
			}
		}

		return true;
	}

	override void OnStartServer(ActionData action_data)
	{
		super.OnStartServer(action_data);

		ItemBase theTarget;
		if (Class.CastTo(theTarget, action_data.m_Target.GetObject()) || Class.CastTo(theTarget, action_data.m_Target.GetParent()))
		{
			PlayerBase thePlayer = PlayerBase.Cast(action_data.m_Player);
			if (theTarget && thePlayer && thePlayer.GetIdentity())
			{
				string theGUID = thePlayer.GetIdentity().GetPlainId();
				if (!TerritoryFlag.SDN_HasTerritoryPermAtPos(theGUID, SDN_TerritoryPerm.DISMANTLE, theTarget.GetPosition()))
				{
					string pname = thePlayer.GetIdentity().GetName();
					if (!SDN_TerritoryConfig.Get() || SDN_TerritoryConfig.Get().PreventEnemyDismantle == 1)
					{
						SDN_Logger.LogRaid("RAID BLOQUEADO: Player " + pname + " (" + theGUID + ") tentou desmantelar uma peca em um territorio sem permissao de DISMANTLE. [Base_Location: " + theTarget.GetPosition().ToString() + "]");
					}
					else
					{
						SDN_Logger.LogRaid("RAID INICIADO: Player " + pname + " (" + theGUID + ") esta a desmantelar uma peca inimiga. [Base_Location: " + theTarget.GetPosition().ToString() + "]");
					}
				}
			}
		}
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		super.OnFinishProgressServer(action_data);

		ItemBase theTarget;
		if (Class.CastTo(theTarget, action_data.m_Target.GetObject()) || Class.CastTo(theTarget, action_data.m_Target.GetParent()))
		{
			PlayerBase thePlayer = PlayerBase.Cast(action_data.m_Player);
			if (theTarget && thePlayer && thePlayer.GetIdentity())
			{
				string theGUID = thePlayer.GetIdentity().GetPlainId();
				if (!TerritoryFlag.SDN_HasTerritoryPermAtPos(theGUID, SDN_TerritoryPerm.DISMANTLE, theTarget.GetPosition()))
				{
					if (SDN_TerritoryConfig.Get() && SDN_TerritoryConfig.Get().PreventEnemyDismantle == 0)
					{
						SDN_Logger.LogRaid("RAID CONCLUIDO: Player " + thePlayer.GetIdentity().GetName() + " (" + theGUID + ") terminou de desmantelar a peca inimiga. [Base_Location: " + theTarget.GetPosition().ToString() + "]");
					}
				}
			}
		}
	}
}
