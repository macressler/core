/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The Fixed Memory API
 *
 * Copyright 1999 Antonio Linares <alinares@fivetech.com>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version, with one exception:
 *
 * The exception is that if you link the Harbour Runtime Library (HRL)
 * and/or the Harbour Virtual Machine (HVM) with other files to produce
 * an executable, this does not by itself cause the resulting executable
 * to be covered by the GNU General Public License. Your use of that
 * executable is in no way restricted on account of linking the HRL
 * and/or HVM code into it.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA (or visit
 * their web site at http://www.gnu.org/).
 *
 */

/*
 * The following parts are Copyright of the individual authors.
 * www - http://www.harbour-project.org
 *
 * Copyright 1999 David G. Holm <dholm@jsd-llc.com>
 *    hb_xmemcpy()
 *    hb_xmemset()
 *
 * Copyright 1999 Victor Szel <info@szelvesz.hu>
 *    hb_xquery()
 *    HB_MEMORY()
 *
 * See doc/license.txt for licensing terms.
 *
 */

/* NOTE: If you turn this on, the memory subsystem will collect information
         about several statistical data about memory management, it will show
         these on exit if memory seem to have leaked.
         This should be normally turned off in a final release */
#define HB_FM_STATISTICS

/* NOTE: The following #include "hbwinapi.h" must
         be ahead of any other #include statements! */
#include "hbwinapi.h"

#ifndef __MPW__
   #include <malloc.h>
#endif

#include "extend.h"
#include "errorapi.h"
#include "hbmemory.ch"

typedef struct _HB_MEMINFO
{
   ULONG ulSize;
   ULONG ulProcLine;
   char  szProcName[ 256 ];
   struct _HB_MEMINFO * pPrevBlock;
   struct _HB_MEMINFO * pNextBlock;
} HB_MEMINFO, * PHB_MEMINFO;

#ifdef HB_FM_STATISTICS
static LONG s_lMemoryBlocks = 0;      /* memory blocks used */
static LONG s_lMemoryMaxBlocks = 0;   /* maximum number of used memory blocks */
static LONG s_lMemoryMaxConsumed = 0; /* memory size consumed */
static LONG s_lMemoryConsumed = 0;    /* memory max size consumed */
#endif

static PHB_MEMINFO pFirstBlock = NULL, pLastBlock = NULL;

void * hb_xalloc( ULONG ulSize )         /* allocates fixed memory, returns NULL on failure */
{
   void * pMem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xalloc(%lu)", ulSize));

   pMem = malloc( ulSize + sizeof( HB_MEMINFO ) );

   if( ! pMem )
      return pMem;

   if( ! pFirstBlock )
   {
      pFirstBlock = pMem;
      pLastBlock  = pMem;
      ( ( PHB_MEMINFO ) pMem )->pPrevBlock = NULL;
   }
   else
   {
      ( ( PHB_MEMINFO ) pMem )->pPrevBlock = pLastBlock;
      pLastBlock = pMem;
   }
   ( ( PHB_MEMINFO ) pMem )->pNextBlock = NULL;

   ( ( PHB_MEMINFO ) pMem )->ulSize = ulSize;  /* size of the memory block */
   if( hb_stack.pItems && ( hb_stack.pBase != hb_stack.pItems ) )
   {
      ( ( PHB_MEMINFO ) pMem )->ulProcLine = hb_stack.pBase->item.asSymbol.lineno; /* PRG line number */
      strcpy( ( ( PHB_MEMINFO ) pMem )->szProcName,
              hb_stack.pBase->item.asSymbol.value->szName ); /* PRG ProcName */
   }
   else
   {
      ( ( PHB_MEMINFO ) pMem )->ulProcLine = 0; /* PRG line number */
      strcpy( ( ( PHB_MEMINFO ) pMem )->szProcName, "" ); /* PRG ProcName */
   }

#ifdef HB_FM_STATISTICS
   s_lMemoryConsumed    += ulSize;
   if( s_lMemoryMaxConsumed < s_lMemoryConsumed )
      s_lMemoryMaxConsumed = s_lMemoryConsumed;
   s_lMemoryBlocks++;
   if( s_lMemoryMaxBlocks < s_lMemoryBlocks )
      s_lMemoryMaxBlocks = s_lMemoryBlocks;
#endif

   return ( char * ) pMem + sizeof( HB_MEMINFO );
}

void * hb_xgrab( ULONG ulSize )         /* allocates fixed memory, exits on failure */
{
   void * pMem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xgrab(%lu)", ulSize));

   pMem = malloc( ulSize + sizeof( HB_MEMINFO ) );

   if( ! pMem )
      hb_errInternal( 9999, "hb_xgrab can't allocate memory", NULL, NULL );

   if( ! pFirstBlock )
   {
      pFirstBlock = pMem;
      pLastBlock  = pMem;
      ( ( PHB_MEMINFO ) pMem )->pPrevBlock = NULL;
   }
   else
   {
      ( ( PHB_MEMINFO ) pMem )->pPrevBlock = pLastBlock;
      pLastBlock->pNextBlock = pMem;
      pLastBlock = pMem;
   }
   ( ( PHB_MEMINFO ) pMem )->pNextBlock = NULL;

   ( ( PHB_MEMINFO ) pMem )->ulSize = ulSize;  /* size of the memory block */
   if( hb_stack.pItems && ( hb_stack.pBase != hb_stack.pItems ) )
   {
      ( ( PHB_MEMINFO ) pMem )->ulProcLine = hb_stack.pBase->item.asSymbol.lineno; /* PRG line number */
      strcpy( ( ( PHB_MEMINFO ) pMem )->szProcName,
              hb_stack.pBase->item.asSymbol.value->szName ); /* PRG ProcName */
   }
   else
   {
      ( ( PHB_MEMINFO ) pMem )->ulProcLine = 0; /* PRG line number */
      strcpy( ( ( PHB_MEMINFO ) pMem )->szProcName, "" ); /* PRG ProcName */
   }

#ifdef HB_FM_STATISTICS
   s_lMemoryConsumed    += ulSize;
   if( s_lMemoryMaxConsumed < s_lMemoryConsumed )
      s_lMemoryMaxConsumed = s_lMemoryConsumed;
   s_lMemoryBlocks++;
   if( s_lMemoryMaxBlocks < s_lMemoryBlocks )
      s_lMemoryMaxBlocks = s_lMemoryBlocks;
#endif

   return ( char * ) pMem + sizeof( HB_MEMINFO );
}

static void DeleteNode( PHB_MEMINFO pMemBlock )
{
   if( pMemBlock->pPrevBlock )
     pMemBlock->pPrevBlock->pNextBlock = pMemBlock->pNextBlock;
   else
     pFirstBlock = pMemBlock->pNextBlock;

   if( pMemBlock->pNextBlock )
      pMemBlock->pNextBlock->pPrevBlock = pMemBlock->pPrevBlock;
   else
      pLastBlock = pMemBlock->pPrevBlock;
}

void * hb_xrealloc( void * pMem, ULONG ulSize )       /* reallocates memory */
{
   PHB_MEMINFO pMemBlock;
#ifdef HB_FM_STATISTICS
   ULONG ulMemSize;
#endif
   void * pResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_xrealloc(%p, %lu)", pMem, ulSize));

   pMemBlock = ( PHB_MEMINFO ) ( ( char * ) pMem - sizeof( HB_MEMINFO ) );

#ifdef HB_FM_STATISTICS
   ulMemSize = pMemBlock->ulSize;
#endif

   if( ulSize == 0 )
      DeleteNode( pMemBlock );
   else
   {
      pResult = realloc( pMemBlock, ulSize + sizeof( HB_MEMINFO ) );

      if( pResult )
      {
         ( ( PHB_MEMINFO ) pResult )->pPrevBlock = pMemBlock->pPrevBlock;
         ( ( PHB_MEMINFO ) pResult )->pNextBlock = pMemBlock->pNextBlock;
         if( pMemBlock->pPrevBlock )
            ( ( PHB_MEMINFO ) pResult )->pPrevBlock->pNextBlock = pResult;
         if( ( ( PHB_MEMINFO ) pResult )->pNextBlock )
            ( ( PHB_MEMINFO ) pResult )->pNextBlock->pPrevBlock = pResult;
      }
   }

   if( ! pResult )
      hb_errInternal( 9999, "hb_xrealloc can't reallocate memory", NULL, NULL );

   ( ( PHB_MEMINFO ) pResult )->ulSize = ulSize;  /* size of the memory block */

#ifdef HB_FM_STATISTICS
   if( ! ulSize )
      s_lMemoryBlocks--;

   s_lMemoryConsumed += ( ulSize - ulMemSize );
   if( s_lMemoryMaxConsumed < s_lMemoryConsumed )
      s_lMemoryMaxConsumed = s_lMemoryConsumed;
#endif

   return ( char * ) pResult + sizeof( HB_MEMINFO );
}

void hb_xfree( void * pMem )            /* frees fixed memory */
{
   PHB_MEMINFO pMemBlock;

   HB_TRACE(HB_TR_DEBUG, ("hb_xfree(%p)", pMem));

   if( pMem )
   {
#ifdef HB_FM_STATISTICS
      s_lMemoryConsumed -= ( ( PHB_MEMINFO ) ( ( char * ) pMem - sizeof( HB_MEMINFO ) ) )->ulSize;
      s_lMemoryBlocks--;
#endif

      pMemBlock = ( PHB_MEMINFO ) ( ( char * ) pMem - sizeof( HB_MEMINFO ) );
      DeleteNode( pMemBlock );
      if( pMemBlock )
         free( ( void * ) pMemBlock );
   }
   else
      hb_errInternal( 9999, "hb_xfree called with a null pointer", NULL, NULL );
}

ULONG hb_xsize( void * pMem ) /* returns the size of an allocated memory block */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xsize(%p)", pMem));

   return ( ( PHB_MEMINFO ) ( ( char * ) pMem - sizeof( HB_MEMINFO ) ) )->ulSize;
}

