/*
  Arquivo: SDN_VanillaStorages.c
  Caminho: SDN_MODS/SDN_TerritoryManager/scripts/4_World/SDN_VanillaStorages.c
*/

modded class Barrel_ColorBase
{
	void Barrel_ColorBase()
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
		if (!ctx.Read(m_SDN_IsLegallyPlaced)) m_SDN_IsLegallyPlaced = true; // Fallback para spawns legados
		return true;
	}
}

modded class SeaChest
{
	void SeaChest()
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
}

modded class WoodenCrate
{
	void WoodenCrate()
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
}
