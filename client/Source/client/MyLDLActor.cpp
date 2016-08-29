// Fill out your copyright notice in the Description page of Project Settings.

#include "client.h"
#include "MyLDLActor.h"

#include "AllowWindowsPlatformTypes.h"
//#include "LDrawModelViewer.h"

// Sets default values
AMyLDLActor::AMyLDLActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//LDrawModelViewer modelViewer;
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

