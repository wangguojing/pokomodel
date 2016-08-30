// Fill out your copyright notice in the Description page of Project Settings.

#include "client.h"
#include "MyLDLActor.h"

#include "AllowWindowsPlatformTypes.h"
#include "ProceduralMeshComponent.h"

#include "LDrawModelViewer.h"
//#include "TCFoundation/TCObject.h"
//#include "TCFoundation/TCTypedValueArray.h"
//#include "TCFoundation/TCArray.h"

#include "TRE/TREModel.h"
#include "TRE/TREMainModel.h"
//#include "TRE/TREVertexStore.h"
//#include "TRE/TREVertexArray.h"
//#include "TRE/TREShapeGroup.h"

// Sets default values
AMyLDLActor::AMyLDLActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	//LDrawModelViewer modelViewer;

	LDrawModelViewer * ldModelViewer = new LDrawModelViewer(-1, -1);
	ldModelViewer->setFilename("C://Users//Public//Documents//LDraw//parts//1.dat");
	ldModelViewer->reload();

	//TREModel * treModel = ldModelViewer->getMainTREModel()->getCurGeomModel();
	//TREShapeGroup ** treShapeGroup = treModel->getShapes();
	//for (int i = 0; i <= TREMLast; i++)
	//{
	//	TREShapeGroup *shape = treShapeGroup[i];

	//	if (shape != NULL)
	//	{
	//		TCULongArray *indices =
	//			shape->getIndices(TRESTriangle, false);
	//		TREVertexStore *vertexStore = shape->getVertexStore();

	//		if (indices != NULL)
	//		{
	//			TREVertexArray *vertices = vertexStore->getVertices();
	//			//indices->getItems();
	//			//int count = indices->getCount();

	//			//for (int p = 0; p < count; p += 3)
	//			//{

	//			//}
	//		}
	//		//indices = shape->getIndices(TRESQuad, false);
	//		//if (indices != NULL)
	//		//{
	//		//	TREVertexArray *vertices = vertexStore->getVertices();
	//		//	int count = indices->getCount();

	//		//	for (int p = 0; p < count; p += 4)
	//		//	{

	//		//	}
	//		//}
	//	}
	//}

	UProceduralMeshComponent* mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));

	TArray<FVector> vertices;

	vertices.Add(FVector(0, 0, 0));
	vertices.Add(FVector(0, 100, 0));
	vertices.Add(FVector(0, 0, 100));

	TArray<int32> Triangles;
	Triangles.Add(0);
	Triangles.Add(2);
	Triangles.Add(1);

	TArray<FVector> normals;
	TArray<FVector2D> UV0;
	TArray<FColor> vertexColors;
	TArray<FProcMeshTangent> tangents;

	mesh->CreateMeshSection(1, vertices, Triangles, normals, UV0, vertexColors, tangents, false);

	// With default options
	//mesh->CreateMeshSection(1, vertices, Triangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);

	//RootComponent = mesh;
	mesh->AttachTo(RootComponent);

	TCObject::release(ldModelViewer);
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

