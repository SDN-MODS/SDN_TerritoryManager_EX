/*
  Arquivo: SDN_ActionRaiseFlag.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/4_World/Actions/SDN_ActionRaiseFlag.c
*/

modded class ActionRaiseFlag: ActionContinuousBase
{	
	// ==========================================
	// VALIDAÇÃO DA AÇÃO (CLIENTE E SERVIDOR)
	// ==========================================
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (super.ActionCondition(player, target, item))
		{
			TerritoryFlag theFlag = TerritoryFlag.Cast(target.GetObject());
			PlayerBase thePlayer = PlayerBase.Cast(player);

			if (theFlag && thePlayer && thePlayer.GetIdentity())
			{
				// NOVO BLOQUEIO: A ação de subir a bandeira fica oculta até a base receber um nome!
				if (theFlag.SDN_GetTerritoryName() == "") return false;

				// Sincroniza os dados do território (membros/dono) se necessário
				theFlag.SDN_SyncTerritoryRateLimited();

				// Verificação de distância de segurança para interagir com o mastro
				float distance = vector.Distance(theFlag.GetPosition(), thePlayer.GetPosition());
				
				if (distance < UAMaxDistances.BASEBUILDING)
				{
					// Nota SDN: No código original, levantar a bandeira não exige permissão.
					// Isso permite que aliados ou estranhos ajudem a manter a base viva.
					return true;
				}
			}
		}

		return false;
	}
	override void OnStartClient(ActionData action_data)
	{
		super.OnStartClient(action_data);
	}

	
	// ==========================================
	// EXECUÇÃO FINALIZADA DA AÇÃO (SERVER-SIDE)
	// ==========================================
	override void OnFinishProgressServer(ActionData action_data)
	{
		// Não chamamos super aqui se o comportamento vanilla for totalmente substituído, 
		// mas mantemos para garantir compatibilidade com outros mods de animação.
		super.OnFinishProgressServer(action_data);

		TerritoryFlag totem = TerritoryFlag.Cast(action_data.m_Target.GetObject());

		if (totem)
		{
			// Cálculo da nova fase da animação (subindo a bandeira)
			float currentPhase = totem.GetAnimationPhase("flag_mast");
			float nextPhase = currentPhase - UAMisc.FLAG_STEP_INCREMENT;
			
			// Executa a animação no mastro
			totem.AnimateFlagEx(nextPhase, action_data.m_Player);
			
			// Adiciona tempo de persistência (refresh) aos itens na área do território
			totem.AddRefresherTime01(UAMisc.FLAG_STEP_INCREMENT);
			
			// Log de auditoria SDN para o administrador do servidor
			// Print("[SDN_TerritoryManager] Bandeira hasteada por: " + action_data.m_Player.GetIdentity().GetName());
		}
	}
};