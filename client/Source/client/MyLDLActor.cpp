// Fill out your copyright notice in the Description page of Project Settings.

#include "client.h"
#include "MyLDLActor.h"


#include "LDrawModelViewer.h"
#include "TCFoundation/TCObject.h"
#include "TCFoundation/TCTypedValueArray.h"

#include "TRE/TREModel.h"
#include "TRE/TREMainModel.h"
#include "TRE/TREVertexStore.h"
#include "TRE/TREShapeGroup.h"

// Sets default values
AMyLDLActor::AMyLDLActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	mesh->AttachTo(RootComponent);
}

// Called when the game starts or when spawned
void AMyLDLActor::BeginPlay()
{
	Super::BeginPlay();

	OnRep_FileName();
}

// Called every frame
void AMyLDLActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void AMyLDLActor::OnRep_FileName()
{
	LDrawModelViewer * ldModelViewer = new LDrawModelViewer(-1, -1);
	ldModelViewer->setFilename(TCHAR_TO_ANSI(*fileName));
	ldModelViewer->reload();

	TREModel * treModel = ldModelViewer->getMainTREModel();

	TREVertexArray * vbArray = new TREVertexArray;
	TCULongArray * ibArray = new TCULongArray;
	TREVertexArray * nbArray = new TREVertexArray;

	treModel->exportVBO(*vbArray, *ibArray, *nbArray);


	TArray<FVector> vertices;
	TArray<int32> triangles;
	TArray<FVector> normals;
	TArray<FVector2D> UV0;
	TArray<FColor> vertexColors;
	TArray<FProcMeshTangent> tangents;

	int count = vbArray->getCount();
	for (int i = 0; i < count; i++)
	{
		const TREVertex &treVertex = (*vbArray)[i];
		vertices.Add(FVector(treVertex.v[0], treVertex.v[1], treVertex.v[2]));
	}

	count = ibArray->getCount();
	for (int i = 0; i < count; i++)
	{
		int index = (*ibArray)[i];
		triangles.Add(index);
	}

	count = nbArray->getCount();
	for (int i = 0; i < count; i++)
	{
		const TREVertex &treNormal = (*nbArray)[i];
		//normals.Add(FVector(treNormal.v[0], treNormal.v[1], treNormal.v[2]));
	}

	//normals.Add(FVector(-1, 0, 0));
	//normals.Add(FVector(-1, 0, 0));
	//normals.Add(FVector(-1, 0, 0));




	mesh->CreateMeshSection(1, vertices, triangles, normals, UV0, vertexColors, tangents, false);

	TCObject::release(ldModelViewer);
	TCObject::release(vbArray);
	TCObject::release(ibArray);
}