void hb_xinit( void ) /* Initialize fixed memory subsystem */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xinit()"));
}

void hb_xexit( void ) /* Deinitialize fixed memory subsystem */
{
   PHB_MEMINFO pMemBlock = pFirstBlock;

   HB_TRACE(HB_TR_DEBUG, ("hb_xexit()"));

#ifdef HB_FM_STATISTICS
   if( s_lMemoryBlocks || hb_cmdargCheck( "INFO" ) )
   {
      char buffer[ 100 ];

      hb_outerr( hb_consoleGetNewLine(), 0 );
      hb_outerr( "----------------------------------------", 0 );
      hb_outerr( hb_consoleGetNewLine(), 0 );
      sprintf( buffer, "Total memory allocated: %li bytes (%li blocks)", s_lMemoryMaxConsumed, s_lMemoryMaxBlocks );
      hb_outerr( buffer, 0 );

      if( s_lMemoryBlocks )
      {
         hb_outerr( hb_consoleGetNewLine(), 0 );
         sprintf( buffer, "WARNING! Memory allocated but not released: %li bytes (%li blocks)", s_lMemoryConsumed, s_lMemoryBlocks );
         hb_outerr( buffer, 0 );
      }

      if( pMemBlock )
      {
         unsigned int ui = 1;

         do
         {
            sprintf( buffer, "\nblock %i: %s line %i", ui++, pMemBlock->szProcName,
                     pMemBlock->ulProcLine );
            HB_TRACE( HB_TR_DEBUG, (buffer));
            pMemBlock = pMemBlock->pNextBlock;
         } while( pMemBlock );
      }
   }
#endif
}

