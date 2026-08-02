/*
  Arquivo: SDN_TerritoryMenu.c
  Caminho: SDN_MODS/SDN_TerritoryManager/scripts/5_Mission/mission/SDN_TerritoryMenu.c
  Descrição: Controlador da Interface (GUI) do Gestor de Território SDN.
*/

class SDN_TerritoryMenu extends UIScriptedMenu
{
	protected Widget m_BackgroundPanel;
	protected ButtonWidget m_CloseButton;
	
	protected EditBoxWidget m_EditBoxName;
	protected ButtonWidget m_SaveNameButton;
	
	protected TextWidget m_OwnerText;
	protected TextWidget m_CoordinatesText;
	protected TextWidget m_RadiusText;
	
	protected TextWidget m_BuildLimitText;
	protected TextWidget m_StorageLimitText;
	protected TextWidget m_CodeLockLimitText;
	
	protected TextWidget m_SelectedMemberName;
	
	protected TextWidget m_TextInviteStatus;
	protected ButtonWidget m_ToggleInvitesButton;
	protected TextWidget m_ToggleInvitesText;
	
	protected TextWidget m_MembersTitle;
	protected TextListboxWidget m_MembersList;
	
	protected ButtonWidget m_PromoteButton;
	protected ButtonWidget m_DemoteButton;
	protected ButtonWidget m_KickButton;
	
	protected ButtonWidget m_LeaveBaseButton;
	protected ButtonWidget m_TransferTerritoryButton;
	protected ButtonWidget m_AdminResetButton;
	protected Widget m_ConfirmationRoot;
	protected TextWidget m_ConfirmationTitle;
	protected TextWidget m_ConfirmationMsg;
	protected ButtonWidget m_ConfirmationBtnYes;
	protected ButtonWidget m_ConfirmationBtnNo;

	protected int m_PendingAction = 0;

	const int ACTION_PROMOTE = 1;
	const int ACTION_KICK = 2;
	const int ACTION_TRANSFER = 3;
	const int ACTION_LEAVE = 4;
	const int ACTION_ADMIN_RESET = 5;

	protected TerritoryFlag m_TargetFlag;
	protected bool m_IsAdmin = false;
	
	void SDN_TerritoryMenu()
	{
		GetGame().GetMission().GetOnInputPresetChanged().Insert(OnInputPresetChanged);
	}
	
	void ~SDN_TerritoryMenu()
	{
		GetGame().GetMission().GetOnInputPresetChanged().Remove(OnInputPresetChanged);
	}

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("SDN_TerritoryManager/gui/layouts/SDN_TerritoryMenu.layout");
		
		m_BackgroundPanel = layoutRoot.FindAnyWidget("BackgroundPanel");
		m_CloseButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CloseButton"));
		
