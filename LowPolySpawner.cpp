/*!------------------------------------------------------------------------------
\file   LowPolySpawner.h

\author Garrett Conti

\par    Project: VIRIDIAN
\par     Course: GAM300

\par    COPYRIGHT (C) 2018 BY DIGIPEN CORP, USA. ALL RIGHTS RESERVED.
------------------------------------------------------------------------------ */

#include  "LowPolySpawner.h"

// Unreal Includes
#include "Components/InstancedStaticMeshComponent.h"

#if WITH_EDITOR
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "ConstructorHelpers.h" // Mesh loader
#endif

#include "Kismet/KismetMathLibrary.h" // MakeRotFromX, Lerp, Abs, RandomIntegerInRangeFromStream

#include "Classes/Engine/StaticMesh.h"

// Our Includes
#include "RandUtils.h" // RandomVector_InRange_FromStream

#if WITH_EDITOR
ALowPolySpawner::ALowPolySpawner() NoExcept :
Billboard( CreateDefaultSubobject< UBillboardComponent >( TEXT( "Billboard" ) ) ),
BoundingBox( CreateDefaultSubobject< UBoxComponent >( TEXT( "BoundingBox" ) ) ),

       YawVisualInstances( CreateDefaultSubobject< UInstancedStaticMeshComponent >( TEXT(        "YawVisualInstances" ) ) ),
InverseYawVisualInstances( CreateDefaultSubobject< UInstancedStaticMeshComponent >( TEXT( "InverseYawVisualInstances" ) ) ),

       PitchVisualInstances( CreateDefaultSubobject< UInstancedStaticMeshComponent >( TEXT(        "PitchVisualInstances" ) ) ),
InversePitchVisualInstances( CreateDefaultSubobject< UInstancedStaticMeshComponent >( TEXT( "InversePitchVisualInstances" ) ) )
#else
ALowPolySpawner::ALowPolySpawner() NoExcept
#endif
{
  PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITOR
  RootComponent = Billboard;

  BoundingBox->SetupAttachment( Billboard );

         YawVisualInstances->SetupAttachment( Billboard );
  InverseYawVisualInstances->SetupAttachment( Billboard );

         PitchVisualInstances->SetupAttachment( Billboard );
  InversePitchVisualInstances->SetupAttachment( Billboard );

  { // Allow for a custom mesh to be used as the Visualizer
    const ConstructorHelpers::FObjectFinder< UStaticMesh > Mesh{ TEXT( "StaticMesh'/Game/Meshes/LowPolyVisualizer.LowPolyVisualizer'" ) };

    if( Mesh.Succeeded() )
    {
      // Get the mesh's Y size to stop overlapping, this assumes the Y and Z are uniform, X does not have to be, but the program assumes X is 1
      MeshSize = static_cast< Size_T >( Mesh.Object->GetBoundingBox().GetExtent().Y * 2 ); // Mult by 2 since its the half boundry of the box

             YawVisualInstances->SetStaticMesh( Mesh.Object );
      InverseYawVisualInstances->SetStaticMesh( Mesh.Object );

             PitchVisualInstances->SetStaticMesh( Mesh.Object );
      InversePitchVisualInstances->SetStaticMesh( Mesh.Object );

             PitchVisualInstances->AddInstance( GenerateTransform( 0.f, false ) );
      InversePitchVisualInstances->AddInstance( GenerateTransform( -180.f, false ) );
    }
    else DebugLogType( "The LowPolyVisualizer mesh is missing from the folder '/Content/Meshes'!", Warning );
  }
#endif
}

// Used to make sure no Visualizer Instance meshes overlap
static constexpr int32 RoundToMultiple( int32 Round, uint32 Multiple ) NoExcept
{
  return Round < 0 ? -static_cast< int32 >( ( -Round + Multiple / 2 ) / Multiple * Multiple ) :
                                            (  Round + Multiple / 2 ) / Multiple * Multiple   ; // RVO
}

