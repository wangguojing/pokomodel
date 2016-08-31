// Fill out your copyright notice in the Description page of Project Settings.

#include "client.h"
#include "MyActor.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AMyActor::AMyActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//USphereComponent* SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
	//RootComponent = SphereComponent;

	UProceduralMeshComponent* mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	/**
	*	Create/replace a section for this procedural mesh component.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*/
	//UFUNCTION(BlueprintCallable, Category = "Components|ProceduralMesh", meta = (AutoCreateRefTerm = "Normals,UV0,VertexColors,Tangents"))
	//	void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	// const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents, bool bCreateCollision);

	TArray<FVector> vertices;

	vertices.Add(FVector(0, 0, 0));
	vertices.Add(FVector(0, 100, 0));
	vertices.Add(FVector(0, 0, 100));
	vertices.Add(FVector(0, 100, 100));

	TArray<int32> Triangles;
	Triangles.Add(2);
	Triangles.Add(1);
	Triangles.Add(0);

	TArray<FVector> normals;
	//normals.Add(FVector(-1, 0, 0));
	//normals.Add(FVector(-1, 0, 0));
	//normals.Add(FVector(-1, 0, 0));

	TArray<FVector2D> UV0;
	//UV0.Add(FVector2D(0, 0));
	//UV0.Add(FVector2D(0, 10));
	//UV0.Add(FVector2D(10, 10));

	TArray<FColor> vertexColors;
	vertexColors.Add(FColor(255, 0, 0));
	vertexColors.Add(FColor(255, 0, 0));
	vertexColors.Add(FColor(255, 0, 0));


	TArray<FProcMeshTangent> tangents;
	//tangents.Add(FProcMeshTangent(-1, 1, 1));
	//tangents.Add(FProcMeshTangent(-1, 1, 1));
	//tangents.Add(FProcMeshTangent(-1, 1, 1));

	mesh->CreateMeshSection(1, vertices, Triangles, normals, UV0, vertexColors, tangents, false);

	// With default options
	//mesh->CreateMeshSection(1, vertices, Triangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);

	//RootComponent = mesh;
	mesh->AttachTo(RootComponent);
}

// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

