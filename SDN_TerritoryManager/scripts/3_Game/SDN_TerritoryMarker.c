/*
  Arquivo: SDN_TerritoryMarker.c
  Caminho (relativo ao mod): SDN_MODS/SDN_TerritoryManager/scripts/3_Game/SDN_TerritoryMarker.c
*/

#ifdef BASICMAP
class SDN_BasicTerritoryMapMarker extends BasicMapCircleMarker 
{
	bool m_SDN_IsOverlapping = false;
	
	void SDN_SetOverlapping(bool state = true)
	{
		m_SDN_IsOverlapping = state;
	}
	
	// Construtor obrigatoriamente tem o mesmo nome da classe
	void SDN_BasicTerritoryMapMarker(string name, vector pos, string icon = "", array<int> colour = NULL, int alpha = 235, bool onHUD = false) 
	{
		Name = name;
		if (icon != "")
		{
			Icon = icon;
		}

		Pos = pos;
		if (colour != NULL)
		{
			Colour = colour;
		}

		Alpha = alpha;
		Is3DMarker = onHUD;
		HideIntersects = false;
		ShowCenterMarker = true;
	}	
	
	// Métodos com 'override' NÂO recebem o prefixo SDN_ para manter a herança funcional da classe pai (BasicMapCircleMarker)
	override bool OnHUD()
	{
		return false;
	}
		
	override int GetColour()
	{
		if (m_SDN_IsOverlapping)
		{
			// Vermelho se os raios de territórios estiverem sobrepostos (conflito)
			return ARGB(190, 255, 44, 20);
		}

		// Laranja padrão do mod
		return ARGB(190, 255, 140, 0);
	}
	
	override string GetIcon()
	{
		// Caminho padrão do BasicMap para o ícone de bandeira
		return "BasicMap\\gui\\images\\flag.paa";
	}
	
	override bool Editable()
	{
		return false; // Bloqueia edição pelo jogador no mapa 2D
	}
	
	override string GetGroup()
	{
		return "SDN_Territories"; // Atualizado para categorizar corretamente na legenda do mapa
	}
}
#endif