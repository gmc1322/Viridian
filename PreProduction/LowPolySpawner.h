/*!------------------------------------------------------------------------------
\file   LowPolySpawner.h

\author Garrett Conti

\par    Project: VIRIDIAN
\par    Course:  GAM300

\par    COPYRIGHT (C) 2018 BY DIGIPEN CORP, USA. ALL RIGHTS RESERVED.
------------------------------------------------------------------------------ */

#pragma once

// This must be first
#include "ObjectMacros.h"

// Unreal Includes
#include "GameFramework/Actor.h"

// Our Includes
#include "Public/Utils/Macros.h"

// STL Includes

// This must be last
#include "LowPolySpawner.generated.h"

// Commonly used forward declarations
class UInstancedStaticMeshComponent;

UCLASS()
class VIRIDIAN_API ALowPolySpawner : public AActor
{
  GENERATED_BODY()

  public:
    ALowPolySpawner() NoExcept;

  public:
#if WITH_EDITOR
    void PostEditChangeProperty( struct FPropertyChangedEvent &PropertyChangedEvent ) NoExcept override;
#else
    void BeginPlay() NoExcept override;
#endif

  // Default Variables
  public:
    // The mesh types to randomly spawn
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Defaults" )
    TArray< UStaticMesh* > GeneratableMeshes;

    // The area to spawn meshes inside
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Defaults" )
    FVector SpawnArea = { 32, 32, 32 };

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Defaults", meta = ( ClampMin = 1 ) )
    int32 SpawnCount = 1; // TODO: link this to foliage spawning graphical options to spawn less on lower settings

    // If true, meshes can spawn inside other objects. (This does not affect spawning inside other instance meshes)
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Defaults" )
    bool AllowGroundOverlap = true;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Defaults" )
    int32 RandomSeed = 0;

#if WITH_EDITORONLY_DATA
    // Used to re-generate meshes after changing variables
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Defaults" )
    bool GenerateMeshes = false;
#endif

  // Random Scale Variables
  public:
    // Enables random scale from Min to Max of the instance meshes
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Random Scale" )
    bool RandomScale = false;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Random Scale", meta = ( EditCondition = "RandomScale" ) )
    FVector MinScale = { 1, 1, 1 };

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Random Scale", meta = ( EditCondition = "RandomScale" ) )
    FVector MaxScale = { 1, 1, 1 };

  // Spawning Visualization Variables
#if WITH_EDITORONLY_DATA
  public:
    // Show the Yaw Visualizer for where the Raytraces will be aimed along the Yaw
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Spawning Visualization" )
    bool ShowYaw = true;

    // Show the Pitch Visualizer for where the Raytraces will be aimed along the Pitch
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Spawning Visualization" )
    bool ShowPitch = true;
#endif

  // Spawn Angle Variables
  public:
    // Starts at 0, set to 0 for no Yaw
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Spawn Angles", meta = ( ClampMin = -175.f, ClampMax = 175.f ) )
    int32 SpawnAngleYaw = 0;

    // Starts at 0, set to 0 for no Pitch
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Spawn Angles", meta = ( ClampMin = -175.f, ClampMax = 175.f ) )
    int32 SpawnAnglePitch = 0;

    // Starts at 0, set to 0 for no Inverse Yaw. While it starts at 0, the angle is inverted, so it really starts at -180
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Spawn Angles", meta = ( ClampMin = -175.f, ClampMax = 175.f ) )
    int32 SpawnAngleInverseYaw = 0;

    // Starts at 0, set to 0 for no Inverse Pitch. While it starts at 0, the angle is inverted, so it really starts at -180 
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Spawn Angles", meta = ( ClampMin = -175.f, ClampMax = 175.f ) )
    int32 SpawnAngleInversePitch = 0;

    // Will subtract 180 to get the StartSpawnAngleInverseYaw
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Spawn Angles", meta = ( ClampMin = -175.f, ClampMax = 175.f ) )
    int32 StartSpawnAngleYaw = 0;

    // Will subtract 180 to get the StartSpawnAngleInversePitch
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Spawn Angles", meta = ( ClampMin = -175.f, ClampMax = 175.f ) )
    int32 StartSpawnAnglePitch = 0;

  // Advanced Variables
  public:
    // How many times to retry the spawning of a mesh if a raycast does not hit
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Low Poly Spawner | Advanced", meta = ( ClampMin = 0 ) )
    int32 RetryCount = 100;

  private:
    void SpawnMeshes() NoExcept;
    FTransform GenerateTransform( float Rot, bool IsYaw ) const NoExcept;

  private:
    UPROPERTY()
    TArray< UInstancedStaticMeshComponent* > Instances; // TODO: Possibly add ability for each mesh instance to have its own options

#if WITH_EDITORONLY_DATA
  private:
    UPROPERTY()
    class UBillboardComponent *Billboard;

    UPROPERTY()
    class UBoxComponent *BoundingBox; // TODO: Add an option for a sphere to act as bounds instead.

    UPROPERTY()
    UInstancedStaticMeshComponent *YawVisualInstances;

    UPROPERTY()
    UInstancedStaticMeshComponent *InverseYawVisualInstances;

    UPROPERTY()
    UInstancedStaticMeshComponent *PitchVisualInstances;

    UPROPERTY()
    UInstancedStaticMeshComponent *InversePitchVisualInstances;

  private:
    Size_T MeshSize = 0;
#endif
};
