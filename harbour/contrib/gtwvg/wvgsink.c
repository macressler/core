/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Source file for the Wvg*Classes
 *
 * Copyright 2008 Andy Wos
 * http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */
/*----------------------------------------------------------------------*/
/*
 *                     Active-X Interface Functions
 *
 *                       Contributed by Andy Wos
 *                   A little tweaked by Pritpal Bedi
 */
/*----------------------------------------------------------------------*/

#include "hbvmint.h"
#include "hbwinole.h"

/*----------------------------------------------------------------------*/

#if defined(__BORLANDC__) && !defined(HB_ARCH_64BIT)
    #undef MAKELONG
    #define MAKELONG(a,b) ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | \
                          (((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16)))
#endif

#if defined( HB_OS_WIN_CE )
   #define HBTEXT( x ) TEXT( x )
#else
   #define HBTEXT( x ) x
#endif

/*----------------------------------------------------------------------*/

static HMODULE  s_hLib = NULL;

typedef BOOL    ( CALLBACK *PATLAXWININIT )( void );
typedef BOOL    ( CALLBACK *PATLAXWINTERM )( void );
typedef HRESULT ( CALLBACK *PATLAXGETCONTROL )( HWND, IUnknown** );
typedef HRESULT ( CALLBACK *PATLAXATTACHCONTROL )( HWND, IUnknown** );
typedef HRESULT ( CALLBACK *PATLAXCREATECONTROL )( LPCOLESTR, HWND, IStream*, IUnknown** );
typedef HRESULT ( CALLBACK *PATLAXCREATECONTROLEX )( LPCOLESTR, HWND, IStream*, IUnknown**, IUnknown**, REFIID, IUnknown* );

/*----------------------------------------------------------------------*/

#if 0
#define __HBTOOUT__
#endif

#ifdef __HBTOOUT__
void extern hb_ToOutDebug( const char * sTraceMsg, ... );
#endif

/*----------------------------------------------------------------------*/

#if !defined( StringCchCat )
   #ifdef UNICODE
      #define StringCchCat(d,n,s)   hb_wcncpy( (d), (s), (n) - 1 )
   #else
      #define StringCchCat(d,n,s)   hb_xstrncpy( (d), (s), (n) - 1 )
   #endif
#endif

/*
 * This function copies szText to destination buffer.
 * NOTE: Unlike the documentation for strncpy, this routine will always append
 *       a null
 */
char * hb_xstrncpy( char * pDest, const char * pSource, ULONG ulLen )
{
   ULONG ulDst, ulSrc;

   pDest[ ulLen ] = 0;
   ulDst = strlen( pDest );
   if( ulDst < ulLen )
   {
      ulSrc = strlen( pSource );
      if( ulDst + ulSrc > ulLen )
         ulSrc = ulLen - ulDst;

      memcpy( &pDest[ ulDst ], pSource, ulSrc );
      pDest[ ulDst + ulSrc ] = 0;
   }
   return pDest;
}


wchar_t *hb_wcncpy( wchar_t *dstW, const wchar_t *srcW, unsigned long ulLen )
{
   ULONG ulDst, ulSrc;

   dstW[ ulLen ] = 0;
   ulDst = lstrlenW( dstW );
   if( ulDst < ulLen )
   {
      ulSrc = lstrlenW( srcW );
      if( ulDst + ulSrc > ulLen )
         ulSrc = ulLen - ulDst;
      memcpy( &dstW[ ulDst ], srcW, ulSrc * sizeof( wchar_t ) );
      dstW[ ulDst + ulSrc ] = 0;
   }
   return dstW;
}

/*----------------------------------------------------------------------*/

static void hb_itemPushList( ULONG ulRefMask, ULONG ulPCount, PHB_ITEM** pItems )
{
   HB_ITEM itmRef;
   ULONG   ulParam;

   if( ulPCount )
   {
      /* initialize the reference item */
      itmRef.type = HB_IT_BYREF;
      itmRef.item.asRefer.offset = -1;
      itmRef.item.asRefer.BasePtr.itemsbasePtr = pItems;

      for( ulParam = 0; ulParam < ulPCount; ulParam++ )
      {
         if( ulRefMask & ( 1L << ulParam ) )
         {
            /* when item is passed by reference then we have to put
               the reference on the stack instead of the item itself */
            itmRef.item.asRefer.value = ulParam+1;
            hb_vmPush( &itmRef );
         }
         else
         {
            hb_vmPush( ( *pItems )[ ulParam ] );
         }
      }
   }
}

/*----------------------------------------------------------------------*/