#if UINT_MAX != ULONG_MAX

/* hb_xmemcpy and hb_xmemset are only needed when
   unsigned int and unsigned long differ in length */

void * hb_xmemcpy( void * pDestArg, void * pSourceArg, ULONG ulLen )
{
   BYTE * pDest;
   BYTE * pSource;
   ULONG  ulRemaining;
   int    iCopySize;

   HB_TRACE(HB_TR_DEBUG, ("hb_xmemcpy(%p, %p, %lu)", pDestArg, pSourceArg, ulLen));

   pDest = ( BYTE * ) pDestArg;
   pSource = ( BYTE * ) pSourceArg;
   ulRemaining = ulLen;

   while( ulRemaining )
   {
      /* Overcome the memcpy() size_t limitation */
      if( ulRemaining > UINT_MAX )
      {
         iCopySize = UINT_MAX;
         ulRemaining -= ( ULONG ) iCopySize;
      }
      else
      {
         iCopySize = ( int ) ulRemaining;
         ulRemaining = 0;
      }
      memcpy( pDest, pSource, iCopySize );
      pDest += iCopySize;
      pSource += iCopySize;
   }
   return pDestArg;
}

void * hb_xmemset( void * pDestArg, int iFill, ULONG ulLen )
{
   BYTE * pDest;
   ULONG  ulRemaining;
   int    iSetSize;

   HB_TRACE(HB_TR_DEBUG, ("hb_xmemset(%p, %d, %lu)", pDestArg, iFill, ulLen));

   pDest = ( BYTE * ) pDestArg;
   ulRemaining = ulLen;

   while( ulRemaining )
   {
      /* Overcome the memset() size_t limitation */
      if( ulRemaining > UINT_MAX )
      {
         iSetSize = UINT_MAX;
         ulRemaining -= ( ULONG ) iSetSize;
      }
      else
      {
         iSetSize = ( int ) ulRemaining;
         ulRemaining = 0;
      }
      memset( pDest, iFill, iSetSize );
      pDest += iSetSize;
   }
   return pDestArg;
}

