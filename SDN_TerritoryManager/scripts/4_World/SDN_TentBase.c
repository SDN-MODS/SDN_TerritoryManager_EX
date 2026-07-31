/*
  Arquivo: SDN_TentBase.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/4_World/SDN_TentBase.c
*/

modded class TentBase
{
	void TentBase()
	{
		m_SDN_IsLegallyPlaced = false;
		RegisterNetSyncVariableBool("m_SDN_IsLegallyPlaced");
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);
		ctx.Write(m_SDN_IsLegallyPlaced);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version)) return false;
		if (!ctx.Read(m_SDN_IsLegallyPlaced)) m_SDN_IsLegallyPlaced = true;
		return true;
	}

	// Evento Vanilla da engine chamado no exato momento em que o item é gerado pelo Central Economy
	override void EEOnCECreate()
	{
		super.EEOnCECreate();
		
		// Define o tempo de vida inicial da tenda recém-spawnada baseando-se no nosso JSON
		this.SetLifetime(SDN_TerritoryConfig.Get().TentCESpawnLifeTime);
	}
	
	// Método customizado protegido pelo nosso prefixo
	void SDN_ResetTentLifeTime()
	{
		// 1. Puxa o tempo de vida máximo do item (definido no types.xml ou na nossa config)
		float maxLifetime = this.GetLifetimeMax();
		
		// 2. Restaura o contador da tenda para o valor máximo
		this.SetLifetime(maxLifetime);
		
		// 3. Otimização: Só chama a API de refresh global de CE se o raio for maior que zero
		float refreshRadius = SDN_TerritoryConfig.Get().TentRadius;
		if (refreshRadius > 0)
		{
			GetCEApi().RadiusLifetimeReset(this.GetPosition(), refreshRadius);
		}
	}
	
	// Evento Vanilla disparado sempre que uma animação do item é alterada (ex: abrir/fechar porta da tenda)
	override void ToggleAnimation(string selection)
	{
		super.ToggleAnimation(selection);
		
		// Se estivermos a rodar no servidor (autoridade máxima), interceptamos a interação para renovar a base
		if (GetGame().IsServer() && SDN_TerritoryConfig.Get().TentRadius >= 0)
		{
			SDN_ResetTentLifeTime();
		}
	}
}
