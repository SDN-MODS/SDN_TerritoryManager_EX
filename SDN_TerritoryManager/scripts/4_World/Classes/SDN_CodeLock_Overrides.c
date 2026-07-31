/*
  Arquivo: SDN_CodeLock_Overrides.c
  Caminho: scripts/4_World/Classes/SDN_CodeLock_Overrides.c
*/

// Cobre Fences, Watchtowers e qualquer base customizada
modded class BaseBuildingBase
{
    override bool CanReceiveAttachment(EntityAI attachment, int slotId)
    {
        // 1. Mantém a lógica de verificação vanilla e de outros mods primeiro
        if (!super.CanReceiveAttachment(attachment, slotId))
            return false;

        string attType = attachment.GetType();
        attType.ToLower();
        
        // 2. Intercepta APENAS se for um cadeado
        if (attachment.IsInherited(CombinationLock) || attType.Contains("codelock"))
        {
            PlayerBase player = null;
            if (attachment.GetHierarchyRootPlayer())
            {
                player = PlayerBase.Cast(attachment.GetHierarchyRootPlayer());
            }

            // Passa para o cérebro gerenciador do mod tomar a decisão
            if (!SDN_CodeLockManager.SDN_CanAttachCodeLock(this, player))
            {
                return false;
            }
        }

        return true;
    }

    override void EEItemAttached(EntityAI item, string slot_name)
    {
        super.EEItemAttached(item, slot_name);
        if (GetGame().IsServer())
        {
            SDN_CodeLockManager.SDN_CheckAndDropCodeLock(this, item);
        }
    }
}

// Cobre todas as Barracas (Tents), que não herdam de BaseBuildingBase, mas de ItemBase
modded class TentBase
{
    override bool CanReceiveAttachment(EntityAI attachment, int slotId)
    {
        if (!super.CanReceiveAttachment(attachment, slotId))
            return false;

        string attType3 = attachment.GetType();
        attType3.ToLower();

        if (attachment.IsInherited(CombinationLock) || attType3.Contains("codelock"))
        {
            PlayerBase player = null;
            if (attachment.GetHierarchyRootPlayer())
            {
                player = PlayerBase.Cast(attachment.GetHierarchyRootPlayer());
            }

            if (!SDN_CodeLockManager.SDN_CanAttachCodeLock(this, player))
            {
                return false;
            }
        }

        return true;
    }

    override void EEItemAttached(EntityAI item, string slot_name)
    {
        super.EEItemAttached(item, slot_name);
        if (GetGame().IsServer())
        {
            SDN_CodeLockManager.SDN_CheckAndDropCodeLock(this, item);
        }
    }
}

// Cobre todos os itens para pegar mods como BBP, etc, que as vezes não herdam de BaseBuildingBase
modded class ItemBase
{
    override bool CanReceiveAttachment(EntityAI attachment, int slotId)
    {
        if (!super.CanReceiveAttachment(attachment, slotId))
            return false;

        // Tents and BaseBuildingBase are ItemBase too, we intercept here just in case they aren't those classes
        string attType2 = attachment.GetType();
        attType2.ToLower();

        if (attachment.IsInherited(CombinationLock) || attType2.Contains("codelock"))
        {
            PlayerBase player = null;
            if (attachment.GetHierarchyRootPlayer())
            {
                player = PlayerBase.Cast(attachment.GetHierarchyRootPlayer());
            }

            if (!SDN_CodeLockManager.SDN_CanAttachCodeLock(this, player))
            {
                return false;
            }
        }

        return true;
    }

    override void EEItemAttached(EntityAI item, string slot_name)
    {
        super.EEItemAttached(item, slot_name);
        if (GetGame().IsServer())
        {
            SDN_CodeLockManager.SDN_CheckAndDropCodeLock(this, item);
        }
    }

    // A ARMA DEFINITIVA CONTRA MODS REBELDES (RBB, etc)
    // Esse evento dispara no PRÓPRIO CADEADO quando ele é anexado a qualquer lugar.
    // Assim não dependemos de que a "Porta" chame o super() do EEItemAttached.
    override void OnWasAttached(EntityAI parent, int slot_id)
    {
        super.OnWasAttached(parent, slot_id);

        if (GetGame().IsServer())
        {
            string myType = this.GetType();
            myType.ToLower();

            // Se ESSE item for um CodeLock ou CombinationLock, manda ele próprio verificar e cair se precisar.
            if (this.IsInherited(CombinationLock) || myType.Contains("codelock"))
            {
                SDN_CodeLockManager.SDN_CheckAndDropCodeLock(parent, this);
            }
        }
    }
}
