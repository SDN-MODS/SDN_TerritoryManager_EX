/*
  Arquivo: SDN_CodeLockManager.c
  Caminho: scripts/4_World/System/SDN_CodeLockManager.c
*/
class SDN_CodeLockManager
{
    // Função principal que verifica se o cadeado pode ser anexado
    static bool SDN_CanAttachCodeLock(EntityAI target, PlayerBase player)
    {
        // Carrega a configuração usando o padrão correto do seu script
        SDN_TerritoryConfig config = SDN_TerritoryConfig.Get();
        if (!config) return true;

        int maxLocks = config.MaxCodeLocksPerTerritory;


        float radius = config.TerritoryRadius;
        vector pos = target.GetPosition();

        // 1. Encontrar a bandeira de território mais próxima
        array<Object> objectsAround = new array<Object>;
        array<CargoBase> proxyCargos = new array<CargoBase>;
        GetGame().GetObjectsAtPosition(pos, radius, objectsAround, proxyCargos);

        // CORREÇÃO: Utilizando a classe vanilla TerritoryFlag
        TerritoryFlag closestFlag = null;
        float closestDist = radius + 1.0;

        foreach (Object obj : objectsAround)
        {
            // CORREÇÃO: Cast seguro para a classe vanilla
            TerritoryFlag flag = TerritoryFlag.Cast(obj);
            if (flag)
            {
                float dist = vector.Distance(pos, flag.GetPosition());
                if (dist <= radius && dist < closestDist)
                {
                    closestDist = dist;
                    closestFlag = flag;
                }
            }
        }

        // Se não encontrou bandeira, não está em um território, então permite o anexo.
        if (!closestFlag) return true;

        // Check player permissions first
        if (player && player.GetIdentity())
        {
            string guid = player.GetIdentity().GetPlainId();
            
            // Abort if territory is abandoned
            if (SDN_TerritoryDatabase.Get())
            {
                SDN_TerritoryData tData = SDN_TerritoryDatabase.Get().GetTerritory(closestFlag.SDN_GetTerritoryID());
                if (tData && tData.AbandonedTimestamp > 0)
                {
                    if (GetGame().IsServer())
                    {
                        string msgReq = SDN_TerritoryConfig.Get().MessageSettings.Alerts.RequireTerritory;
                        SDN_MessageManager.SendAlert(player, "RequireTerritory", msgReq, true);
                    }
                    return false;
                }
            }

            if (!closestFlag.SDN_IsTerritoryOwner(guid) && !closestFlag.SDN_CheckPlayerPermission(guid, SDN_TerritoryPerm.REMOVEMEMBER))
            {
                if (GetGame().IsServer())
                {
                    SDN_Logger.LogWarning("Player " + player.GetIdentity().GetName() + " (" + guid + ") tentou colocar um CodeLock na base '" + closestFlag.SDN_GetTerritoryName() + "' sem permissao de Moderador. [ID: " + closestFlag.SDN_GetTerritoryID() + " | Loc: " + closestFlag.GetPosition().ToString() + "]");
                    string msgWit = SDN_TerritoryConfig.Get().MessageSettings.Alerts.WithinTerritory;
                    SDN_MessageManager.SendAlert(player, "WithinTerritory", msgWit, true);
                }
                return false;
            }
        }
        else
        {
            // Bloqueia colocar cadeado diretamente do chão (exploit de bypass)
            return false;
        }

        if (maxLocks < 0) return true;

        // 2. Contar cadeados existentes dentro da área do território
        int currentLocks = 0;
        array<Object> territoryObjs = new array<Object>;
        GetGame().GetObjectsAtPosition(closestFlag.GetPosition(), radius, territoryObjs, proxyCargos);

        foreach (Object tObj : territoryObjs)
        {
            // Verifica apenas se for construção ou barraca ou ItemBase geral para abranger mods
            if (tObj.IsInherited(BaseBuildingBase) || tObj.IsInherited(TentBase) || tObj.IsInherited(ItemBase))
            {
                EntityAI baseObj = EntityAI.Cast(tObj);
                if (baseObj && baseObj.GetInventory())
                {
                    for (int i = 0; i < baseObj.GetInventory().AttachmentCount(); i++)
                    {
                        EntityAI attachment = baseObj.GetInventory().GetAttachmentFromIndex(i);
                        if (attachment)
                        {
                            string attType = attachment.GetType();
                            attType.ToLower();
                            if (attachment.IsInherited(CombinationLock) || attType.Contains("codelock"))
                            {
                                currentLocks++;
                            }
                        }
                    }
                }
            }
        }

        // 3. Validação Final
        if (currentLocks >= maxLocks)
        {
            if (GetGame().IsServer())
            {
                if (player && player.GetIdentity())
                {
                    SDN_Logger.LogWarning("Player " + player.GetIdentity().GetName() + " (" + player.GetIdentity().GetPlainId() + ") foi impedido de colocar o CodeLock numero " + (currentLocks + 1).ToString() + ". O limite da base e " + maxLocks.ToString() + ". [ID: " + closestFlag.SDN_GetTerritoryID() + " | Loc: " + closestFlag.GetPosition().ToString() + "]");
                }
                SDN_NotifyPlayersLimitReached(target.GetPosition());
            }
            return false;
        }

        return true;
    }

