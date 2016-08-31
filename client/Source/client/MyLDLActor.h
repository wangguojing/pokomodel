// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AllowWindowsPlatformTypes.h"
#include "ProceduralMeshComponent.h"
#include "UnrealString.h"

#include "TRE/TREVertexArray.h"
#include "TCFoundation/TCTypedValueArray.h"

#include "GameFramework/Actor.h"
#include "MyLDLActor.generated.h"

UCLASS()
class CLIENT_API AMyLDLActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyLDLActor();

	/** If RelativeLocation should be considered relative to the world, rather than the parent */
	UPROPERTY(EditAnywhere)
	FString fileName;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

private:
	UProceduralMeshComponent* mesh;

	//UFUNCTION()
	void OnRep_FileName();
	
};