#undef  INTERFACE
#define INTERFACE IEventHandler

DECLARE_INTERFACE_ ( INTERFACE, IDispatch )
{
   /* IUnknown functions */
   STDMETHOD  ( QueryInterface ) ( THIS_ REFIID, void ** ) PURE;
   STDMETHOD_ ( ULONG, AddRef )  ( THIS ) PURE;
   STDMETHOD_ ( ULONG, Release ) ( THIS ) PURE;
   /* IDispatch functions */
   STDMETHOD ( GetTypeInfoCount ) ( THIS_ UINT * ) PURE;
   STDMETHOD ( GetTypeInfo ) ( THIS_ UINT, LCID, ITypeInfo ** ) PURE;
   STDMETHOD ( GetIDsOfNames ) ( THIS_ REFIID, LPOLESTR *, UINT, LCID, DISPID * ) PURE;
   STDMETHOD ( Invoke ) ( THIS_ DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT * ) PURE;
};

#if !defined( HB_OLE_C_API )
typedef struct
{
   HRESULT ( STDMETHODCALLTYPE * QueryInterface ) ( IEventHandler*, REFIID, void** );
   ULONG   ( STDMETHODCALLTYPE * AddRef ) ( IEventHandler* );
   ULONG   ( STDMETHODCALLTYPE * Release ) ( IEventHandler* );
   HRESULT ( STDMETHODCALLTYPE * GetTypeInfoCount ) ( IEventHandler*, UINT* );
   HRESULT ( STDMETHODCALLTYPE * GetTypeInfo ) ( IEventHandler*,  UINT, LCID, ITypeInfo** );
   HRESULT ( STDMETHODCALLTYPE * GetIDsOfNames ) ( IEventHandler*, REFIID, LPOLESTR*, UINT, LCID, DISPID* );
   HRESULT ( STDMETHODCALLTYPE * Invoke ) ( IEventHandler*, DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* );
} IEventHandlerVtbl;
#endif

typedef struct
{
   IEventHandlerVtbl*      lpVtbl;
   int                     count;
   IConnectionPoint*       pIConnectionPoint;  /* Ref counted of course. */
   DWORD                   dwEventCookie;
   char*                   parent_on_invoke;
   IID                     device_event_interface_iid;
   PHB_ITEM                pSelf;              /* object to handle the events (optional) */
   PHB_ITEM                pEvents;
   int                     iID_riid;
} MyRealIEventHandler;

/*----------------------------------------------------------------------*/
/*               Here are IEventHandler's functions.                    */
/*----------------------------------------------------------------------*/