    // --- POST ATTACH (FOR BYPASSING MODS) ---
    static void SDN_CheckAndDropCodeLock(EntityAI target, EntityAI item)
    {
        if (!item || !target) return;

        string attType = item.GetType();
        attType.ToLower();

        if (item.IsInherited(CombinationLock) || attType.Contains("codelock"))
        {
            bool shouldDrop = false;

            // 1. Verifica se quem colocou tinha permissão
            if (SDN_IsPlayerUnauthorizedPostAttach(target))
            {
                shouldDrop = true;
            }
            // 2. Se tinha permissão, verifica se estourou o limite de cadeados da base
            else if (SDN_IsCodeLockLimitExceededPostAttach(target))
            {
                SDN_NotifyPlayersLimitReached(target.GetPosition());
                shouldDrop = true;
            }

            if (shouldDrop)
            {
                // Using CallQueue to delay the drop prevents inventory state corruption
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(SDN_CodeLockManager.SDN_DropItem, target, item);
            }
        }
    }

    static bool SDN_IsPlayerUnauthorizedPostAttach(EntityAI target)
    {
        SDN_TerritoryConfig config = SDN_TerritoryConfig.Get();
        if (!config) return false;

        float radius = config.TerritoryRadius;
        vector pos = target.GetPosition();

        array<Object> objectsAround = new array<Object>;
        array<CargoBase> proxyCargos = new array<CargoBase>;
        GetGame().GetObjectsAtPosition(pos, radius, objectsAround, proxyCargos);

        TerritoryFlag closestFlag = null;
        float closestDist = radius + 1.0;

        foreach (Object obj : objectsAround)
        {
            TerritoryFlag flag = TerritoryFlag.Cast(obj);
            if (flag)
            {
                float dist = vector.Distance(pos, flag.GetPosition());
                if (dist <= radius && dist < closestDist)
                {
                    closestDist = dist;
                    closestFlag = flag;
                }
            }
        }

        if (!closestFlag) return false; // Not in territory

        PlayerBase closestPlayer = null;
        float closestPlayerDist = 5.0; // 5 meters max for attachment
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        foreach (Man p : players)
        {
            float pDist = vector.Distance(p.GetPosition(), target.GetPosition());
            if (pDist < closestPlayerDist)
            {
                closestPlayerDist = pDist;
                closestPlayer = PlayerBase.Cast(p);
            }
        }

        if (closestPlayer && closestPlayer.GetIdentity())
        {
            string guid = closestPlayer.GetIdentity().GetPlainId();
            if (!closestFlag.SDN_IsTerritoryOwner(guid) && !closestFlag.SDN_CheckPlayerPermission(guid, SDN_TerritoryPerm.REMOVEMEMBER))
            {
                string msgWit = SDN_TerritoryConfig.Get().MessageSettings.Alerts.WithinTerritory;
                SDN_Logger.LogWarning("Player " + closestPlayer.GetIdentity().GetName() + " (" + guid + ") tentou forcar a colocacao de um CodeLock na base '" + closestFlag.SDN_GetTerritoryName() + "' sem permissao de Moderador. [ID: " + closestFlag.SDN_GetTerritoryID() + " | Loc: " + closestFlag.GetPosition().ToString() + "]");
                SDN_MessageManager.SendAlert(closestPlayer, "WithinTerritory", msgWit, true);
                return true; // Unauthorized
            }
        }

        return false; // Authorized or no player found
    }

