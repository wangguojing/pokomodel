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
	
	LDrawModelViewer * ldModelViewer = new LDrawModelViewer(-1, -1);
	ldModelViewer->setFilename("D://MyDocuments//LDraw//Models//Star Wars//7140 - X-Wing Fighter.mpd");
	ldModelViewer->reload();

	TREModel * treModel = ldModelViewer->getMainTREModel();

	TREModel::TREVertexIndexMap vertexIndexMap;
	TREVertexArray * vbArray = new TREVertexArray;
	TCULongArray * ibArray = new TCULongArray;

	treModel->exportVBO(vertexIndexMap, *vbArray, *ibArray);


	TArray<FVector> vertices;
	int count = vbArray->getCount();

	for (int i = 0; i < count; i++)
	{
		const TREVertex &treVertex = (*vbArray)[i];
		vertices.Add(FVector(treVertex.v[0], treVertex.v[1], treVertex.v[2]));
	}

	TArray<int32> triangles;
	count = ibArray->getCount();

	for (int i = 0; i < count; i++)
	{
		int index = (*ibArray)[i];
		triangles.Add(index);
	}

	TArray<FVector> normals;
	TArray<FVector2D> UV0;
	TArray<FColor> vertexColors;
	TArray<FProcMeshTangent> tangents;

	mesh->CreateMeshSection(1, vertices, triangles, normals, UV0, vertexColors, tangents, false);

	// With default options
	//mesh->CreateMeshSection(1, vertices, Triangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);

	TCObject::release(ldModelViewer);
	TCObject::release(vbArray);
	TCObject::release(ibArray);
}

// Called every frame
void AMyLDLActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

