/*
  Arquivo: ActionSDN_OpenTerritoryMenu.c
  Caminho: SDN_MODS/SDN_TerritoryManager/scripts/4_World/Actions/Interact/ActionSDN_OpenTerritoryMenu.c
  Descrição: Action disparada pela tecla 'F' para abrir o Painel UI da base.
*/

class ActionSDN_OpenTerritoryMenu: ActionInteractBase
{
	void ActionSDN_OpenTerritoryMenu()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
		m_HUDCursorIcon = CursorIcons.CloseHood;
	}

	override void CreateConditionComponents()  
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTCursor(UAMaxDistances.DEFAULT);
	}

	override string GetText()
	{
		return "Abrir Gestor de Território";
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		TerritoryFlag flag = TerritoryFlag.Cast(target.GetObject());
		if (!flag) return false;

		// Mostrar a action apenas se tiver mastro construído ou se o config permitir interação prematura
		if (flag.FindAttachmentBySlotName("Material_FPole_Flag"))
		{
			// Verifica se tem permissão (Dono, Membro, ou Admin) ou se a base não tem dono (para reivindicar)
			string guid = "";
			if (player.GetIdentity()) guid = player.GetIdentity().GetPlainId();
			
			if (flag.SDN_CanReceiveNewOwner()) return true; // Para reivindicar (UI abre vazia/bloqueada pedindo nome)
			if (flag.SDN_IsTerritoryOwner(guid)) return true;
			if (flag.SDN_IsTerritoryMember(guid)) return true;
			
			// Admin bypass
			if (SDN_TerritoryConfig.Get() && SDN_TerritoryConfig.Get().ServerAdmins)
			{
				if (SDN_TerritoryConfig.Get().ServerAdmins.Find(guid) != -1) return true;
			}
		}

		return false;
	}

	override void OnExecuteClient(ActionData action_data)
	{
		super.OnExecuteClient(action_data);
		
		TerritoryFlag flag = TerritoryFlag.Cast(action_data.m_Target.GetObject());
		if (flag)
		{
			TerritoryFlag.m_SDN_TargetFlagForMenu = flag;
			GetGame().GetUIManager().EnterScriptedMenu(748392, null);
		}
	}
}