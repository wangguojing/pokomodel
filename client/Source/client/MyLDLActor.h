// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AllowWindowsPlatformTypes.h"
#include "ProceduralMeshComponent.h"
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

	void printStlTriangle(TArray<FVector> & uvbs, TArray<int32> & uibs, 
		TREVertexArray *vertices, TCULongArray *indices, 
		int ix, int i0, int i1, int i2);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

private:
	UProceduralMeshComponent* mesh;
	
};