#if WITH_EDITOR
void ALowPolySpawner::PostEditChangeProperty( FPropertyChangedEvent &PropertyChangedEvent ) NoExcept 
{
  Super::PostEditChangeProperty( PropertyChangedEvent );

  if( PropertyChangedEvent.MemberProperty ) // Possible for MemberProperty to be null! (I've had it happen)
  {
    const FString &ChangedProperty = PropertyChangedEvent.MemberProperty->GetNameCPP();

    // Go through each character in the changed property name instead of string compare for more efficiency
    // This is possibly the most confusion part of the code just becouse of how much it does!

    const char FirstChar = ChangedProperty[ 0 ];

    if( FirstChar == 'G' ) // Generat...
    {  
      if( ChangedProperty[ 7 ] == 'a' ) // GeneratableMeshes
      {
        // TODO: Could be change to only clear when less meshes, and add when more, instead of always re-creating array

        // We add/removed Instance Meshes, clear array
        Instances.Empty( Instances.Num() );
      }
      else // GenerateMeshes
      {
        GenerateMeshes = false;

        // Create Instance components if they don't exits or have been destroyed
        if( Instances.Num() == 0 || !( Instances[ 0 ] ) )
        {
          Instances.Empty( Instances.Num() );

          for( UStaticMesh *const Iter : GeneratableMeshes )
          {
            auto *const Instance = NewObject< UInstancedStaticMeshComponent >( this );

            Instances.Emplace( Instance );

            Instance->SetStaticMesh( Iter );

            Instance->AttachToComponent( RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale );
          }
        }
        else for( auto &Iter : Instances ) // Otherwise, clear existing instances
        {
          Iter->ClearInstances();
        }

        SpawnMeshes();
      }
    }
    else // S...
    {
      const auto SpawnYaw = [ this ]()NoExcept->void
      {
        YawVisualInstances->ClearInstances();

        if( StartSpawnAnglePitch || StartSpawnAngleYaw )
        {
          YawVisualInstances->AddInstance( GenerateTransform( static_cast< float >( StartSpawnAngleYaw ), true ) );
        }

        if( SpawnAngleYaw % MeshSize ) SpawnAngleYaw = RoundToMultiple( SpawnAngleYaw, MeshSize );

        if( SpawnAngleYaw >= 0 )
        {
          // Make sure to not overlap
          if( SpawnAngleInverseYaw - SpawnAngleYaw <= -180 ) SpawnAngleYaw = 175 + SpawnAngleInverseYaw;

          for( int32 i = 5; i <= SpawnAngleYaw; i += 5 )
          {
            YawVisualInstances->AddInstance( GenerateTransform( static_cast< float >( i + StartSpawnAngleYaw ), true ) );
          }
        }
        else
        {
          // Make sure to not overlap
          if( SpawnAngleInverseYaw - SpawnAngleYaw >= 180 ) SpawnAngleYaw = -175 + SpawnAngleInverseYaw;

          for( int32 i = -5; i >= SpawnAngleYaw; i -= 5 )
          {
            YawVisualInstances->AddInstance( GenerateTransform( static_cast< float >( i + StartSpawnAngleYaw ), true ) );
          }
        }
      };

      const auto SpawnPitch = [ this ]()NoExcept->void
      {
        PitchVisualInstances->ClearInstances();

        PitchVisualInstances->AddInstance( GenerateTransform( static_cast< float >( StartSpawnAnglePitch ), false ) );

        if( SpawnAnglePitch % MeshSize ) SpawnAnglePitch = RoundToMultiple( SpawnAnglePitch, MeshSize );
        
        if( SpawnAnglePitch >= 0 )
        {
          // Make sure to not overlap
          if( SpawnAngleInversePitch - SpawnAnglePitch <= -180 ) SpawnAnglePitch = 175 + SpawnAngleInversePitch;

          for( int32 i = 5; i <= SpawnAnglePitch; i += 5 )
          {
            PitchVisualInstances->AddInstance( GenerateTransform( static_cast< float >( i + StartSpawnAnglePitch ), false ) );
          }
        }
        else
        {
          // Make sure to not overlap
          if( SpawnAngleInversePitch - SpawnAnglePitch >= 180 ) SpawnAnglePitch = -175 + SpawnAngleInversePitch;

          for( int32 i = -5; i >= SpawnAnglePitch; i -= 5 )
          {
            PitchVisualInstances->AddInstance( GenerateTransform( static_cast< float >( i + StartSpawnAnglePitch ), false ) );
          }
        }
      };

      const auto SpawnInverseYaw = [ this ]()NoExcept->void
      {
        InverseYawVisualInstances->ClearInstances();

        if( StartSpawnAnglePitch || StartSpawnAngleYaw )
        {
          InverseYawVisualInstances->AddInstance( GenerateTransform( -180.f + StartSpawnAngleYaw, true ) );
        }

        if( SpawnAngleInverseYaw % MeshSize ) SpawnAngleInverseYaw = RoundToMultiple( SpawnAngleInverseYaw, MeshSize );
        
        if( SpawnAngleInverseYaw >= 0 )
        {
          // Make sure to not overlap
          if( SpawnAngleYaw - SpawnAngleInverseYaw < -180 ) SpawnAngleInverseYaw = 175 + SpawnAngleYaw;

          for( int32 i = 5; i <= SpawnAngleInverseYaw; i += 5 )
          {
            InverseYawVisualInstances->AddInstance( GenerateTransform( -180.f + i + StartSpawnAngleYaw, true ) );
          }
        }
        else
        {
          // Make sure to not overlap
          if( SpawnAngleYaw - SpawnAngleInverseYaw > 180 ) SpawnAngleInverseYaw = -175 + SpawnAngleYaw;

          for( int32 i = -5; i >= SpawnAngleInverseYaw; i -= 5 )
          {
            InverseYawVisualInstances->AddInstance( GenerateTransform( -180.f + i + StartSpawnAngleYaw, true ) );
          }
        }
      };

      const auto SpawnInversePitch = [ this ]()NoExcept->void
      {
        InversePitchVisualInstances->ClearInstances();

        InversePitchVisualInstances->AddInstance( GenerateTransform( -180.f + StartSpawnAnglePitch, false ) );

        if( SpawnAngleInversePitch % MeshSize ) SpawnAngleInversePitch = RoundToMultiple( SpawnAngleInversePitch, MeshSize );
        
        if( SpawnAngleInversePitch >= 0 )
        {
          // Make sure to not overlap
          if( SpawnAnglePitch - SpawnAngleInversePitch < -180 ) SpawnAngleInversePitch = 175 + SpawnAnglePitch;

          for( int32 i = 5; i <= SpawnAngleInversePitch; i += 5 )
          {
            InversePitchVisualInstances->AddInstance( GenerateTransform( -180.f + i + StartSpawnAnglePitch, false ) );
          }
        }
        else
        {
          // Make sure to not overlap
          if( SpawnAnglePitch - SpawnAngleInversePitch > 180 ) SpawnAngleInversePitch = -175 + SpawnAnglePitch;

          for( int32 i = -5; i >= SpawnAngleInversePitch; i -= 5 )
          {
            InversePitchVisualInstances->AddInstance( GenerateTransform( -180.f + i + StartSpawnAnglePitch, false ) );
          }
        } 
      };

      const char SecondChar = ChangedProperty[ 1 ];

      if( SecondChar == 'p' ) // SpawnA...
      {
        if( ChangedProperty[ 6 ] == 'r' ) // SpawnArea
        {
          // Set the spawn area bounding box to see where meshes could be spawned
          BoundingBox->SetBoxExtent( SpawnArea, false );
        }
        else // SpawnAngle...
        {
          const char TenthChar = ChangedProperty[ 10 ];

          if( TenthChar == 'Y' ) // SpawnAngleYaw
          {
            SpawnYaw();
          }
          else if( TenthChar == 'P' ) // SpawnAnglePitch
          {
            SpawnPitch();
          }
          else  // SpawnAngleInverse
          {
            const char SeventeenthChar = ChangedProperty[ 17 ];

            if( SeventeenthChar == 'Y' ) // SpawnAngleInverseYaw
            {
              SpawnInverseYaw();
            }
            else // SpawnAngleInversePitch
            {
              SpawnInversePitch();
            }
          }
        }
      }
      else if( SecondChar == 't' ) // StartSpawn
      {
        if( ChangedProperty[ 15 ] == 'P' ) // StartSpawnAnglePitch
        {
          if( StartSpawnAnglePitch % MeshSize ) StartSpawnAnglePitch = RoundToMultiple( StartSpawnAnglePitch, MeshSize );

          if( ShowPitch )
          {
            SpawnPitch();
            SpawnInversePitch();
          }

          if( ShowYaw )
          {
            SpawnYaw();
            SpawnInverseYaw();
          }
        }
        else // StartSpawnAngleYaw
        {
          if( StartSpawnAngleYaw % MeshSize ) StartSpawnAngleYaw = RoundToMultiple( StartSpawnAngleYaw, MeshSize );

          if( ShowYaw )
          {
            SpawnYaw();
            SpawnInverseYaw();
          }

          if( ShowPitch )
          {
            SpawnPitch();
            SpawnInversePitch();
          }
        }
      }
      else // Show...
      {
        const char FourthChar = ChangedProperty[ 4 ];

        if( FourthChar == 'Y' ) // ShowYaw
        {
          YawVisualInstances->ClearInstances();
          InverseYawVisualInstances->ClearInstances();
        }
        else if( FourthChar == 'P' ) // ShowPitch
        {
          PitchVisualInstances->ClearInstances();
          InversePitchVisualInstances->ClearInstances();

          if( ShowPitch ) 
          { 
            PitchVisualInstances->AddInstance( GenerateTransform( static_cast< float >( StartSpawnAnglePitch ), false ) );

            InversePitchVisualInstances->AddInstance( GenerateTransform( -180.f + StartSpawnAnglePitch, false ) );
          }
        }
      }
    }
  }
}
#else
void ALowPolySpawner::BeginPlay() NoExcept
{
  Super::BeginPlay();

  // Always generate Instance components, this should be the first and only time in shipping mode

  // Create first component to be made Root
  {
    auto *const Instance = NewObject< UInstancedStaticMeshComponent >();

    Instances.Emplace( Instance );

    Instance->SetStaticMesh( GeneratableMeshes[ 0 ] );

    RootComponent = Instance;
  }

  int32 i = 1;

  // Create the rest of the components
  for( const int32 Num = GeneratableMeshes.Num(); i < Num; ++i )
  {
    auto *const Instance = NewObject< UInstancedStaticMeshComponent >();

    Instances.Emplace( Instance );

    Instance->SetStaticMesh( GeneratableMeshes[ i ] );

    Instance->AttachToComponent( RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale );
  }

  SpawnMeshes();
}
#endif

