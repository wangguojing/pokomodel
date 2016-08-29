// Fill out your copyright notice in the Description page of Project Settings.

#include "client.h"
#include "clientGameMode.h"

#include "AllowWindowsPlatformTypes.h"
#include "LDrawModelViewer.h"

AclientGameMode::AclientGameMode()
{
	LDrawModelViewer * m = new LDrawModelViewer(100, 100);
	m->setFilename("C://Users//Public//Documents//LDraw//parts//1.dat");
	m->reload();
	//if (m->loadLDLModel() && m->parseModel())
	//{
	//	int a = 1;
	//}
	//else
	//	int b = 2;
}

