/*
  Arquivo: SDN_ActionBuildPart.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/4_World/Actions/SDN_ActionBuildPart.c
*/

modded class ActionBuildPart: ActionContinuousBase
{
	// ==========================================
	// VARIÁVEIS DE CONTROLE (CLIENT-SIDE CACHE)
	// ==========================================
	// Cooldown de segurança para evitar que o Cliente espanque a tela do jogador 
	// com dezenas de notificações por segundo ao olhar para o holograma.
	static int m_SDN_LastBuildWarningTime = 0;

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
		
		// Verifica se o alvo é o próprio objeto base ou se faz parte do "parent" (ex: peças anexadas a uma parede)
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

				// Validação Central de Permissões Matemáticas (Rodando instantaneamente graças ao nosso Cache de Bandeiras)
				if (!TerritoryFlag.SDN_HasTerritoryPermAtPos(theGUID, SDN_TerritoryPerm.BUILD, theTarget.GetPosition()))
				{
					if (GetGame().IsClient())
					{
							return false;
						}
				}
			}
		}

		return true;
	}
	
	// ==========================================
	// EXECUÇÃO FINALIZADA DA AÇÃO (SERVER-SIDE)
	// ==========================================
	override void OnFinishProgressServer(ActionData action_data)
	{	
		super.OnFinishProgressServer(action_data);

		TerritoryFlag theFlag = TerritoryFlag.Cast(action_data.m_Target.GetObject());
		if (!theFlag)
		{
			return;
		}
		
		Construction construction = theFlag.GetConstruction();
		
		// Cast seguro obrigatório em Enforce Script atual para acessar as variáveis internas da ação
		BuildPartActionData buildData = BuildPartActionData.Cast(action_data);
		if (!buildData)
		{
			return;
		}

		string part_name = buildData.m_PartType;
		
		// Verifica se a parte da construção (ex: base) foi construída com sucesso no motor do DayZ
		if (construction && construction.IsPartConstructed(part_name))
		{
			// Se o jogador acabou de construir a parte essencial da bandeira (a base do mastro)
			if (part_name == "base")
			{
				// Removemos o Auto-Claim! A bandeira nasce órfã, exigindo que o jogador a reivindique.
				
				// Aplica o dano bónus à ferramenta (Martelo/Machado) usando a configuração do JSON
				if (action_data.m_MainItem) 
				{
					action_data.m_MainItem.DecreaseHealth(SDN_TerritoryConfig.Get().BuildBonusSledgeDamage, false);
				}
			}
		}
	}

	override void OnStartClient(ActionData action_data)
	{
		super.OnStartClient(action_data);
		
		TerritoryFlag flag = TerritoryFlag.Cast(action_data.m_Target.GetObject());
		if (flag && !flag.SDN_CheckPlayerPermission(action_data.m_Player.GetIdentity().GetId(), SDN_TerritoryPerm.BUILD))
		{
			string msgWit = SDN_TerritoryConfig.Get().MessageSettings.Alerts.WithinTerritory;
			SDN_MessageManager.SendAlert(PlayerBase.Cast(action_data.m_Player), "WithinTerritory", msgWit, true);
		}
	}
}