// Created a macro since it runs diffrent spawn code and I did not want to use function ptrs for something this small
#define MainSpawnLoop( SpawnCode )                                                                                          \
for( int32 i = 0; i < SpawnCount; ++i )                                                                                     \
{                                                                                                                           \
  for( int32 j = 0; ; ) /* Try x times, then quit */                                                                        \
  {                                                                                                                         \
    const FVector SpawnLoc{ URandUtils::RandomPointInBoundingBox_FromStream( ActorLoc, SpawnArea, TempStream ) };           \
                                                                                                                            \
    FHitResult Hit;                                                                                                         \
                                                                                                                            \
    if( World->LineTraceSingleByChannel( Hit, SpawnLoc, FVector{ SpawnLoc.X, SpawnLoc.Y, EndZ }, ECC_WorldStatic,           \
                                         CollisionParams, FCollisionResponseParams::DefaultResponseParam ) )                \
    {                                                                                                                       \
      SpawnCode                                                                                                             \
    }                                                                                                                       \
                                                                                                                            \
    if( ++j >= RetryCount )                                                                                                 \
    {                                                                                                                       \
      DebugLogType( "A LowPolySpawner was unable to register a hit, to spawn a mesh, after trying %i times!", Warning, j ); \
                                                                                                                            \
      break;                                                                                                                \
    }                                                                                                                       \
  }                                                                                                                         \
}                                                                                                                           \