static HRESULT STDMETHODCALLTYPE QueryInterface( IEventHandler *self, REFIID vTableGuid, void **ppv )
{
   if( IsEqualIID( vTableGuid, HB_ID_REF( IID_IUnknown ) ) )
   {
      *ppv = ( IUnknown * ) self;
#ifdef __HBTOOUT__
hb_ToOutDebug( "..................if ( IsEqualIID( vTableGuid, HB_ID_REF( IID_IUnknown ) ) )" );
#endif
      HB_VTBL( self )->AddRef( HB_THIS( self ) );
      return S_OK;
   }

   if( IsEqualIID( vTableGuid, HB_ID_REF( IID_IDispatch ) ) )
   {
      *ppv = ( IDispatch * ) self;
#ifdef __HBTOOUT__
hb_ToOutDebug( "..................if ( IsEqualIID( vTableGuid, HB_ID_REF( IID_IDispatch ) ) )" );
#endif
      HB_VTBL( self )->AddRef( HB_THIS( self ) );
      return S_OK;
   }

   if( IsEqualIID( vTableGuid, HB_ID_REF( ( ( MyRealIEventHandler * ) self )->device_event_interface_iid ) ) )
   {
      if( ++( ( ( MyRealIEventHandler * ) self )->iID_riid ) == 1 )
      {
         *ppv = ( IDispatch* ) self;
#ifdef __HBTOOUT__
hb_ToOutDebug( "..................if ( IsEqualIID( vTableGuid, HB_ID_REF( ( ( MyRealIEventHandler * ) self )->device_event_interface_iid ) ) )" );
#endif
         HB_VTBL( self )->AddRef( HB_THIS( self ) );
      }
      return S_OK;
   }
   *ppv = 0;
   return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE AddRef( IEventHandler *self )
{
   return ++( ( MyRealIEventHandler * ) self )->count;
}


static ULONG STDMETHODCALLTYPE Release( IEventHandler *self )
{
   if( --( ( MyRealIEventHandler * ) self )->count == 0 )
   {
      if( ( ( MyRealIEventHandler * ) self)->pSelf )
      {
         hb_itemRelease( ( ( MyRealIEventHandler * ) self )->pSelf );
      }

      if( ( MyRealIEventHandler * ) self )
         GlobalFree( ( MyRealIEventHandler * ) self );

      return ( ULONG ) 0;
   }
   else
      return ( ULONG ) ( ( MyRealIEventHandler * ) self )->count;
}

static HRESULT STDMETHODCALLTYPE GetTypeInfoCount( IEventHandler *self, UINT *pCount )
{
   HB_SYMBOL_UNUSED( self );
   HB_SYMBOL_UNUSED( pCount );

   return ( HRESULT ) E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE GetTypeInfo( IEventHandler *self, UINT itinfo, LCID lcid, ITypeInfo **pTypeInfo )
{
   HB_SYMBOL_UNUSED( self );
   HB_SYMBOL_UNUSED( itinfo );
   HB_SYMBOL_UNUSED( lcid );
   HB_SYMBOL_UNUSED( pTypeInfo );

   return ( HRESULT ) E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE GetIDsOfNames( IEventHandler *self, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid )
{

   HB_SYMBOL_UNUSED( self );
   HB_SYMBOL_UNUSED( riid );
   HB_SYMBOL_UNUSED( rgszNames );
   HB_SYMBOL_UNUSED( cNames );
   HB_SYMBOL_UNUSED( lcid );
   HB_SYMBOL_UNUSED( rgdispid );

   return ( HRESULT ) E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Invoke( IEventHandler *self, DISPID dispid, REFIID riid,
                                      LCID lcid, WORD wFlags, DISPPARAMS *params,
                                      VARIANT *result, EXCEPINFO *pexcepinfo, UINT *puArgErr )
{
   int        iArg;
   int        i;
   ULONG      ulRefMask = 0;
   ULONG      ulPos;
   PHB_ITEM   pItem;
   PHB_ITEM   pItemArray[ 32 ]; /* max 32 parameters? */
   PHB_ITEM   *pItems;
   PHB_ITEM   Key;

#if 0
hb_ToOutDebug( "event = %i",(int)dispid );
#endif

   /* We implement only a "default" interface */
   if( !IsEqualIID( riid, HB_ID_REF( IID_NULL ) ) )
      return( ( HRESULT ) DISP_E_UNKNOWNINTERFACE );

   HB_SYMBOL_UNUSED( lcid );
   HB_SYMBOL_UNUSED( wFlags );
   HB_SYMBOL_UNUSED( result );
   HB_SYMBOL_UNUSED( pexcepinfo );
   HB_SYMBOL_UNUSED( puArgErr );

   Key = hb_itemNew( NULL );
   if( hb_hashScan( ( ( MyRealIEventHandler * ) self )->pEvents, hb_itemPutNL( Key, dispid ), &ulPos ) )
   {
      PHB_ITEM pArray = hb_hashGetValueAt( ( ( MyRealIEventHandler * ) self )->pEvents, ulPos );
      PHB_ITEM pExec  = hb_arrayGetItemPtr( pArray, 1 );

      if( pExec )
      {
         hb_vmPushState();

         switch ( hb_itemType( pExec ) )
         {
         case HB_IT_BLOCK:
            hb_vmPushSymbol( &hb_symEval );
            hb_vmPush( pExec );
            break;

         case HB_IT_STRING:
            {
               PHB_ITEM pObject = hb_arrayGetItemPtr( pArray, 3 );

               hb_vmPushSymbol( hb_dynsymSymbol( hb_dynsymFindName( hb_itemGetCPtr( pExec ) ) ) );
               if( HB_IS_OBJECT( pObject ) )
                  hb_vmPush( pObject );
               else
                  hb_vmPushNil();
            }
            break;

         case HB_IT_POINTER:
            hb_vmPushSymbol( hb_dynsymSymbol( ( ( PHB_SYMB ) pExec )->pDynSym ) );
            hb_vmPushNil();
            break;
         }

         iArg = params->cArgs;

         if( iArg > ( int ) HB_SIZEOFARRAY( pItemArray ) )
            iArg = HB_SIZEOFARRAY( pItemArray );

         for( i = 1; i <= iArg; i++ )
         {
            pItem = hb_itemNew( NULL );
            hb_oleVariantToItem( pItem, &( params->rgvarg[ iArg - i ] ) );
            pItemArray[ i - 1 ] = pItem;
            ulRefMask |= ( 1L << ( i - 1 ) );                                /* set bit i */
         }

         if( iArg )
         {
            pItems = pItemArray;
            hb_itemPushList( ulRefMask, iArg, &pItems );
         }

         /* execute */
         hb_vmDo( ( USHORT ) iArg );

         for( i = iArg; i > 0; i-- )
         {
            if( HB_IS_BYREF( pItemArray[ iArg - i ] ) )
               hb_oleItemToVariant( &( params->rgvarg[ iArg - i ] ), pItemArray[ iArg - i ] );
         }

         /* Pritpal */
         if( iArg )
         {
            for( i = iArg; i > 0; i-- )
               hb_itemRelease( pItemArray[ i - 1 ] );
         }
         hb_vmPopState();
      }
   }
   hb_itemRelease( Key );   /* Pritpal */

   return ( HRESULT ) S_OK;
}

/*----------------------------------------------------------------------*/

static const IEventHandlerVtbl IEventHandler_Vtbl =
{
   QueryInterface,
   AddRef,
   Release,
   GetTypeInfoCount,
   GetTypeInfo,
   GetIDsOfNames,
   Invoke
};

typedef IEventHandler device_interface;

/*----------------------------------------------------------------------*/

static HRESULT SetupConnectionPoint( device_interface* pdevice_interface, REFIID riid, void** pThis, int* pn )
{
   IConnectionPointContainer*  pIConnectionPointContainerTemp = NULL;
   IUnknown*                   pIUnknown;
   IConnectionPoint*           m_pIConnectionPoint = NULL;
   IEnumConnectionPoints*      m_pIEnumConnectionPoints;
   HRESULT                     hr;
   IID                         rriid;
   register IEventHandler*     thisobj;
   DWORD                       dwCookie = 0;

   HB_SYMBOL_UNUSED( riid );
   HB_SYMBOL_UNUSED( pn );

   thisobj = ( IEventHandler * ) GlobalAlloc( GMEM_FIXED, sizeof( MyRealIEventHandler ) );
   if( thisobj )
   {
      ( ( MyRealIEventHandler * ) thisobj )->lpVtbl = ( IEventHandlerVtbl * ) &IEventHandler_Vtbl;
      ( ( MyRealIEventHandler * ) thisobj )->pSelf = NULL;
      ( ( MyRealIEventHandler * ) thisobj )->count = 0;
      ( ( MyRealIEventHandler * ) thisobj )->iID_riid = 0;

      hr = HB_VTBL( thisobj )->QueryInterface( HB_THIS_( thisobj ) HB_ID_REF( IID_IUnknown ), (void **) (void*) &pIUnknown );
      if( hr == S_OK && pIUnknown )
      {
         hr = HB_VTBL( pdevice_interface )->QueryInterface( HB_THIS_( pdevice_interface ) HB_ID_REF( IID_IConnectionPointContainer ), (void**) (void*) &pIConnectionPointContainerTemp);
         if( hr == S_OK && pIConnectionPointContainerTemp )
         {
            hr = HB_VTBL( pIConnectionPointContainerTemp )->EnumConnectionPoints( HB_THIS_( pIConnectionPointContainerTemp ) &m_pIEnumConnectionPoints );
            if( hr == S_OK && m_pIEnumConnectionPoints )
            {
               do
               {
                  hr = HB_VTBL( m_pIEnumConnectionPoints )->Next( HB_THIS_( m_pIEnumConnectionPoints ) 1, &m_pIConnectionPoint, NULL );
                  if( hr == S_OK )
                  {
                     hr = HB_VTBL( m_pIConnectionPoint )->GetConnectionInterface( HB_THIS_( m_pIConnectionPoint ) &rriid );
                     if( hr == S_OK )
                     {
                        /**************           This has to be review         *******************
                               PellesC was generating GPF at this point
                               After commenting it out, I could not see any difference in objects
                               I play with. Cannot say why did I retained it so long.            */
                        #if 1
                        ( ( MyRealIEventHandler* ) thisobj )->device_event_interface_iid = rriid;
                        #endif

                        hr = HB_VTBL( m_pIConnectionPoint )->Advise( HB_THIS_( m_pIConnectionPoint ) pIUnknown, &dwCookie );
                        if( hr == S_OK )
                        {
                           ( ( MyRealIEventHandler* ) thisobj )->pIConnectionPoint = m_pIConnectionPoint;
                           ( ( MyRealIEventHandler* ) thisobj )->dwEventCookie     = dwCookie;
                        }
                        else
                           hr = S_OK;
                     }
                     else
                        hr = S_OK;
                  }
               } while( hr == S_OK );
               HB_VTBL( m_pIEnumConnectionPoints )->Release( HB_THIS( m_pIEnumConnectionPoints ) );
               m_pIEnumConnectionPoints = NULL;
            }
            HB_VTBL( pIConnectionPointContainerTemp )->Release( HB_THIS( pIConnectionPointContainerTemp ) );
            pIConnectionPointContainerTemp = NULL;
         }
         HB_VTBL( pIUnknown )->Release( HB_THIS( pIUnknown ) );
         pIUnknown = NULL;
      }
   }
   else
      hr = E_OUTOFMEMORY;

   *pThis = ( void * ) thisobj;

   return hr;
}
/*----------------------------------------------------------------------*/

HB_FUNC( HB_AX_SHUTDOWNCONNECTIONPOINT )
{
   MyRealIEventHandler* hSink = ( MyRealIEventHandler * ) ( HB_PTRDIFF ) hb_parnint( 1 );

   if( hSink && hSink->pIConnectionPoint )
   {
      hSink->dwEventCookie = 0;
      HB_VTBL( hSink->pIConnectionPoint )->Release( HB_THIS( hSink->pIConnectionPoint ) );
      hSink->pIConnectionPoint = NULL;
   }

   if( hSink && hSink->pEvents )
      hb_itemRelease( hSink->pEvents );
}

/*----------------------------------------------------------------------*/

HB_FUNC( HB_AX_SETUPCONNECTIONPOINT )
{
   HRESULT              hr;
   MyRealIEventHandler* hSink = NULL;
   LPIID                riid  = ( LPIID ) &IID_IDispatch;
   int                  n = 0;

   hr = SetupConnectionPoint( ( device_interface* ) hb_oleParam( 1 ), ( REFIID ) riid, ( void** ) (void*) &hSink, &n ) ;

   hSink->pEvents = hb_itemNew( hb_param( 4, HB_IT_ANY ) );

   hb_stornint( ( HB_PTRDIFF ) hSink, 2 );
   hb_storni( n, 3 );
   hb_retnl( hr );
}

/*----------------------------------------------------------------------*/
/*                ActiveX Container Management Interface                */
/*----------------------------------------------------------------------*/
HB_FUNC( HB_AX_ATLAXWININIT )
{
   BOOL bRet = FALSE;

   if( !s_hLib )
   {
      PATLAXWININIT AtlAxWinInit;

      TCHAR szLibName[ MAX_PATH + 1 ] = { 0 };

      /* please always check if given function need size in TCHARs or bytes
       * in MS documentation.
       */
      GetSystemDirectory( szLibName, MAX_PATH );

      /* TEXT() macro can be used for literal (and only for literal) string
       * values. It creates array of TCHAR items with given text. It cannot
       * be used to ecapsulate non literal values. In different [x]Harbour
       * source code you may find things like TEXT( hb_parc( 1 ) ) - it's
       * a technical nonsense written by someone who has no idea what this
       * macro does.
       * Use new string functions (StringCchCat() in this case) which always
       * set trailing 0 in the given buffer just like hb_strn*() functions.
       * [l]str[n]cat() is absolute and should not be used by new code. It does
       * not guarantee buffer overflow protection and/or setting trailing 0.
       * StringCch*() functions operate on TCHAR types.
       */
      StringCchCat( szLibName, MAX_PATH + 1, TEXT( "\\atl.dll" ) );

       /* Please note that I intentionally removed any casting when szLibName
        * is passed to WinAPI functions. Such casting can pacify warnings so
        * program will be compiled but code will be still wrong so it does not
        * fix anything and only makes much harder later fixing when someone
        * will look for wrong code which is not UNICODE ready. The wrong casting
        * related to different character representations used only to pacify
        * warnings is the biggest problem in MS-Win 3-rd party code written
        * for [x]Harbour because it only hides bugs and then people have to
        * look for the code line by line to fix it. I dedicated above note to
        * developers of few well known MS-Win GUI projects for [x]Harbour.
        * Please remember about it.
        */
      s_hLib = LoadLibrary( szLibName );

      if( s_hLib )
      {
         AtlAxWinInit = ( PATLAXWININIT ) GetProcAddress( s_hLib, HBTEXT( "AtlAxWinInit" ) );

         if( AtlAxWinInit )
         {
            if( ( AtlAxWinInit )() )
               bRet = TRUE;
         }

         if( !bRet )
         {
            FreeLibrary( s_hLib );
            s_hLib = NULL;
         }
      }
   }
   else
      bRet = TRUE;

   hb_retl( bRet );
}
/*----------------------------------------------------------------------*/

HB_FUNC( HB_AX_ATLAXWINTERM )
{
   PATLAXWINTERM AtlAxWinTerm;
   BOOL          bRet = FALSE;

   if( s_hLib )
   {
      AtlAxWinTerm = ( PATLAXWINTERM ) GetProcAddress( s_hLib, HBTEXT( "AtlAxWinTerm" ) );

      if( AtlAxWinTerm )
      {
         if( AtlAxWinTerm() )
         {
            FreeLibrary( s_hLib );
            s_hLib = NULL;
            bRet = TRUE;
         }
      }
   }
   hb_retl( bRet );
}

/*----------------------------------------------------------------------*/
/*
 *    ::hObj := HB_AX_AtlAxGetControl( "ATLAXWin", ::hContainer, ::CLSID, ::nID, ;
 *                   ::aPos[ 1 ], ::aPos[ 2 ], ::aSize[ 1 ], ::aSize[ 2 ], ::style, ::exStyle, @hx )
 */
HB_FUNC( HB_AX_ATLAXGETCONTROL ) /* HWND hWnd = handle of control container window */
{
   IUnknown  *pUnk = NULL;
   IDispatch *obj;
   PATLAXGETCONTROL AtlAxGetControl;
   HWND  hWnd      = NULL;
   char  *lpcclass = hb_parcx( 1 );
   HWND  hParent   = ( ISPOINTER( 2 ) ? ( HWND ) hb_parptr( 2 ) : ( HWND )( HB_PTRDIFF ) hb_parnint( 2 ) );
   char  *Caption  = hb_parcx( 3 );
   HMENU id        = HB_ISNUM(  4 ) ? ( HMENU ) ( HB_PTRDIFF ) hb_parnint( 4 ) : ( HMENU ) ( HB_PTRDIFF ) -1 ;
   int   x         = HB_ISNUM(  5 ) ? hb_parni(  5 ) : 0;
   int   y         = HB_ISNUM(  6 ) ? hb_parni(  6 ) : 0;
   int   w         = HB_ISNUM(  7 ) ? hb_parni(  7 ) : 0;
   int   h         = HB_ISNUM(  8 ) ? hb_parni(  8 ) : 0;
   int   Style     = HB_ISNUM(  9 ) ? hb_parni(  9 ) : WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
   int   Exstyle   = HB_ISNUM( 10 ) ? hb_parni( 10 ) : 0;

   AtlAxGetControl = ( PATLAXGETCONTROL ) GetProcAddress( s_hLib, HBTEXT( "AtlAxGetControl" ) );
   if( AtlAxGetControl )
   {
      LPTSTR cCaption = HB_TCHAR_CONVTO( Caption );
      LPTSTR cClass = HB_TCHAR_CONVTO( lpcclass );

      hWnd = ( HWND ) CreateWindowEx( Exstyle, cClass, cCaption, Style, x, y, w, h, hParent, id,
                                                                GetModuleHandle( NULL ), NULL );
      HB_TCHAR_FREE( cCaption );
      HB_TCHAR_FREE( cClass );

      if( hWnd )
      {
         ( AtlAxGetControl )( hWnd, &pUnk );

         if( pUnk )
         {
            HB_VTBL( pUnk )->QueryInterface( HB_THIS_( pUnk ) HB_ID_REF( IID_IDispatch ), ( void** ) (void*) &obj );
            HB_VTBL( pUnk )->Release( HB_THIS( pUnk ) );

            hb_itemReturnRelease( hb_oleItemPut( NULL, obj ) );
         }
      }
   }

   /* return the control handle */
   hb_stornint( ( HB_PTRDIFF ) hWnd, 12 );
   hb_stornint( ( HB_PTRDIFF ) pUnk, 13 );
}

/*----------------------------------------------------------------------*/

HB_FUNC( HB_AX_ATLCREATEWINDOW ) /* ( hWndContainer, CLSID, menuID=0, x, y, w, h, style, exstyle ) --> pWnd */
{
   LPTSTR cCaption = HB_TCHAR_CONVTO( hb_parcx( 2 ) );

   hb_retptr( ( void * ) ( HB_PTRDIFF ) CreateWindowEx( HB_ISNUM( 9 ) /* Exstyle */,
                                                        TEXT( "ATLAXWin" ),
                                                        cCaption,
                                                        HB_ISNUM( 8 ) ? hb_parni( 8 ) : WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS /* Style */,
                                                        hb_parni( 4 ) /* x */,
                                                        hb_parni( 5 ) /* y */,
                                                        hb_parni( 6 ) /* w */,
                                                        hb_parni( 7 ) /* h */,
                                                        ( HWND ) hb_parptr( 1 ) /* hParent */,
                                                        HB_ISPOINTER( 3 ) ? ( HMENU ) hb_parptr( 3 ) : ( HMENU ) ( HB_PTRDIFF ) -1 /* id */,
                                                        GetModuleHandle( NULL ),
                                                        NULL ) );

   HB_TCHAR_FREE( cCaption );
}

/*----------------------------------------------------------------------*/

HB_FUNC( HB_AX_ATLGETCONTROL ) /* HWND hWnd = handle of control container window */
{
   PATLAXGETCONTROL AtlAxGetControl;
   IDispatch        *obj;
   IUnknown         *pUnk = NULL;
   HWND             hWnd = ( ISPOINTER( 1 ) ? ( HWND ) hb_parptr( 1 ) : ( HWND )( HB_PTRDIFF ) hb_parnint( 1 ) );

   AtlAxGetControl = ( PATLAXGETCONTROL ) GetProcAddress( s_hLib, HBTEXT( "AtlAxGetControl" ) );
   if( AtlAxGetControl )
   {
      if( hWnd )
      {
         ( AtlAxGetControl )( hWnd, &pUnk );

         if( pUnk )
         {
            HB_VTBL( pUnk )->QueryInterface( HB_THIS_( pUnk ) HB_ID_REF( IID_IDispatch ), ( void** ) (void*) &obj );
            HB_VTBL( pUnk )->Release( HB_THIS( pUnk ) );

            hb_itemReturnRelease( hb_oleItemPut( NULL, obj ) );
            hb_stornint( ( HB_PTRDIFF ) pUnk, 2 );
         }
      }
   }
}

/*----------------------------------------------------------------------*/

HB_FUNC( HB_AX_ATLGETUNKNOWN ) /* HWND hWnd = handle of control container window */
{
   PATLAXGETCONTROL AtlAxGetControl;
   IUnknown         *pUnk = NULL;
   HWND             hWnd = ( ISPOINTER( 1 ) ? ( HWND ) hb_parptr( 1 ) : ( HWND )( HB_PTRDIFF ) hb_parnint( 1 ) );

   AtlAxGetControl = ( PATLAXGETCONTROL ) GetProcAddress( s_hLib, HBTEXT( "AtlAxGetControl" ) );
   if( AtlAxGetControl )
   {
      if( hWnd )
      {
         ( AtlAxGetControl )( hWnd, &pUnk );

         if( pUnk )
            hb_retnint( ( HB_PTRDIFF ) pUnk );
      }
   }
}

/*----------------------------------------------------------------------*/

HB_FUNC( HB_AX_ATLSETVERB )
{
   HWND hwnd = ( ISPOINTER( 1 ) ? ( HWND ) hb_parptr( 1 ) : ( HWND )( HB_PTRDIFF ) hb_parnint( 1 ) );

   if( hwnd )
   {
      IUnknown *pUnk = ( IUnknown* ) ( HB_PTRDIFF ) hb_parnint( 2 );

      IOleObject *lpOleObject = NULL;
      if( SUCCEEDED( HB_VTBL( pUnk )->QueryInterface( HB_THIS_( pUnk ) HB_ID_REF( IID_IOleObject ), ( void** ) ( void* ) &lpOleObject ) ) )
      {
         IOleClientSite* lpOleClientSite;

         HB_VTBL( pUnk )->Release( HB_THIS( pUnk ) );

         if( SUCCEEDED( HB_VTBL( lpOleObject )->GetClientSite( HB_THIS_( lpOleObject ) &lpOleClientSite ) ) )
         {
            MSG Msg;
            RECT rct;

            memset( &Msg, 0, sizeof( MSG ) );
            GetClientRect( hwnd, &rct );

            HB_VTBL( lpOleObject )->DoVerb( HB_THIS_( lpOleObject ) hb_parni( 3 ), &Msg, lpOleClientSite, 0, hwnd, &rct );
         }
      }
   }
}

/*----------------------------------------------------------------------*/

#if 0

HB_FUNC( WVG_AXGETUNKNOWN ) /* ( hWnd ) --> pUnk */
{
   IUnknown*   pUnk = NULL;
   HRESULT     lOleError;

   if( ! s_pAtlAxGetControl )
   {
      hb_oleSetError( S_OK );
      hb_errRT_BASE_SubstR( EG_UNSUPPORTED, 3012, "ActiveX not initialized", HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
      return;
   }

   lOleError = ( *s_pAtlAxGetControl )( ( HWND ) hb_parptr( 1 ), &pUnk );

   hb_oleSetError( lOleError );

   if( lOleError == S_OK )
      hb_retptr( pUnk );
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

HB_FUNC( WVG_AXDOVERB ) /* ( hWndAx, iVerb ) --> hResult */
{
   HWND        hWnd = ( HWND ) hb_parptr( 1 );
   IUnknown*   pUnk = NULL;
   HRESULT     lOleError;

   if( ! s_pAtlAxGetControl )
   {
      hb_oleSetError( S_OK );
      hb_errRT_BASE_SubstR( EG_UNSUPPORTED, 3012, "ActiveX not initialized", HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
      return;
   }

   lOleError = ( *s_pAtlAxGetControl )( hWnd, &pUnk );

   if( lOleError == S_OK )
   {
      IOleObject *lpOleObject = NULL;

      lOleError = HB_VTBL( pUnk )->QueryInterface( HB_THIS_( pUnk ) HB_ID_REF( IID_IOleObject ), ( void** ) ( void* ) &lpOleObject );
      if( lOleError == S_OK )
      {
         IOleClientSite* lpOleClientSite;

         lOleError = HB_VTBL( lpOleObject )->GetClientSite( HB_THIS_( lpOleObject ) &lpOleClientSite );
         if( lOleError == S_OK )
         {
            MSG Msg;
            RECT rct;

            memset( &Msg, 0, sizeof( MSG ) );
            GetClientRect( hWnd, &rct );
            HB_VTBL( lpOleObject )->DoVerb( HB_THIS_( lpOleObject ) hb_parni( 2 ), &Msg, lpOleClientSite, 0, hWnd, &rct );
         }
      }
   }

   hb_oleSetError( lOleError );

   hb_retni( ( int ) lOleError );
}

HB_FUNC( __XAXREGISTERHANDLER )  /* ( pDisp, bHandler ) --> pSink */
{
   IDispatch * pDisp = hb_oleParam( 1 );

   if( pDisp )
   {
      PHB_ITEM pItemBlock = hb_param( 2, HB_IT_BLOCK | HB_IT_SYMBOL );

      if( pItemBlock )
      {
         IConnectionPointContainer*  pCPC = NULL;
         IEnumConnectionPoints*      pEnumCPs = NULL;
         IConnectionPoint*           pCP = NULL;
         HRESULT                     lOleError;
         IID                         rriid;

         lOleError = HB_VTBL( pDisp )->QueryInterface( HB_THIS_( pDisp ) HB_ID_REF( IID_IConnectionPointContainer ), ( void** ) ( void* ) &pCPC );
         if( lOleError == S_OK && pCPC )
         {
            lOleError = HB_VTBL( pCPC )->EnumConnectionPoints( HB_THIS_( pCPC ) &pEnumCPs );
            if( lOleError == S_OK && pEnumCPs )
            {
               HRESULT hr = S_OK;
               do
               {
                  lOleError = HB_VTBL( pEnumCPs )->Next( HB_THIS_( pEnumCPs ) 1, &pCP, NULL );
                  if( lOleError == S_OK )
                  {
                     lOleError = HB_VTBL( pCP )->GetConnectionInterface( HB_THIS_( pCP ) &rriid );
                     if( lOleError == S_OK )
                     {
                        DWORD dwCookie = 0;
                        ISink * pSink = ( ISink* ) hb_gcAlloc( sizeof( ISink ), hb_sink_destructor ); /* TODO: GlobalAlloc GMEM_FIXED ??? */
                        pSink->lpVtbl = ( IDispatchVtbl * ) &ISink_Vtbl;
                        pSink->count = 0; /* We do not need to increment it here, Advise will do it auto */
                        pSink->pItemHandler = hb_itemNew( pItemBlock );

                        lOleError = HB_VTBL( pCP )->Advise( HB_THIS_( pCP ) ( IUnknown* ) pSink, &dwCookie );

                        pSink->pConnectionPoint = pCP;
                        pSink->dwCookie = dwCookie;
                        hb_retptrGC( pSink );
                        hr = 1;
                     }
                     else
                        lOleError = S_OK;
                  }
               } while( hr == S_OK );
               HB_VTBL( pEnumCPs )->Release( HB_THIS( pEnumCPs ) );
            }
            HB_VTBL( pCPC )->Release( HB_THIS( pCPC ) );
         }

         hb_oleSetError( lOleError );
         if( lOleError != S_OK )
            hb_errRT_BASE_SubstR( EG_ARG, 3012, "Failed to obtain connection point", HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
      }
      else
         hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
   }
}

#endif
