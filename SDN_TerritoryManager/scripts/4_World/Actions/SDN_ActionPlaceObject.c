/*
  Arquivo: SDN_ActionPlaceObject.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/4_World/Actions/SDN_ActionPlaceObject.c
  Descrição: Fechadura de compatibilidade para mods que usam a ação Vanilla "PlaceObject" (ex: Exodus Base Build).
*/

modded class ActionPlaceObject
{
	static int m_SDN_LastSync_Place = 0;
	protected bool m_SDN_CanPlaceHere_Place = false;
	protected vector m_SDN_LastCheckLocation_Place = vector.Zero;

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		// 1. Deixa a engine original do DayZ (ou do Exodus) fazer as verificações base dela primeiro
		if (super.ActionCondition(player, target, item))
		{
			PlayerBase thePlayer = PlayerBase.Cast(player);
			
			// 2. Interceta o holograma do objeto que o jogador está a tentar colocar
			if (thePlayer && thePlayer.GetHologramLocal() && thePlayer.GetHologramLocal().GetProjectionEntity())
			{
				vector projectionPos = thePlayer.GetHologramLocal().GetProjectionEntity().GetPosition();

				// Otimização: Só recalcula as regras se o jogador mover o holograma mais de 40cm
				if (vector.Distance(m_SDN_LastCheckLocation_Place, projectionPos) > 0.4)
				{
					string theGUID = "";
					if (thePlayer.GetIdentity())
					{
						theGUID = thePlayer.GetIdentity().GetPlainId();
					}

					m_SDN_LastCheckLocation_Place = projectionPos;
					m_SDN_CanPlaceHere_Place = SDN_CanIPlaceHere_Place(item, thePlayer.GetHologramLocal().GetProjectionEntity(), projectionPos, theGUID);
					return m_SDN_CanPlaceHere_Place;
				}
				else
				{
					// Se o holograma não se moveu, devolve o resultado da última verificação matemática
					return m_SDN_CanPlaceHere_Place;
				}
			}
			return true;
		}
		
		return false;
	}

	// ==========================================
	// LÓGICA DE VALIDAÇÃO (CÓPIA DO DEPLOYOBJECT)
	// ==========================================
	protected bool SDN_CanIPlaceHere_Place(EntityAI kit, EntityAI item, vector pos, string GUID = "")
	{
		int curTime = GetGame().GetTime();
		
		if (pos == vector.Zero || !item || !kit)
		{
			m_SDN_CanPlaceHere_Place = false;
			return m_SDN_CanPlaceHere_Place;
		}

		// 1. Verificação de No Build Zones (Zonas Restritas)
		bool canBuildKit = true;
		
		foreach (SDN_NoBuildZone zone : SDN_TerritoryConfig.Get().NoBuildZones)
		{
			if (zone && zone.Check(pos))
			{
				canBuildKit = false;
				break;
			}
		}

		if (!canBuildKit)
		{
			string msgNBZ = SDN_TerritoryConfig.Get().MessageSettings.Alerts.NoBuildZone;
			SDN_MessageManager.SendAlert(PlayerBase.Cast(kit.GetHierarchyRootPlayer()), "NoBuildZone", msgNBZ, true);
			m_SDN_CanPlaceHere_Place = false;
			return m_SDN_CanPlaceHere_Place;
		}

		// 2. Verificação de Whitelist (Itens que ignoram regras de território)
		string kitType = kit.GetType();
		string itemType = item.GetType();
		kitType.ToLower();
		itemType.ToLower();

		bool inWhitelist = false;
		foreach (string whiteEntry : SDN_TerritoryConfig.Get().WhiteList)
		{
			whiteEntry.ToLower();
			if (kitType.Contains(whiteEntry) || itemType.Contains(whiteEntry))
			{
				inWhitelist = true;
				break;
			}
		}

		if (inWhitelist)
		{
			m_SDN_CanPlaceHere_Place = true;
			return m_SDN_CanPlaceHere_Place;
		}

		// 3. Verificação de Bandeiras e Permissões
		float theRadius = SDN_TerritoryConfig.Get().TerritoryRadius;

		// Varredura de território usando o sistema central de bandeiras
		foreach (TerritoryFlag flag : TerritoryFlag.m_SDN_AllFlags)
		{
			if (!flag) continue;

			if (vector.DistanceSq(flag.GetPosition(), pos) <= (theRadius * theRadius))
			{
				if (true)
				{
					m_SDN_CanPlaceHere_Place = flag.SDN_CheckPlayerPermission(GUID, SDN_TerritoryPerm.DEPLOY);
					
					if (!m_SDN_CanPlaceHere_Place)
					{ 	if (GetGame().IsClient()) {
							if (GetGame().GetInput().LocalPress("UADefaultAction", false)) {
								int cT = GetGame().GetTime();
								if (!m_SDN_LastSync_Place || cT > m_SDN_LastSync_Place + 3000) {
									m_SDN_LastSync_Place = cT;
									string msgWit = SDN_TerritoryConfig.Get().MessageSettings.Alerts.WithinTerritory;
									SDN_MessageManager.SendAlert(PlayerBase.Cast(kit.GetHierarchyRootPlayer()), "WithinTerritory", msgWit, true);
								}
							}
						}
						return false;
					}
					else
					{
						// ==========================================
						// PASSO 3: VERIFICAÇÃO DE LIMITES (BASE E ARMAZENAMENTO)
						// ==========================================
						
						// 3.1 - Verifica primeiro se é uma Peça de Base
						int maxParts = SDN_TerritoryConfig.Get().BaseBuildPartsMax;
						TStringArray limitsList = SDN_TerritoryConfig.Get().BaseBuildParts;
						bool isLimitedBuildItem = false;
						
						if (maxParts > 0 && limitsList && limitsList.Count() > 0)
						{
							foreach (string partClass : limitsList)
							{
								string checkClass = partClass; checkClass.ToLower();
								if (kitType.Contains(checkClass) || itemType.Contains(checkClass))
								{
									isLimitedBuildItem = true;
									break;
								}
							}
						}

						if (isLimitedBuildItem)
						{
							if (flag.SDN_IsBuildLimitReached())
							{
								string msgMaxB = SDN_TerritoryConfig.Get().MessageSettings.Alerts.MaxBuildParts;
								msgMaxB.Replace("%1", maxParts.ToString());
								SDN_MessageManager.SendAlert(PlayerBase.Cast(kit.GetHierarchyRootPlayer()), "MaxBuildParts", msgMaxB, true);
								m_SDN_CanPlaceHere_Place = false;
								return m_SDN_CanPlaceHere_Place;
							}
						}
						else
						{
							// 3.2 - Se NÃO é peça de base, verifica se é um Armazenamento
							int maxStorage = SDN_TerritoryConfig.Get().BaseStoragePartsMax;
							TStringArray storageList = SDN_TerritoryConfig.Get().BaseStorageParts;
							bool isStorageItem = false;

							if (maxStorage > 0 && storageList && storageList.Count() > 0)
							{
								foreach (string storageClass : storageList)
								{
									string checkClassS = storageClass; checkClassS.ToLower();
									if (kitType.Contains(checkClassS) || itemType.Contains(checkClassS))
									{
										isStorageItem = true;
										break;
									}
								}
							}

							if (isStorageItem)
							{
								if (flag.SDN_IsStorageLimitReached())
								{
									string msgMaxS = SDN_TerritoryConfig.Get().MessageSettings.Alerts.MaxStorageParts;
									msgMaxS.Replace("%1", maxStorage.ToString());
									SDN_MessageManager.SendAlert(PlayerBase.Cast(kit.GetHierarchyRootPlayer()), "MaxStorageParts", msgMaxS, true);
									m_SDN_CanPlaceHere_Place = false;
									return m_SDN_CanPlaceHere_Place;
								}
							}
						}
					}

					return m_SDN_CanPlaceHere_Place;
				}
			}
		}

		// 4. Verificação de Obrigatoriedade de Território (O Fecho da Porta Traseira)
		if (SDN_TerritoryConfig.Get().RequireTerritory)
		{
			string msgReq = SDN_TerritoryConfig.Get().MessageSettings.Alerts.RequireTerritory;
			SDN_MessageManager.SendAlert(PlayerBase.Cast(kit.GetHierarchyRootPlayer()), "RequireTerritory", msgReq, true);
			m_SDN_CanPlaceHere_Place = false;
		} 
		else 
		{
			m_SDN_CanPlaceHere_Place = true;
		}

		return m_SDN_CanPlaceHere_Place;
	}

	// ==========================================
}