#define SpawnWithOutScale                                                                   \
Instances[ UKismetMathLibrary::RandomIntegerInRangeFromStream( 0, Num, TempStream ) ]->     \
  AddInstanceWorldSpace( { UKismetMathLibrary::MakeRotFromX( Hit.Normal ), Hit.Location } ) \

#define SpawnWithScale                                                                                       \
Instances[ UKismetMathLibrary::RandomIntegerInRangeFromStream( 0, Num, TempStream ) ]->                      \
  AddInstanceWorldSpace( { UKismetMathLibrary::MakeRotFromX( Hit.Normal ), Hit.Location,                     \
                           URandUtils::RandomVector_InRange_FromStream( MinScale, MaxScale, TempStream ) } ) \

void ALowPolySpawner::SpawnMeshes() NoExcept
{
  // INITIALIZING const vars

  TArray< TEnumAsByte< EObjectTypeQuery > > CollidableObjects;

  CollidableObjects.Emplace( ECC_WorldStatic );

  UWorld *const World = GetWorld();

  const FCollisionQueryParams CollisionParams{ FName{ "LPS" }, false, this }; // Custom params to not overlap with ourselves


  const Size_T Num = Instances.Num() - 1; // Sub 1, we use Num to index into the array

  const FRandomStream TempStream{ RandomSeed };

  const FVector &ActorLoc = GetActorLocation();

  //-- INITIALIZING const vars
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // One dir                    = rand point, move in dir
  // Two plus dir, without gaps = rand point, move in rand dir on blended axes
  // Two plus dir, with gaps    = rand point, move in rand dir on non-blended axis

  // Generate Random 3D vector based on XYZ angle, XY use actual angle, Z uses range from -1 to 1 (if both pos and neg)
  // Pos X to Pos Y = 0->90, Pos XY to Neg X = 0->180, Pos X to Neg YX = (-180)->0, Pos XY to Neg XY = 0->360
  // Pos Z = 0->1, Neg Z = (-1)->0, Pos Z to Neg Z = (-1)->1
  // x = sqrt( 1-Z^2 ) * cos( Yaw ), y = sqrt( 1-Z^2 ) * sin( Yaw ) z = Z

  /*int32 Yaw;
  int32 Pitch;

  // TODO: Add in new angle offsets and inverts
  if( SpawnAngleYaw )
  {
    if( SpawnAnglePitch )
    {
      if( SpawnAngleYaw > 0 )
      {
        Yaw = UKismetMathLibrary::RandomIntegerInRangeFromStream( 0, SpawnAngleYaw, TempStream );
      }
      else
      {
        Yaw = UKismetMathLibrary::RandomIntegerInRangeFromStream( SpawnAngleYaw, 0, TempStream );
      }

      // Convert to -1 -> 1
      if( SpawnAnglePitch > 0 )
      {
        // If larger, we are adding negative
        if( SpawnAnglePitch > 180 )
        {
          Pitch = UKismetMathLibrary::RandomIntegerInRangeFromStream( ( SpawnAnglePitch - 180 ) / -180, 1, TempStream );
        }
        else
        {
          Pitch = UKismetMathLibrary::RandomIntegerInRangeFromStream( 0, SpawnAnglePitch / 180, TempStream );
        }
      }
      else
      {
        // If larger, we are adding positive
        if( SpawnAnglePitch < -180 )
        {
          Pitch = UKismetMathLibrary::RandomIntegerInRangeFromStream( -1, ( SpawnAnglePitch + 180 ) / -180, TempStream );
        }
        else
        {
          Pitch = UKismetMathLibrary::RandomIntegerInRangeFromStream( SpawnAnglePitch / 180.f, 0, TempStream );
        }
      }
    }
    else 
    {
      Pitch = 0;

      if( SpawnAngleYaw > 0 )
      {
        Yaw = UKismetMathLibrary::RandomIntegerInRangeFromStream( 0, SpawnAngleYaw, TempStream );
      }
      else
      {
        Yaw = UKismetMathLibrary::RandomIntegerInRangeFromStream( SpawnAngleYaw, 0, TempStream );
      }
    }
  }
  else 
  {
    Yaw = 0;

    if( SpawnAnglePitch > 0 )
    {
      Pitch = UKismetMathLibrary::RandomIntegerInRangeFromStream( 0, SpawnAnglePitch, TempStream );
    }
    else if( SpawnAnglePitch < 0 )
    {
      Pitch = UKismetMathLibrary::RandomIntegerInRangeFromStream( SpawnAnglePitch, 0, TempStream );
    }
    else Pitch = 0;
  }*/

  // TODO: Replace with dynamic angle code
  const float EndZ = ActorLoc.Z - SpawnArea.Z;

  // TODO: Add option for no instance mesh overlaps
  if( RandomScale )
  {
    if( AllowGroundOverlap )
    {
      MainSpawnLoop(
        // TODO: Add option to rotate the scale based on the object's rotation.
        //       So if the object rotates upword, but normally faces towards the x axis, z will scale it upwards instead of x.
        SpawnWithScale; break;
      )
    }
    else
    {
      MainSpawnLoop(
        // TODO: Should we re-generate the vector or just move it up?
        if( !( Hit.bStartPenetrating ) ) // If we did not start the trace inside an object
        {
          SpawnWithScale; break;
        }
      )
    }
  }
  else if( AllowGroundOverlap )
  {
    MainSpawnLoop( SpawnWithOutScale; break; );
  }
  else
  {
    MainSpawnLoop( 
      // TODO: Should we re-generate the vector or just move it up?
      if( !( Hit.bStartPenetrating ) ) // If we did not start the trace inside an object
      {
        SpawnWithOutScale; break;
      }
    )
  }
}

// BUG: Currently stretches scale
FTransform ALowPolySpawner::GenerateTransform( const float Rot, const bool IsYaw ) const NoExcept
{
  FTransform Return;

  if( IsYaw )
  {
    Return.SetRotation( FRotator{ 0.f, Rot, 0.f }.Quaternion() );

    Return.SetScale3D( { UKismetMathLibrary::Lerp( SpawnArea.X, SpawnArea.Y, UKismetMathLibrary::Abs( Rot ) / 90.f ), 1.f, 1.f } );
  }
  else
  {
    Return.SetRotation( FRotator{ Rot, 0.f , 0.f }.Quaternion() );

    Return.SetScale3D( { UKismetMathLibrary::Lerp( SpawnArea.X, SpawnArea.Z, UKismetMathLibrary::Abs( Rot ) / 90.f ), 1.f, 1.f } );
  }

  return Return; // NRVO
}
