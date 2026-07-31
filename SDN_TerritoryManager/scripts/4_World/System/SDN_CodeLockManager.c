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

            if (!closestFlag.SDN_IsTerritoryOwner(guid) && !closestFlag.SDN_CheckPlayerPermission(guid, SDN_TerritoryPerm.BUILD))
            {
                if (GetGame().IsServer())
                {
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
                if (baseObj)
                {
                    bool hasLock = false;

                    if (baseObj.GetAttachmentByType(CombinationLock))
                    {
                        currentLocks++;
                    }
                    
                    for (int i = 0; i < baseObj.GetInventory().AttachmentCount(); i++)
                    {
                        EntityAI attachment = baseObj.GetInventory().GetAttachmentFromIndex(i);
                        if (attachment)
                        {
                            string attType = attachment.GetType();
                            attType.ToLower();
                            if (attType.Contains("codelock"))
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
                SDN_NotifyPlayersLimitReached(target.GetPosition());
            }
            return false;
        }

        return true;
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