    static void SDN_DropItem(EntityAI target, EntityAI item)
    {
        if (target && item && target.GetInventory())
        {
            target.GetInventory().DropEntity(InventoryMode.SERVER, target, item);
        }
    }

    static bool SDN_IsCodeLockLimitExceededPostAttach(EntityAI target)
    {
        SDN_TerritoryConfig config = SDN_TerritoryConfig.Get();
        if (!config) return false;

        int maxLocks = config.MaxCodeLocksPerTerritory;
        if (maxLocks < 0) return false;

        float radius = config.TerritoryRadius;
        vector pos = target.GetPosition();

        array<Object> objectsAround = new array<Object>;
        array<CargoBase> proxyCargos = new array<CargoBase>;
        GetGame().GetObjectsAtPosition(pos, radius, objectsAround, proxyCargos);

        TerritoryFlag closestFlag = null;
        float closestDist = radius + 1.0;

        foreach (Object obj : objectsAround)
        {
            TerritoryFlag flag = TerritoryFlag.Cast(obj);
            if (flag)
            {
                float dist = vector.Distance(pos, flag.GetPosition());
                if (dist <= radius && dist < closestDist)
                {
                    closestDist = dist;
                    closestFlag = flag;
                }
            }
        }

        if (!closestFlag) return false; // Not in territory

        int currentLocks = 0;
        array<Object> territoryObjs = new array<Object>;
        GetGame().GetObjectsAtPosition(closestFlag.GetPosition(), radius, territoryObjs, proxyCargos);

        foreach (Object tObj : territoryObjs)
        {
            if (tObj.IsInherited(BaseBuildingBase) || tObj.IsInherited(TentBase) || tObj.IsInherited(ItemBase))
            {
                EntityAI baseObj = EntityAI.Cast(tObj);
                if (baseObj && baseObj.GetInventory())
                {
                    for (int i = 0; i < baseObj.GetInventory().AttachmentCount(); i++)
                    {
                        EntityAI attachment = baseObj.GetInventory().GetAttachmentFromIndex(i);
                        if (attachment)
                        {
                            string attType = attachment.GetType();
                            attType.ToLower();
                            if (attachment.IsInherited(CombinationLock) || attType.Contains("codelock"))
                            {
                                currentLocks++;
                            }
                        }
                    }
                }
            }
        }

        // Post-attach: The item is already attached, so it is counted in currentLocks.
        // Therefore, if currentLocks > maxLocks, we exceed the limit.
        if (currentLocks > maxLocks)
        {
            return true;
        }

        return false;
    }


    // Função de notificação via RPC Vanilla
    static void SDN_NotifyPlayersLimitReached(vector pos)
    {
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);

        foreach (Man player : players)
        {
            if (vector.Distance(player.GetPosition(), pos) <= 5.0)
            {
                PlayerBase pb = PlayerBase.Cast(player);
                if (pb && pb.GetIdentity())
                {
                    GetGame().RPCSingleParam(pb, ERPCs.RPC_USER_ACTION_MESSAGE, new Param1<string>("ERRO DE TERRITÓRIO: Limite máximo de Cadeados/CodeLocks atingido!"), true, pb.GetIdentity());
                }
            }
        }
    }
}