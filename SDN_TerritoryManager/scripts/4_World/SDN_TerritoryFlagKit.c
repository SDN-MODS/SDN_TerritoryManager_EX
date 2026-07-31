/*
  Arquivo: SDN_TerritoryFlagKit.c
  Caminho: SDN_MODS/SDN_TerritoryManager/scripts/4_World/SDN_TerritoryFlagKit.c
*/

modded class TerritoryFlag
{
	override string GetConstructionKitType()
	{
		// Se o AutoBuild estiver ativado, o jogador não recebe o kit de volta ao desmantelar a base.
		// Isso evita exploits de gerar kits e bandeiras infinitas!
		if (SDN_TerritoryConfig.Get() && SDN_TerritoryConfig.Get().AutoBuildFlagpoleMode > 0)
		{
			return string.Empty;
		}
		
		return super.GetConstructionKitType();
	}
}

modded class TerritoryFlagKit extends KitBase
{	
	override void OnPlacementComplete( Man player, vector position = "0 0 0", vector orientation = "0 0 0" )
	{
		int buildMode = 0;
		if (SDN_TerritoryConfig.Get())
		{
			buildMode = SDN_TerritoryConfig.Get().AutoBuildFlagpoleMode;
		}

		// ==========================================
		// MODO VANILLA: O DayZ trata do assunto normalmente
		// ==========================================
		if (buildMode == 0)
		{
			super.OnPlacementComplete(player, position, orientation);
			return;
		}

		// ==========================================
		// MODO AUTO-BUILD ATIVADO
		// ==========================================
		if ( GetGame().IsServer() )
		{
			PlayerBase player_base = PlayerBase.Cast( player );
			
			// Cria o mastro invisível no chão
			TerritoryFlag FlagKit = TerritoryFlag.Cast( GetGame().CreateObjectEx( "TerritoryFlag", GetPosition(), ECE_PLACE_ON_SURFACE ) );
			
			if (FlagKit)
			{
				FlagKit.SetPosition( position );
				FlagKit.SetOrientation( orientation );

				HideAllSelections();
				SetIsDeploySound( true );

				// MODO 1: Apenas Base e Suporte
				if (buildMode >= 1)
				{
					SDN_ForceBuildPartSafe(FlagKit, player_base, "base");
					SDN_ForceBuildPartSafe(FlagKit, player_base, "support");
				}

				// MODO 2: Adiciona o Mastro
				if (buildMode >= 2)
				{
					SDN_ForceBuildPartSafe(FlagKit, player_base, "pole");
				}

				// MODO 3: Anexa automaticamente o tecido e sobe a bandeira!
				if (buildMode == 3)
				{
					FlagKit.GetInventory().CreateAttachment("Flag_DayZ");		
					
					// Correção: A bandeira não é uma peça de construção, é uma animação!
					// O valor 0.0 garante que ela nasce içada no topo.
					FlagKit.AnimateFlagEx(1.0, player_base);
				}
			}
		}
	}
	
	// ==========================================
	// FUNÇÃO ANTI-CRASH (BYPASS DO RAG_BASEBUILDING)
	// ==========================================
	void SDN_ForceBuildPartSafe(TerritoryFlag flag, PlayerBase player, string part_name)
	{
		// 1. Em vez de lidar com a ConstructionPart que causou o erro, manipulamos o modelo 3D.
		// Forçamos a fase de animação para visível (0) e escondemos a versão arruinada (1)
		flag.SetAnimationPhase(part_name, 0);
		flag.SetAnimationPhase(part_name + "_ruin", 1);
		
		// 2. Atualizamos as texturas
		flag.UpdateVisuals();
		
		// 3. Disparamos a função interna oficial da engine que regista o NavMesh e Caixas de Colisão.
		// Como não chamamos o BuildPartServer, o jogo nunca vai pedir materiais e nunca vai dar Crash!
		flag.OnPartBuiltServer(player, part_name, AT_BUILD_PART);
	}
};