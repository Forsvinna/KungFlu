#pragma once

struct Variables
{
	Variables()
	{

	}
	struct
	{
		bool	Opened = false;
	} Menu;
	struct
	{
		bool	Enabled = true;
		bool	Box = true;
		bool	Health = true;
		bool	Skeleton = true;
		bool	AimSpot = false;
		bool	Name = true;
		bool	Distance = true;
		bool	PlayerEnable = true;
	} Visuals;


};
extern Variables Vars;