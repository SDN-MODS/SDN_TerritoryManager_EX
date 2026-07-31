/*
  Arquivo: SDN_ItemBase.c
  Caminho (relativo ao mod): scripts/4_World/SDN_ItemBase.c
*/

modded class ItemBase extends InventoryItem
{
	protected bool m_SDN_IsStorageCached = false;
	protected bool m_SDN_IsStorage = false;

	protected bool m_SDN_IsVanillaTargetCached = false;
	protected bool m_SDN_IsVanillaTarget = false;

	protected bool m_SDN_IsLegallyPlaced = true;

	bool SDN_IsVanillaStorageTarget()
	{
		if (!m_SDN_IsVanillaTargetCached)
		{
			string t = this.GetType();
			t.ToLower();
			if (t.Contains("barrel_") || t.Contains("seachest") || t.Contains("woodencrate"))
			{
				m_SDN_IsVanillaTarget = true;
			}
			m_SDN_IsVanillaTargetCached = true;
		}
		return m_SDN_IsVanillaTarget;
	}

	bool SDN_CheckIsStorage()
	{
		if (!m_SDN_IsStorageCached)
		{
			if (GetGame().IsServer() && SDN_TerritoryConfig.Get())
			{
				m_SDN_IsStorage = SDN_TerritoryConfig.Get().IsStorageItem(this.GetType());
			}
			m_SDN_IsStorageCached = true;
		}
		return m_SDN_IsStorage;
	}

	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		super.CF_OnStoreSave(storage);
		
		if (GetGame().IsServer())
		{
			if (this.SDN_CheckIsStorage() || SDN_IsVanillaStorageTarget())
			{
				// CORREÇÃO: Utilizando a API moderna do CF com chave String
				CF_ModStorage ctx = storage.Get("SDN_TerritoryManager");
				if (ctx)
				{
					ctx.Write(m_SDN_IsLegallyPlaced);
				}
			}
		}
	}

	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage)) return false;
		
		if (GetGame().IsServer())
		{
			if (this.SDN_CheckIsStorage() || SDN_IsVanillaStorageTarget())
			{
				// CORREÇÃO: Utilizando a API moderna do CF com chave String
				CF_ModStorage ctx = storage.Get("SDN_TerritoryManager");
				if (ctx)
				{
					if (!ctx.Read(m_SDN_IsLegallyPlaced))
					{
						m_SDN_IsLegallyPlaced = true;
					}
				}
				else
				{
					m_SDN_IsLegallyPlaced = true;
				}
				
				SetSynchDirty();
			}
		}
		return true;
	}

	override void OnItemLocationChanged(EntityAI old_owner, EntityAI new_owner)
	{
		super.OnItemLocationChanged(old_owner, new_owner);

		if (GetGame().IsServer())
		{
			// Se o jogador pegar o item para a mão, perde o status de legally placed
			if (new_owner)
			{
				if (this.SDN_CheckIsStorage() || SDN_IsVanillaStorageTarget())
				{
					m_SDN_IsLegallyPlaced = false;
					SetSynchDirty();
				}
			}
			
			// Executa a lógica de LifeTime vanilla APENAS no spawn (quando ambos owners são nulos)
			if (!new_owner && !old_owner)
			{
				if (SDN_IsVanillaStorageTarget())
				{
					if (!TerritoryFlag.SDN_IsInsideTerritory(this.GetPosition()))
					{
						if (SDN_TerritoryConfig.Get() && SDN_TerritoryConfig.Get().LifeTimeStorageVanilla > 0)
						{
							this.SetLifetime(SDN_TerritoryConfig.Get().LifeTimeStorageVanilla);
						}
					}
				}
			}
		}
	}

	override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
	{
		super.OnPlacementComplete(player, position, orientation);

		if (GetGame().IsServer())
		{
			if (this.SDN_CheckIsStorage() || SDN_IsVanillaStorageTarget())
			{
				m_SDN_IsLegallyPlaced = true;
				SetSynchDirty();
				
				if (SDN_IsVanillaStorageTarget())
				{
					if (!TerritoryFlag.SDN_IsInsideTerritory(this.GetPosition()))
					{
						if (SDN_TerritoryConfig.Get() && SDN_TerritoryConfig.Get().LifeTimeStorageVanilla > 0)
						{
							this.SetLifetime(SDN_TerritoryConfig.Get().LifeTimeStorageVanilla);
						}
					}
					else
					{
						this.SetLifetime(this.GetLifetimeMax());
					}
				}
			}
		}
	}

	override void EEOnCECreate()
	{
		super.EEOnCECreate();
		
		if (GetGame().IsServer())
		{
			if (SDN_IsVanillaStorageTarget())
			{
				if (!TerritoryFlag.SDN_IsInsideTerritory(this.GetPosition()))
				{
					if (SDN_TerritoryConfig.Get() && SDN_TerritoryConfig.Get().LifeTimeStorageVanilla > 0)
					{
						this.SetLifetime(SDN_TerritoryConfig.Get().LifeTimeStorageVanilla);
					}
				}
			}
		}
	}

	bool SDN_IsLegallyPlaced()
	{
		return m_SDN_IsLegallyPlaced;
	}

	override bool CanReceiveItemIntoCargo(EntityAI item)
	{
		if (!super.CanReceiveItemIntoCargo(item)) return false;

		// ==========================================
		// REGRA UNIVERSAL PARA BARRIS DE FOGO
		// ==========================================
		string myType = this.GetType();
		myType.ToLower();
		
		// Se EU for um barril de fogueira...
		if (myType.Contains("barrelholes"))
		{
			string incomingType = item.GetType();
			incomingType.ToLower();
			
			// Se o item que estão a tentar meter dentro de mim NÃO for lenha, paus ou cascas...
			if (!incomingType.Contains("firewood") && !incomingType.Contains("woodenstick") && !incomingType.Contains("bark"))
			{
				return false; // Bloqueia a entrada (Ex: Armas, latas de comida, tendas)
			}
			
			// Se chegou aqui, é madeira/casca, então DEIXA PASSAR e ignora as regras do território!
			return true;
		}

		// ==========================================
		// REGRA DO TERRITÓRIO (Para baús e barris normais)
		// ==========================================
		if (SDN_IsVanillaStorageTarget())
		{
			if (!m_SDN_IsLegallyPlaced) return false;
		}
		else
		{
			if (GetGame().IsServer())
			{
				if (this.SDN_CheckIsStorage() && !m_SDN_IsLegallyPlaced) return false;
			}
		}
		
		return true;
	}

	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (!super.CanReceiveAttachment(attachment, slotId)) return false;
		
		if (SDN_IsVanillaStorageTarget())
		{
			if (!m_SDN_IsLegallyPlaced) return false;
		}
		else
		{
			if (GetGame().IsServer())
			{
				if (this.SDN_CheckIsStorage() && !m_SDN_IsLegallyPlaced) return false;
			}
		}
		
		return true;
	}

	override bool CanReleaseCargo(EntityAI cargo)
	{
		if (!super.CanReleaseCargo(cargo)) return false;

		if (SDN_IsVanillaStorageTarget() && !m_SDN_IsLegallyPlaced) return false;
		
		return true;
	}
	
	override bool CanReleaseAttachment(EntityAI attachment)
	{
		if (!super.CanReleaseAttachment(attachment)) return false;

		if (SDN_IsVanillaStorageTarget() && !m_SDN_IsLegallyPlaced) return false;
		
		return true;
	}

	override bool CanDisplayCargo()
	{
		if (!super.CanDisplayCargo()) return false;

		if (SDN_IsVanillaStorageTarget() && !m_SDN_IsLegallyPlaced) return false;

		return true;
	}

	override bool CanDisplayAttachmentCategory(string category_name)
	{
		if (!super.CanDisplayAttachmentCategory(category_name)) return false;

		if (SDN_IsVanillaStorageTarget() && !m_SDN_IsLegallyPlaced) return false;

		return true;
	}

	// ==========================================
	// CADEADO DE MOTOR (ANTI-ROUBO DE CAIXAS CHEIAS)
	// ==========================================
	override bool CanPutIntoHands(EntityAI parent)
	{
		if (!super.CanPutIntoHands(parent)) return false;

		if (this.SDN_CheckIsStorage() || SDN_IsVanillaStorageTarget())
		{
			if (this.GetInventory())
			{
				if (this.GetInventory().GetCargo() && this.GetInventory().GetCargo().GetItemCount() > 0) return false;
				if (this.GetInventory().AttachmentCount() > 0) return false;
			}
		}
		return true;
	}

	override bool CanPutInCargo(EntityAI parent)
	{
		if (!super.CanPutInCargo(parent)) return false;

		if (this.SDN_CheckIsStorage() || SDN_IsVanillaStorageTarget())
		{
			if (this.GetInventory())
			{
				if (this.GetInventory().GetCargo() && this.GetInventory().GetCargo().GetItemCount() > 0) return false;
				if (this.GetInventory().AttachmentCount() > 0) return false;
			}
		}
		return true;
	}

	int SDN_GetSyncedLifeTime()
	{
		if (GetGame().IsServer()) return this.GetLifetimeMax();

		int defaultLT = this.ConfigGetInt("lifetime");
		if (defaultLT > 0) return defaultLT;

		return 3888000;
	}
}