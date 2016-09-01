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

	treModel->exportVBO(*vbArray, *ibArray);

	
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

		FVector v(treVertex.v[0], treVertex.v[1], treVertex.v[2]);
		v = v.RotateAngleAxis(90, FVector(1, 0, 0));
		v = v.RotateAngleAxis(180, FVector(0, 1, 0));
		
		vertices.Add(v);
	}

	count = ibArray->getCount();
	for (int i = 0; i < count; i++)
	{
		int index = (*ibArray)[i];
		triangles.Add(index);
	}

	//for (int i = count; i >= 0; i--)
	//{
	//	int index = (*ibArray)[i];
	//	triangles.Add(index);
	//}

	mesh->CreateMeshSection(1, vertices, triangles, normals, UV0, vertexColors, tangents, false);

	TCObject::release(ldModelViewer);
	TCObject::release(vbArray);
	TCObject::release(ibArray);
}