#endif

ULONG hb_xquery( USHORT uiMode )
{
   ULONG ulResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_xquery(%hu)", uiMode));

   /* TODO: Return the correct values instead of 9999 [vszel] */

   switch( uiMode )
   {
   case HB_MEM_CHAR:       /*               (Free Variable Space [KB])          */
      #if defined(_Windows) || defined(WINNT)
      {
         MEMORYSTATUS memorystatus;
         GlobalMemoryStatus( &memorystatus );
         ulResult = memorystatus.dwAvailPhys / 1024;
      }
      #else
         ulResult = 9999;
      #endif
      break;

   case HB_MEM_BLOCK:      /*               (Largest String [KB])               */
      #if defined(_Windows) || defined(WINNT)
      {
         MEMORYSTATUS memorystatus;
         GlobalMemoryStatus( &memorystatus );
         ulResult = HB_MIN_( memorystatus.dwAvailPhys, ULONG_MAX ) / 1024;
      }
      #else
         ulResult = 9999;
      #endif
      break;

   case HB_MEM_RUN:        /*               (RUN Memory [KB])                   */
      #if defined(_Windows) || defined(WINNT)
      {
         MEMORYSTATUS memorystatus;
         GlobalMemoryStatus( &memorystatus );
         ulResult = memorystatus.dwAvailPhys / 1024;
      }
      #else
         ulResult = 9999;
      #endif
      break;

   case HB_MEM_VM:         /* UNDOCUMENTED! (Virtual Memory [KB])               */
      #if defined(_Windows) || defined(WINNT)
      {
         MEMORYSTATUS memorystatus;
         GlobalMemoryStatus( &memorystatus );
         ulResult = memorystatus.dwAvailVirtual / 1024;
      }
      #else
         ulResult = 9999;
      #endif
      break;

   case HB_MEM_EMS:        /* UNDOCUMENTED! (Free Expanded Memory [KB]) (?)     */
      #if defined(_Windows) || defined(WINNT)
         ulResult = 0;
      #else
         ulResult = 9999;
      #endif
      break;

   case HB_MEM_FM:         /* UNDOCUMENTED! (Fixed Memory/Heap [KB]) (?)        */
      #if defined(_Windows) || defined(WINNT)
      {
         MEMORYSTATUS memorystatus;
         GlobalMemoryStatus( &memorystatus );
         ulResult = memorystatus.dwTotalPhys / 1024;
      }
      #else
         ulResult = 9999;
      #endif
      break;

   case HB_MEM_FMSEGS:     /* UNDOCUMENTED! (Segments in Fixed Memory/Heap) (?) */
      #if defined(_Windows) || defined(WINNT)
         ulResult = 1;
      #else
         ulResult = 9999;
      #endif
      break;

   case HB_MEM_SWAP:       /* UNDOCUMENTED! (Free Swap Memory [KB])             */
      #if defined(_Windows) || defined(WINNT)
      {
         MEMORYSTATUS memorystatus;
         GlobalMemoryStatus( &memorystatus );
         ulResult = memorystatus.dwAvailPageFile / 1024;
      }
      #else
         ulResult = 9999;
      #endif
      break;

   case HB_MEM_CONV:       /* UNDOCUMENTED! (Free Conventional [KB])            */
      #if defined(_Windows) || defined(WINNT)
         ulResult = 0;
      #else
         ulResult = 9999;
      #endif
      break;

   case HB_MEM_EMSUSED:    /* UNDOCUMENTED! (Used Expanded Memory [KB]) (?)     */
      ulResult = 0;
      break;

   case HB_MEM_USED:       /* Harbour extension (Memory used [bytes])           */
#ifdef HB_FM_STATISTICS
      ulResult = s_lMemoryConsumed;
#else
      ulResult = 0;
#endif
      break;

   case HB_MEM_USEDMAX:    /* Harbour extension (Maximum memory used [bytes])   */
#ifdef HB_FM_STATISTICS
      ulResult = s_lMemoryMaxConsumed;
#else
      ulResult = 0;
#endif
      break;

   case HB_MEM_STACKITEMS: /* Harbour extension (Total items on the stack)      */
      ulResult = hb_stack.wItems;
      break;

   case HB_MEM_STACK:      /* Harbour extension (Total memory size used by the stack [bytes]) */
      ulResult = hb_stack.wItems * sizeof( HB_ITEM );
      break;

   default:
      ulResult = 0;
   }

   return ulResult;
}

HARBOUR HB_MEMORY( void )
{
   hb_retnl( hb_xquery( hb_parni( 1 ) ) );
}