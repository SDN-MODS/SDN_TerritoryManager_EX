/*
  Arquivo: SDN_ActionConstructor.c
  Caminho: SDN_MODS/SDN_TerritoryManager/scripts/4_World/Actions/SDN_ActionConstructor.c
  Objetivo: Registar todas as ações customizadas do mod no motor do DayZ.
*/

modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		
		// --------------------------------------------------------
		// AÇÕES DA BANDEIRA DE TERRITÓRIO
		// --------------------------------------------------------
		actions.Insert(ActionSDN_OpenTerritoryMenu);
		actions.Insert(ActionSDN_AcceptMembership);
	}
}