// Fill out your copyright notice in the Description page of Project Settings.

#include "client.h"
#include "MyLDLActor.h"

#include "AllowWindowsPlatformTypes.h"
#include "LDLMainModel.h"

// Sets default values
AMyLDLActor::AMyLDLActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	LDLMainModel * mainModel = new LDLMainModel;
	//mainModel->setAlertSender(this);

	// First, release the current TREModels, if they exist.
	//releaseTREModels();
	//mainModel->setLowResStuds(!flags.qualityStuds);
	//mainModel->setGreenFrontFaces(flags.bfc && flags.greenFrontFaces);
	//mainModel->setRedBackFaces(flags.bfc && flags.redBackFaces);
	//mainModel->setBlueNeutralFaces(flags.bfc && flags.blueNeutralFaces);
	//mainModel->setBlackEdgeLines(flags.blackHighlights);
	//mainModel->setExtraSearchDirs(extraSearchDirs);
	//mainModel->setProcessLDConfig(flags.processLDConfig);
	//mainModel->setSkipValidation(flags.skipValidation);
	//mainModel->setBoundingBoxesOnly(flags.boundingBoxesOnly);
	//mainModel->setSeamWidth(seamWidth);
	//mainModel->setCheckPartTracker(flags.checkPartTracker);
	//mainModel->setTexmaps(flags.texmaps);
	//if (flags.needsResetMpd)
	//{
	//	mpdChildIndex = 0;
	//	flags.needsResetMpd = false;
	//}
	mainModel->load("C:\\Users\\Public\\Documents\\LDraw\\parts\\1.dat");

	//calcSize();
}

// Called when the game starts or when spawned
void AMyLDLActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyLDLActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