		m_EditBoxName = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("EditBoxName"));
		m_SaveNameButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("SaveNameButton"));
		
		m_OwnerText = TextWidget.Cast(layoutRoot.FindAnyWidget("OwnerText"));
		m_CoordinatesText = TextWidget.Cast(layoutRoot.FindAnyWidget("CoordinatesText"));
		m_RadiusText = TextWidget.Cast(layoutRoot.FindAnyWidget("RadiusText"));
		
		m_BuildLimitText = TextWidget.Cast(layoutRoot.FindAnyWidget("BuildLimitText"));
		m_StorageLimitText = TextWidget.Cast(layoutRoot.FindAnyWidget("StorageLimitText"));
		m_CodeLockLimitText = TextWidget.Cast(layoutRoot.FindAnyWidget("CodeLockLimitText"));
		
		m_SelectedMemberName = TextWidget.Cast(layoutRoot.FindAnyWidget("SelectedMemberName"));
		
		m_TextInviteStatus = TextWidget.Cast(layoutRoot.FindAnyWidget("TextInviteStatus"));
		m_ToggleInvitesButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("ToggleInvitesButton"));
		m_ToggleInvitesText = TextWidget.Cast(layoutRoot.FindAnyWidget("ToggleInvitesText"));
		
		m_MembersTitle = TextWidget.Cast(layoutRoot.FindAnyWidget("MembersTitle"));
		m_MembersList = TextListboxWidget.Cast(layoutRoot.FindAnyWidget("MembersList"));
		
		m_PromoteButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("PromoteButton"));
		m_DemoteButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("DemoteButton"));
		m_KickButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("KickButton"));
		
		m_LeaveBaseButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("LeaveBaseButton"));
		m_TransferTerritoryButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("TransferTerritoryButton"));
		m_AdminResetButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("AdminResetButton"));

		// Popup Initialization
		m_ConfirmationRoot = GetGame().GetWorkspace().CreateWidgets("SDN_TerritoryManager/GUI/layouts/SDN_ConfirmationPopup.layout", layoutRoot);
		if (m_ConfirmationRoot)
		{
			m_ConfirmationTitle = TextWidget.Cast(m_ConfirmationRoot.FindAnyWidget("SDN_TxtTitle"));
			m_ConfirmationMsg = TextWidget.Cast(m_ConfirmationRoot.FindAnyWidget("SDN_TxtMessage"));
			m_ConfirmationBtnYes = ButtonWidget.Cast(m_ConfirmationRoot.FindAnyWidget("SDN_BtnConfirm"));
			m_ConfirmationBtnNo = ButtonWidget.Cast(m_ConfirmationRoot.FindAnyWidget("SDN_BtnCancel"));
			m_ConfirmationRoot.Show(false);
		}

		return layoutRoot;
	}
	void UpdateMenu()
	{
		if (m_ConfirmationRoot && m_ConfirmationRoot.IsVisible()) m_ConfirmationRoot.Show(false); // Reset popup on update
		if (!m_TargetFlag) return;
		
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !player.GetIdentity()) return;
		string guid = player.GetIdentity().GetPlainId();
		
		m_IsAdmin = false;
		if (SDN_TerritoryConfig.Get() && SDN_TerritoryConfig.Get().ServerAdmins)
		{
			if (SDN_TerritoryConfig.Get().ServerAdmins.Find(guid) != -1) m_IsAdmin = true;
		}

		bool isOwner = m_TargetFlag.SDN_IsTerritoryOwner(guid);
		bool isModerator = m_TargetFlag.SDN_CheckPlayerPermission(guid, SDN_TerritoryPerm.REMOVEMEMBER);
		
		// Admin Bypass
		if (m_IsAdmin)
		{
			isOwner = true;
			isModerator = true;
		}

		// NOME E DONO
		if (m_TargetFlag.SDN_GetTerritoryName() != "")
		{
			m_EditBoxName.SetText(m_TargetFlag.SDN_GetTerritoryName());
		}
		
		string ownerName = m_TargetFlag.SDN_GetTerritoryOwnerName();
		if (ownerName == "") ownerName = "Desconhecido";
		if (m_OwnerText) m_OwnerText.SetText("Current Owner: " + ownerName);
		
		if (m_CoordinatesText)
		{
			vector pos = m_TargetFlag.GetPosition();
			m_CoordinatesText.SetText("Central Coordinates: X: " + pos[0].ToString() + " | Y: " + pos[2].ToString());
		}
		
		if (m_RadiusText)
		{
			float radius = SDN_TerritoryConfig.Get().TerritoryRadius;
			m_RadiusText.SetText("Protection Radius: " + radius.ToString() + "m");
		}

		// BLOQUEIOS BASEADOS EM PERMISSÃO
		m_EditBoxName.Enable(m_TargetFlag.SDN_CanReceiveNewOwner());
		m_SaveNameButton.Show(m_TargetFlag.SDN_CanReceiveNewOwner());

		// LIMITES (Exibidos para todos os membros)
		if (m_TargetFlag.SDN_IsTerritoryMember(guid) || isOwner || isModerator)
		{
			int maxParts = SDN_TerritoryConfig.Get().BaseBuildPartsMax;
			int curParts = m_TargetFlag.SDN_GetBuiltPartsCount();
			if (m_BuildLimitText) m_BuildLimitText.SetText("Base Parts: " + curParts.ToString() + " / " + maxParts.ToString());
			
			int maxStorage = SDN_TerritoryConfig.Get().BaseStoragePartsMax;
			int curStorage = m_TargetFlag.SDN_GetStorageCount();
			if (m_StorageLimitText) m_StorageLimitText.SetText("Storage Limit: " + curStorage.ToString() + " / " + maxStorage.ToString()); 
			
			int maxLocks = SDN_TerritoryConfig.Get().MaxCodeLocksPerTerritory;
			int curLocks = m_TargetFlag.SDN_GetCodeLockCount();
			string lockMaxStr = maxLocks.ToString();
			if (maxLocks < 0) lockMaxStr = "Ilimitado";
			if (m_CodeLockLimitText) m_CodeLockLimitText.SetText("Code Lock Limit: " + curLocks.ToString() + " / " + lockMaxStr);
		}
		else
		{
			if (m_BuildLimitText) m_BuildLimitText.SetText("Base Parts: Hidden");
			if (m_StorageLimitText) m_StorageLimitText.SetText("Storage Limit: Hidden");
			if (m_CodeLockLimitText) m_CodeLockLimitText.SetText("Code Lock Limit: Hidden");
		}

		// STATUS DE CONVITES
		if (m_TargetFlag.SDN_IsBaseFull())
		{
			m_TextInviteStatus.SetText("Current Status: FULL");
			m_ToggleInvitesText.SetText("LIMITE ALCANÇADO");
			m_ToggleInvitesButton.Show(false);
		}
		else
		{
			if (m_TargetFlag.SDN_CanAddMember())
			{
				m_TextInviteStatus.SetText("Current Status: OPEN (1 min)");
				m_ToggleInvitesText.SetText("CANCELAR CONVITES");
			}
			else
			{
				m_TextInviteStatus.SetText("Current Status: CLOSED");
				m_ToggleInvitesText.SetText("ATIVAR CONVITES");
			}
			m_ToggleInvitesButton.Show(isOwner || isModerator);
		}

		// ATUALIZAÇÃO DO SELECIONADO
		if (m_SelectedMemberName) m_SelectedMemberName.SetText("Selected: None");

		// LISTA DE MEMBROS
		m_MembersList.ClearItems();
		TStringArray members = m_TargetFlag.SDN_TerritoryMembersList();
		int maxMembers = SDN_TerritoryConfig.Get().MaxMembersPerTerritory;
		int curMembers = 0;
		if (members) curMembers = members.Count();
		
		int displayCurMembers = curMembers;
		if (m_TargetFlag.SDN_GetTerritoryOwner() != "") displayCurMembers += 1;
		m_MembersTitle.SetText("BASE MEMBERS (" + displayCurMembers.ToString() + " / " + maxMembers.ToString() + ")");

		// Cores
		int colorOnline = ARGB(255, 50, 205, 50); // Verde
		int colorOffline = ARGB(255, 205, 50, 50); // Vermelho

		// Exibe o Dono primeiro na lista se ele existir
		if (m_TargetFlag.SDN_GetTerritoryOwner() != "")
		{
			string oGUID = m_TargetFlag.SDN_GetTerritoryOwner();
			string oName = m_TargetFlag.SDN_GetTerritoryOwnerName();
			if (oName == "") oName = "Desconhecido";
			
			string oStatus = " [OFF]";
			int oColor = colorOffline;
			
			if (m_TargetFlag.m_SDN_OnlineMembersCache && m_TargetFlag.m_SDN_OnlineMembersCache.Find(oGUID) != -1)
			{
				oStatus = " [ON]";
				oColor = colorOnline;
			}
			
			int ownerRow = m_MembersList.AddItem(oName + oStatus + " (Dono)", new Param1<string>(oGUID), 0);
			m_MembersList.SetItemColor(ownerRow, 0, oColor);
		}

		if (members)
		{
			for (int i = 0; i < members.Count(); i++)
			{
				string memberGuid = members.Get(i);
				string memberName = m_TargetFlag.SDN_GetMemberName(memberGuid);
				if (memberName == "") memberName = "Desconhecido";
				
				int perm = m_TargetFlag.SDN_GetMemberPermission(memberGuid);
				string rank = "Membro";
				if (perm & SDN_TerritoryPerm.REMOVEMEMBER) rank = "Moderador";
				
				string mStatus = " [OFF]";
				int mColor = colorOffline;
				
				if (m_TargetFlag.m_SDN_OnlineMembersCache && m_TargetFlag.m_SDN_OnlineMembersCache.Find(memberGuid) != -1)
				{
					mStatus = " [ON]";
					mColor = colorOnline;
				}
				
				int memberRow = m_MembersList.AddItem(memberName + mStatus + " (" + rank + ")", new Param1<string>(memberGuid), 0);
				m_MembersList.SetItemColor(memberRow, 0, mColor);
			}
		}

		// BOTÕES DE GERENCIAMENTO (Precisam de item selecionado, desativados por padrão)
		m_PromoteButton.Show(isOwner);
		m_DemoteButton.Show(isOwner); 
		m_KickButton.Show(isOwner || isModerator);
		
		// SAÍDA E ADMIN
		m_LeaveBaseButton.Show(m_TargetFlag.SDN_IsTerritoryMember(guid) || isOwner);
		m_TransferTerritoryButton.Show(isOwner);
		m_AdminResetButton.Show(m_IsAdmin);
	}

	override void OnShow()
	{
		super.OnShow();
		
		m_TargetFlag = TerritoryFlag.m_SDN_TargetFlagForMenu;
		
		SetFocus(layoutRoot);
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		GetGame().GetUIManager().ShowUICursor(true);
		UpdateMenu();
	}

	override void OnHide()
	{
		super.OnHide();
		GetGame().GetMission().PlayerControlEnable(false);
		GetGame().GetUIManager().ShowUICursor(false);
	}


	void ShowConfirmationPopup(string title, string msg, int action)
	{
		if (m_ConfirmationRoot)
		{
			if (m_ConfirmationTitle) m_ConfirmationTitle.SetText(title);
			if (m_ConfirmationMsg) m_ConfirmationMsg.SetText(msg);
			m_PendingAction = action;
			m_ConfirmationRoot.Show(true);
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		if (button != MouseState.LEFT) return false;
		
		if (!m_TargetFlag) return false;

		if (w == m_CloseButton)
		{
			Close();
			return true;
		}
		else if (w == m_SaveNameButton)
		{
			string newName = m_EditBoxName.GetText();
			GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPC_RenameTerritory", new Param2<TerritoryFlag, string>(m_TargetFlag, newName), true, null);
			return true;
		}
		else if (w == m_ToggleInvitesButton)
		{
			GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPC_ToggleInvites", new Param1<TerritoryFlag>(m_TargetFlag), true, null);
			// Simula atualização instantânea para UX
			m_TargetFlag.SDN_AllowMemberToBeAdded(!m_TargetFlag.SDN_CanAddMember());
			UpdateMenu();
			return true;
		}

		else if (w == m_ConfirmationBtnNo)
		{
			if (m_ConfirmationRoot) m_ConfirmationRoot.Show(false);
			m_PendingAction = 0;
			return true;
		}
		else if (w == m_ConfirmationBtnYes)
		{
			if (m_ConfirmationRoot) m_ConfirmationRoot.Show(false);

			if (m_PendingAction == ACTION_PROMOTE)
			{
				int rowProm2 = m_MembersList.GetSelectedRow();
				if (rowProm2 > -1)
				{
					Param1<string> paramProm2;
					m_MembersList.GetItemData(rowProm2, 0, paramProm2);
					if (paramProm2) GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPC_PromoteMember", new Param2<TerritoryFlag, string>(m_TargetFlag, paramProm2.param1), true, null);
				}
			}
			else if (m_PendingAction == ACTION_KICK)
			{
				int rowKick2 = m_MembersList.GetSelectedRow();
				if (rowKick2 > -1)
				{
					Param1<string> paramKick2;
					m_MembersList.GetItemData(rowKick2, 0, paramKick2);
					if (paramKick2) GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPC_KickMember", new Param2<TerritoryFlag, string>(m_TargetFlag, paramKick2.param1), true, null);
				}
			}
			else if (m_PendingAction == ACTION_TRANSFER)
			{
				int rowTransfer2 = m_MembersList.GetSelectedRow();
				if (rowTransfer2 > -1)
				{
					Param1<string> paramTransfer2;
					m_MembersList.GetItemData(rowTransfer2, 0, paramTransfer2);
					if (paramTransfer2) GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPC_TransferTerritory", new Param2<TerritoryFlag, string>(m_TargetFlag, paramTransfer2.param1), true, null);
				}
			}
			else if (m_PendingAction == ACTION_LEAVE)
			{
				GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPC_LeaveTerritory", new Param1<TerritoryFlag>(m_TargetFlag), true, null);
				Close();
			}
			else if (m_PendingAction == ACTION_ADMIN_RESET)
			{
				GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPC_AdminReset", new Param1<TerritoryFlag>(m_TargetFlag), true, null);
				Close();
			}

			m_PendingAction = 0;
			return true;
		}
		else if (w == m_PromoteButton)
		{
			ShowConfirmationPopup("PROMOVER MEMBRO", "Deseja promover este membro para Moderador?", ACTION_PROMOTE);
			return true;
		}
		else if (w == m_DemoteButton)
		{
			int rowDem = m_MembersList.GetSelectedRow();
			if (rowDem > -1)
			{
				Param1<string> paramDem;
				m_MembersList.GetItemData(rowDem, 0, paramDem);
				if (paramDem)
				{
					GetRPCManager().SendRPC("SDN_TerritoryManager", "SDN_RPC_DemoteMember", new Param2<TerritoryFlag, string>(m_TargetFlag, paramDem.param1), true, null);
				}
			}
			return true;
		}
		else if (w == m_KickButton)
		{
			ShowConfirmationPopup("EXPULSAR MEMBRO", "Deseja remover este membro da base?", ACTION_KICK);
			return true;
		}
		else if (w == m_TransferTerritoryButton)
		{
			ShowConfirmationPopup("TRANSFERIR BASE", "Deseja transferir a base? Você perderá o cargo de dono.", ACTION_TRANSFER);
			return true;
		}
		else if (w == m_LeaveBaseButton)
		{
			ShowConfirmationPopup("SAIR DA BASE", "Deseja sair da base? Você perderá o acesso.", ACTION_LEAVE);
			return true;
		}
		else if (w == m_AdminResetButton)
		{
			ShowConfirmationPopup("RESETAR BASE", "ATENÇÃO ADMIN: Deseja deletar esta base permanentemente?", ACTION_ADMIN_RESET);
			return true;
		}

		else if (w == m_MembersList)
		{
			int selectedRow = m_MembersList.GetSelectedRow();
			if (selectedRow > -1 && m_SelectedMemberName)
			{
				string memberText;
				m_MembersList.GetItemText(selectedRow, 0, memberText);
				
				// Removendo rank (Dono), (Moderador), (Membro) se desejar, mas colocar como Selected resolve.
				m_SelectedMemberName.SetText("Selected: " + memberText);
			}
			return true;
		}

		return false;
	}

	void OnInputPresetChanged()
	{
		#ifdef PLATFORM_CONSOLE
		UpdateMenu();
		#endif
	}
}