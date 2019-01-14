/*!------------------------------------------------------------------------------
\file   InteractionFileLoader.h

\author Garrett Conti

\par    Project: VIRIDIAN
\par    Course:  GAM300

\par    COPYRIGHT (C) 2018 BY DIGIPEN CORP, USA. ALL RIGHTS RESERVED.
------------------------------------------------------------------------------ */

#pragma once

// This must be first
#include "ObjectMacros.h"

// Unreal Includes
#include "Kismet/BlueprintFunctionLibrary.h"

// Our Includes
#include "Public/Utils/Macros.h"

// STL Includes

// This must be last
#include "InteractionFileLoader.generated.h"

USTRUCT( BlueprintType, Category = "Interactions" )
struct VIRIDIAN_API FInteractionText
{
  GENERATED_BODY()

  enum class ETextMarkup : int32
  {
    Bold = 'b',
    Italic = 'i',
    StrikeThrough = 's',
    Underline = 'u',
  };

  public:
    FInteractionText() NoExcept {}

    FInteractionText( FInteractionText &&Move ) NoExcept;

  public:
    void operator=( const FInteractionText &Copy ) NoExcept;
    void operator=(       FInteractionText &&Move ) NoExcept;

  public:
    static constexpr const char *const MarkupStrings[] = { "Bold", "Italic", "StrikeThrough", "Underline" };

    // Used to set bits for which markups a string contains
    static constexpr size_t Bold          = 1 << 0;
    static constexpr size_t Italic        = 1 << 1;
    static constexpr size_t StrikeThrough = 1 << 2;
    static constexpr size_t Underline     = 1 << 3;
    static constexpr size_t MaxMaskCount  = sizeof( MarkupStrings ) / sizeof( *MarkupStrings );

  public:  
    // Unreal does not have the ability to render a line of text with multiple fonts, so we do multiple renders.
    UPROPERTY( BlueprintReadOnly )
    TArray< FString > Text; // The text to render, split into multiple string to handle font changes for markups.

    UPROPERTY( BlueprintReadOnly )
    TArray< FString > TextMarkup; // Each index has a corresponding element in the Text array
};

UCLASS( Const )
class VIRIDIAN_API UInteractionFileLoader : public UBlueprintFunctionLibrary
{
  GENERATED_BODY()

  public:
    // Returns a struct that contains an array of text.
    // The struct has another array that determines any markups applied to each text element.
    UFUNCTION( BlueprintCallable, Category = "Interactions", meta = ( BlueprintThreadSafe ) )
    static TArray< FInteractionText > LoadInteractionFile( FName FileName ) NoExcept;
};
