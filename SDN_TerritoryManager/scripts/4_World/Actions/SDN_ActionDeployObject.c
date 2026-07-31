/*
  Arquivo: SDN_ActionDeployObject.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/4_World/Actions/SDN_ActionDeployObject.c
*/

modded class ActionDeployObject : ActionContinuousBase
{
	protected int m_SDN_LastSync = 0;
	static int m_SDN_LastSync_DeployUI = 0;
	protected bool m_SDN_CanPlaceHere = false;
	protected vector m_SDN_LastCheckLocation = vector.Zero;
	
	#ifdef BASICMAP
	protected ref SDN_BasicTerritoryMapMarker m_SDN_BASICT_Marker;
	#endif
	
	void ~ActionDeployObject() 
	{
		SDN_RemoveTheBasicMapMarker();
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (super.ActionCondition(player, target, item))
		{
			PlayerBase thePlayer = PlayerBase.Cast(player);
			
			// === DECLARAÇÃO ÚNICA DA VARIÁVEL (Correção do Crash) ===
			string theGUID = "";
			if (thePlayer && thePlayer.GetIdentity()) 
			{
				theGUID = thePlayer.GetIdentity().GetPlainId();
			}

			// ==========================================
			// 1. A TRAVA ABSOLUTA NO SERVIDOR (ANTI-BYPASS)
			// ==========================================
			if (GetGame().IsServer() && item && item.IsInherited(TerritoryFlagKit) && SDN_TerritoryDatabase.Get())
			{
				bool isAdmin = (SDN_TerritoryConfig.Get().ServerAdmins.Find(theGUID) != -1);
				if (!isAdmin)
				{
					int cd = SDN_TerritoryDatabase.Get().GetCooldownRemaining(theGUID);
					if (cd > 0)
					{
						string msgCastigo = SDN_TerritoryConfig.Get().MessageSettings.Alerts.PunishmentActive;
						SDN_MessageManager.SendAlert(thePlayer, "PunishmentActive", msgCastigo, true);
						return false; 
					}

					int tCount = SDN_TerritoryDatabase.Get().GetActiveTerritoryCount(theGUID);
					if (tCount >= SDN_TerritoryConfig.Get().MaxTerritoriesPerPlayer)
					{
						string msgBases = SDN_TerritoryConfig.Get().MessageSettings.Alerts.MaxBasesReached;
						msgBases.Replace("%1", SDN_TerritoryConfig.Get().MaxTerritoriesPerPlayer.ToString());
						SDN_MessageManager.SendAlert(thePlayer, "MaxBasesReached", msgBases, true);
						return false; 
					}
				}
			}

			// ==========================================
			// 2. LÓGICA DO HOLOGRAMA (CLIENT-SIDE)
			// ==========================================
			if (thePlayer && thePlayer.GetHologramLocal() && thePlayer.GetHologramLocal().GetProjectionEntity()) 
			{
				vector projectionPos = thePlayer.GetHologramLocal().GetProjectionEntity().GetPosition();

				if (vector.Distance(m_SDN_LastCheckLocation, projectionPos) > 0.4)
				{
					#ifdef BASICMAP
					bool ShowTerritoryOnMap = BasicMap().ShowSelfOnMap();
					if (item && item.IsInherited(TerritoryFlagKit) && ShowTerritoryOnMap)
					{
						if (!m_SDN_BASICT_Marker)
						{
							Print("[SDN_TerritoryManager] [BasicMap] Criando Marcador para TerritoryFlag");
							m_SDN_BASICT_Marker = new SDN_BasicTerritoryMapMarker("", projectionPos);
							m_SDN_BASICT_Marker.SetRadius(SDN_TerritoryConfig.Get().TerritoryRadius);
							BasicMap().AddMarker("SDN_Territories", m_SDN_BASICT_Marker);
						}
						else if (m_SDN_BASICT_Marker.GetPosition() != projectionPos)
						{
							if (m_SDN_BASICT_Marker.GetPosition() == vector.Zero)
							{
								BasicMap().AddMarker("SDN_Territories", m_SDN_BASICT_Marker);
							}

							m_SDN_BASICT_Marker.SetPosition(projectionPos);
							m_SDN_BASICT_Marker.SetRadius(SDN_TerritoryConfig.Get().TerritoryRadius);
						}
					}
					else 
					{
						if (m_SDN_BASICT_Marker && m_SDN_BASICT_Marker.GetPosition() != vector.Zero && ShowTerritoryOnMap)
						{
							BasicMap().RemoveMarker("SDN_Territories", m_SDN_BASICT_Marker);
							m_SDN_BASICT_Marker.SetPosition(vector.Zero);
							m_SDN_BASICT_Marker.SetRadius(0);
						}
					}
					#endif

					// === A declaração dupla que estava aqui foi eliminada! ===
					
					m_SDN_LastCheckLocation = projectionPos;
					m_SDN_CanPlaceHere = SDN_CanIPlaceHere(item, thePlayer.GetHologramLocal().GetProjectionEntity(), projectionPos, theGUID);
					return m_SDN_CanPlaceHere;
				}
				else 
				{
					return m_SDN_CanPlaceHere;
				}
			}
			else 
			{
				SDN_RemoveTheBasicMapMarker();
			}

			return true;
		}
		else 
		{
			SDN_RemoveTheBasicMapMarker();
		}

		return false;
	}
	
	override void OnEndClient(ActionData action_data)
	{
		super.OnEndClient(action_data);
	}
	
	void SDN_RemoveTheBasicMapMarker()
	{
		#ifdef BASICMAP
		if (GetGame() && GetGame().IsClient() && m_SDN_BASICT_Marker && m_SDN_BASICT_Marker.GetPosition() != vector.Zero)
		{
			Print("[SDN_TerritoryManager] [BasicMap] Removendo Marcador para TerritoryFlag");
			BasicMap().RemoveMarker(BasicMap().CLIENT_KEY, m_SDN_BASICT_Marker);
			m_SDN_BASICT_Marker.SetPosition(vector.Zero);
			m_SDN_BASICT_Marker.SetRadius(0);
		}
		#endif
	}
	
	protected bool SDN_CanIPlaceHere(EntityAI kit, EntityAI item, vector pos, string GUID = "")
	{
		int curTime = GetGame().GetTime();
		
		if (pos == vector.Zero || !item || !kit)
		{
			m_SDN_CanPlaceHere = false;
			return m_SDN_CanPlaceHere;
		}

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
			m_SDN_CanPlaceHere = false;
			return m_SDN_CanPlaceHere;
		}

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
			m_SDN_CanPlaceHere = true;
			return m_SDN_CanPlaceHere;
		}

		if (kitType == "cabin_kit")
		{
			array<Object> cabinObjects = new array<Object>;
			GetGame().GetObjectsAtPosition(pos, SDN_TerritoryConfig.Get().TerritoryRadius * 1.2, cabinObjects, null);

			for (int x = 0; x < cabinObjects.Count(); x++)
			{
				Object obj = cabinObjects.Get(x);
				if (obj && obj.GetType() == "Prefab_Cabin")
				{
					string msgCab = SDN_TerritoryConfig.Get().MessageSettings.Alerts.OneCabinPerBase;
					SDN_MessageManager.SendAlert(PlayerBase.Cast(kit.GetHierarchyRootPlayer()), "OneCabinPerBase", msgCab, true);
					m_SDN_CanPlaceHere = false;
					return m_SDN_CanPlaceHere;
				}
			}
		}
		
		float theRadius = SDN_TerritoryConfig.Get().TerritoryRadius;
		bool isFlagKit = kit.IsInherited(TerritoryFlagKit);

		if (isFlagKit)
		{
			// ==========================================
			// LÓGICA DA DISTÂNCIA MÍNIMA ENTRE TERRITÓRIOS
			// ==========================================
			float minDistance = SDN_TerritoryConfig.Get().MinDistanceBetweenTerritories;
			
			if (minDistance > (theRadius * 2))
			{
				theRadius = minDistance; 
			}
			else
			{
				theRadius = theRadius * 2; 
			}
		}

		bool foundConflict = false;
		foreach (TerritoryFlag flag : TerritoryFlag.m_SDN_AllFlags)
		{
			if (!flag) continue;

			if (vector.DistanceSq(flag.GetPosition(), pos) <= (theRadius * theRadius))
			{
				if (isFlagKit)
				{
					string msgDist = SDN_TerritoryConfig.Get().MessageSettings.Alerts.DistanceConflict;
					msgDist.Replace("%1", SDN_TerritoryConfig.Get().MinDistanceBetweenTerritories.ToString());
					SDN_MessageManager.SendAlert(PlayerBase.Cast(kit.GetHierarchyRootPlayer()), "DistanceConflict", msgDist, true);

					#ifdef BASICMAP
					if (m_SDN_BASICT_Marker)
					{
						m_SDN_BASICT_Marker.SDN_SetOverlapping(true);
					}
					#endif

					m_SDN_CanPlaceHere = false;
					return m_SDN_CanPlaceHere;
				}

				if (true)
				{
					if (m_SDN_LastSync < curTime)
					{
						m_SDN_LastSync = curTime + 60000;
						flag.SDN_SyncTerritory();
					}

					m_SDN_CanPlaceHere = flag.SDN_CheckPlayerPermission(GUID, SDN_TerritoryPerm.DEPLOY);
					
					if (!m_SDN_CanPlaceHere)
					{
						if (GetGame().IsClient()) {
							if (GetGame().GetInput().LocalPress("UADefaultAction", false)) {
								if (curTime > m_SDN_LastSync_DeployUI + 3000) {
									m_SDN_LastSync_DeployUI = curTime;
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
						// VERIFICAÇÃO DE LIMITES (BASE E ARMAZENAMENTO)
						// ==========================================
						string kType = kit.GetType(); kType.ToLower();
						string iType = item.GetType(); iType.ToLower();
						
						int maxParts = SDN_TerritoryConfig.Get().BaseBuildPartsMax;
						TStringArray limitsList = SDN_TerritoryConfig.Get().BaseBuildParts;
						bool isLimitedBuildItem = false;
						
						if (maxParts > 0 && limitsList && limitsList.Count() > 0)
						{
							foreach (string partClass : limitsList)
							{
								string checkClass = partClass; checkClass.ToLower();
								if (kType.Contains(checkClass) || iType.Contains(checkClass))
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
								m_SDN_CanPlaceHere = false;
								return m_SDN_CanPlaceHere;
							}
						}
						else
						{
							int maxStorage = SDN_TerritoryConfig.Get().BaseStoragePartsMax;
							TStringArray storageList = SDN_TerritoryConfig.Get().BaseStorageParts;
							bool isStorageItem = false;

							if (maxStorage > 0 && storageList && storageList.Count() > 0)
							{
								foreach (string storageClass : storageList)
								{
									string checkClassS = storageClass; checkClassS.ToLower();
									if (kType.Contains(checkClassS) || iType.Contains(checkClassS))
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
									m_SDN_CanPlaceHere = false;
									return m_SDN_CanPlaceHere;
								}
							}
						}
					} 

					return m_SDN_CanPlaceHere; 
				} 
			} 
		} 

		if (isFlagKit)
		{
			#ifdef BASICMAP
			if (m_SDN_BASICT_Marker)
			{
				m_SDN_BASICT_Marker.SDN_SetOverlapping(false);
			}
			#endif
			m_SDN_CanPlaceHere = true;
			return m_SDN_CanPlaceHere;
		}

		if (SDN_TerritoryConfig.Get().RequireTerritory)
		{
			string msgReq = SDN_TerritoryConfig.Get().MessageSettings.Alerts.RequireTerritory;
			SDN_MessageManager.SendAlert(PlayerBase.Cast(kit.GetHierarchyRootPlayer()), "RequireTerritory", msgReq, true);
			m_SDN_CanPlaceHere = false;
		} 
		else 
		{
			string despawnMsg = SDN_TerritoryConfig.Get().MessageSettings.Alerts.DeSpawnWarning;
			int finalLifeTime = 0;
			
			ItemBase theItem = ItemBase.Cast(item);
			ItemBase theKit = ItemBase.Cast(kit);
			string itemNameDisplay = kit.GetDisplayName();
			
			int itemLt = SDN_TerritoryConfig.Get().GetKitLifeTime(itemType);
			if (itemLt <= 0 && theItem)
			{
				itemLt = theItem.SDN_GetSyncedLifeTime();
			}

			int kitLt = SDN_TerritoryConfig.Get().GetKitLifeTime(kitType);
			if (kitLt <= 0 && theKit)
			{
				kitLt = theKit.SDN_GetSyncedLifeTime();
			}

			if (itemLt > kitLt)
			{
				finalLifeTime = itemLt;
				itemNameDisplay = item.GetDisplayName();
			}
			else 
			{
				finalLifeTime = kitLt;
			}

			string niceTime = SDN_TerritoryConfig.Get().GetKitDeSpawnWarning(finalLifeTime, 0);
			
			if (niceTime != "")
			{
				despawnMsg.Replace("%1", niceTime); // NOVO
				despawnMsg.Replace("%2", itemNameDisplay); // NOVO
				SDN_MessageManager.SendAlert(PlayerBase.Cast(kit.GetHierarchyRootPlayer()), "DeSpawnWarning", despawnMsg, false);
			}

			m_SDN_CanPlaceHere = true;
		}

		return m_SDN_CanPlaceHere;
	}
};