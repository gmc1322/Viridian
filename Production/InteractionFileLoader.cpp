/*!------------------------------------------------------------------------------
\file   InteractionFileLoader.cpp

\author Garrett Conti

\par    Project: VIRIDIAN
\par    Course:  GAM300

\par    COPYRIGHT (C) 2018 BY DIGIPEN CORP, USA. ALL RIGHTS RESERVED.
------------------------------------------------------------------------------ */

#include "Public/InteractionText/InteractionFileLoader.h"
#include "Kismet/KismetSystemLibrary.h"

#include <fstream>

FInteractionText::FInteractionText( FInteractionText &&Move ) NoExcept : Text{ std::move( Move.Text ) }, TextMarkup{ std::move( Move.TextMarkup ) } {}

void FInteractionText::operator=( const FInteractionText &Copy ) NoExcept
{
  Text       = Copy.Text;
  TextMarkup = Copy.TextMarkup;
}

void FInteractionText::operator=( FInteractionText &&Move ) NoExcept
{
  Text       = std::move( Move.Text );
  TextMarkup = std::move( Move.TextMarkup );
}

TArray< FInteractionText > UInteractionFileLoader::LoadInteractionFile( const FName FileName ) NoExcept
{
  std::string FullText{}; // Wholes all of the text from the file

  // Load the file
  {
    std::ifstream InputFile{ *( UKismetSystemLibrary::GetProjectContentDirectory() + "InteractionTextFiles/" + FileName.GetPlainNameString() ) };

    DebugAssert( !( InputFile.is_open() ), "Unable to open file '%s'!", return TArray< FInteractionText >{},
                 *( UKismetSystemLibrary::GetProjectContentDirectory() + "InteractionTextFiles/" + FileName.GetPlainNameString() ) )

    // Get how large the file is
    InputFile.seekg( 0, std::ifstream::end );

    const size_t Size = static_cast< size_t >( InputFile.tellg() );

    // Resize the string to the exact size it needs to be
    FullText.resize( Size );

    InputFile.seekg( 0, std::ifstream::beg );

    // Load the entire file into the string
    InputFile.read( &( FullText.front() ), Size );

    InputFile.close();
  }

  // All of the blocks of text, each one has an array of text to render for that block and the markups for the text.
  TArray< FInteractionText > TextBlocks;

  size_t TextIndex = 0; // Our current index into the FullText
  
  for( FInteractionText TextBlock; ; )
  {
    // TODO: the sectioning character is technically hardcoded and could change, easy fix though (could be replace by a markup, or just a constexpr)

    const size_t NextIndex = FullText.find( '\n', TextIndex ); // We are sectioning blocks off by the newline

    // Check if there is any text left
    if( NextIndex == std::string::npos ) break;

    // Get the the block of text to check for markups
    {
      size_t PrevIndex = 0;
      
      const std::string Text = FullText.substr( TextIndex, NextIndex - TextIndex );

      // Start the markup loop
      {
        size_t Index = 0;

        // Find the first markup, if any
        while( ( Index = Text.find( '<', Index ) ) != std::string::npos )
        {
          if( Index ) // Make sure we have a normal string, the first part could be all marked up!
          {
            // Push back the not marked-up string
            TextBlock.Text.Emplace( Text.substr( PrevIndex, Index - PrevIndex ).c_str() ); // Conversion from char* to FString
            TextBlock.TextMarkup.Add( "Regular" );
          }

          int32 Bitmask = 0; // Used to know what markups are being applied to the strings

        // Get the markups for the string
        StartMarkup:
          do // There might be multiple markups
          {
            // TODO: Replace the switch with the more dynamic array version to make it easier to add/remove markups
            //       It's also faster but uses more memory

            switch( Text[ Index + 1 ] ) // Set the markup bits
            {
              case static_cast< int >( FInteractionText::ETextMarkup::Bold ):
                Bitmask |= FInteractionText::Bold;
                break;

              case static_cast< int >( FInteractionText::ETextMarkup::Italic ):
                Bitmask |= FInteractionText::Italic;
                break;

              case static_cast< int >( FInteractionText::ETextMarkup::StrikeThrough ):
                Bitmask |= FInteractionText::StrikeThrough;
                break;

              case static_cast< int >( FInteractionText::ETextMarkup::Underline ):
                Bitmask |= FInteractionText::Underline;
                break;

              default:
              {
                ANSICHAR AnsiName[ NAME_SIZE ];

                FileName.GetPlainANSIString( AnsiName );

                DebugLogType( "Unknown markup '%c' in the file '%s'!", Error, Text[ Index + 1 ], AnsiName );
              }
            }

            // TODO: Currently hardcoded, but would need to change a lot if the markups became more then two letters anyways. (The switch for example)
            Index += 3; // Move over to the next markup, each is three characters: <x>
          }
          while( Text[ Index ] == '<' ); // While there are markups side-by-side

        // Add the string and its markups
        StartString:
          PrevIndex = Index; // Save the index to get the substring

          Index = Text.find( '<', Index + 1 );

          // Push back the text with the current bitmask
          TextBlock.Text.Emplace( Text.substr( PrevIndex, Index - PrevIndex ).c_str() );

          // Parse through the set markups
          {
            FString FontMarkup;
            size_t i = 0;

            // Check to see which markups were set
            for( ; /* We know there is always one possible markup, or this file would be useless */ ; )
            {
              // Check if the text has a markup
              if( Bitmask & ( 1 << i ) ) 
              {
                // Add the first markup
                FontMarkup.Append( FInteractionText::MarkupStrings[ i ] );

                // Add the rest of the markups, this allows us to not add an extra space
                for( ; ++i < FInteractionText::MaxMaskCount; )
                {
                  if( Bitmask & ( 1 << i ) )
                  {
                    FontMarkup.Append( " " );
                    FontMarkup.Append( FInteractionText::MarkupStrings[ i ] );
                  }
                }

                break;
              }

              if( ++i == FInteractionText::MaxMaskCount ) break;
            }

            TextBlock.TextMarkup.Add( FontMarkup );
          }

          // Unset markups, jump to adding more markups, or jump to add more strings
          for( ; /* we know there is at least one bit set in the mask */ ; )
          {
            // If there is not an end markup, but another beginning we start over, adding to the current Bitmask
            // EXAMPLE: <B><I>ABCD<U>EFGH</U></B>IJKL</I>
            //          ABCD is bold and italic. EFGH is bold, italic, and underlined. IJKL is italic.

            if( Text[ Index + 1 ] != '/' ) goto StartMarkup; // Start parsing the markups again

            switch( Text[ Index + 2 ] ) // Unset the markup bits
            {
              case static_cast< int >( FInteractionText::ETextMarkup::Bold ) :
                Bitmask &= ~( FInteractionText::Bold );
                break;

              case static_cast< int >( FInteractionText::ETextMarkup::Italic ):
                Bitmask &= ~( FInteractionText::Italic );
                break;

              case static_cast< int >( FInteractionText::ETextMarkup::StrikeThrough ):
                Bitmask &= ~( FInteractionText::StrikeThrough );
                break;

              case static_cast< int >( FInteractionText::ETextMarkup::Underline ):
                Bitmask &= ~( FInteractionText::Underline );
                break;

              default:
              {
                ANSICHAR AnsiName[ NAME_SIZE ];

                FileName.GetPlainANSIString( AnsiName );

                DebugLogType( "Unknown markup '%c' in the file '%s'!", Error, Text[ Index + 1 ], AnsiName );
              }
            }

            Index += 4; // Move over to the next end markup, each one is four characters: </x>

            if( Bitmask ) // Loop while there are masks left, otherwise break and continue with a normal string. (No markups)
            {
              // If its not a markup character, its another string.
              if( Text[ Index ] != '<' ) goto StartString; // Go back and add it, with the current state of the Bitmask
            }
            else break;
          }

          PrevIndex = Index;
        } // Text while loop
      } // End markup scope

      // Add the last string to the text block
      {
        const std::string LastString = Text.substr( PrevIndex );

        // Check if there is any text left and push that back too
        if( LastString.size() ) // This makes sure we don't add an empty string
        {
          TextBlock.Text.Emplace( LastString.c_str() );
          TextBlock.TextMarkup.Add( "Regular" ); // It should not be marked up
        }
      }
    } // End text scope

    // Move the block, allowing us to not have to fully reconstruct another one
    TextBlocks.Add( std::move( TextBlock ) );

    TextIndex = NextIndex + 1; // Have to move over one character to not find it again
  } // TextBlock loop 

  return TextBlocks;
}

