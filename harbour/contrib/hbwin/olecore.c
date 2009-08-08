/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * OLE library
 *
 * Copyright 2000, 2003 Jose F. Gimenez (JFG) <jfgimenez@wanadoo.es>
 * Copyright 2008, 2009 Mindaugas Kavaliauskas <dbtopas at dbtopas.lt>
 * www - http://www.harbour-project.org
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

#include "hbwinole.h"

/* base date value in OLE (1899-12-30) as julian day */
#define HB_OLE_DATE_BASE      0x0024D9AB

static PHB_DYNS s_pDyns_hb_oleauto;
static PHB_DYNS s_pDyns_hObjAccess;
static PHB_DYNS s_pDyns_hObjAssign;

typedef struct
{
   HRESULT  lOleError;
} HB_OLEDATA, * PHB_OLEDATA;

static HB_TSD_NEW( s_oleData, sizeof( HB_OLEDATA ), NULL, NULL );
#define hb_getOleData()       ( ( PHB_OLEDATA ) hb_stackGetTSD( &s_oleData ) )


HB_FUNC_EXTERN( WIN_OLEAUTO );


void hb_oleSetError( HRESULT lOleError )
{
   hb_getOleData()->lOleError = lOleError;
}


HRESULT hb_oleGetError( void )
{
   return hb_getOleData()->lOleError;
}


static void hb_olecore_init( void* cargo )
{
   HB_SYMBOL_UNUSED( cargo );

   s_pDyns_hb_oleauto = hb_dynsymGetCase( "WIN_OLEAUTO" );
   s_pDyns_hObjAccess = hb_dynsymGetCase( "__HOBJ" );
   s_pDyns_hObjAssign = hb_dynsymGetCase( "___HOBJ" );

   if( s_pDyns_hObjAccess == s_pDyns_hObjAssign )
   {
      /* Never executed. Just force linkage */
      HB_FUNC_EXEC( WIN_OLEAUTO );
   }

   hb_oleInit();
}


static HB_GARBAGE_FUNC( hb_ole_destructor )
{
   IDispatch**  ppDisp = ( IDispatch** ) Cargo;

   if( *ppDisp )
   {
      HB_VTBL( *ppDisp )->Release( HB_THIS( *ppDisp ) );
      *ppDisp = NULL;
   }
}


static HB_GARBAGE_FUNC( hb_oleenum_destructor )
{
   IEnumVARIANT**  ppEnum = ( IEnumVARIANT** ) Cargo;

   if( *ppEnum )
   {
      HB_VTBL( *ppEnum )->Release( HB_THIS( *ppEnum ) );
      *ppEnum = NULL;
   }
}


IDispatch* hb_oleParam( int iParam )
{
   IDispatch**  ppDisp = ( IDispatch** ) hb_parptrGC( hb_ole_destructor, iParam );

   if( ppDisp && *ppDisp )
      return *ppDisp;

   hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
   return NULL;
}


static IEnumVARIANT* hb_oleenumParam( int iParam )
{
   IEnumVARIANT**  ppEnum = ( IEnumVARIANT** ) hb_parptrGC( hb_oleenum_destructor, iParam );

   if( ppEnum && *ppEnum )
      return *ppEnum;

   hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
   return NULL;
}


/* Unicode string management */

static wchar_t* AnsiToWide( const char* szString )
{
   int       iLen;
   wchar_t*  szWide;

   iLen = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szString, -1, NULL, 0 );
   szWide = ( wchar_t* ) hb_xgrab( iLen * sizeof( wchar_t ) );
   MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szString, -1, szWide, iLen );
   return szWide;
}


static char* WideToAnsi( const wchar_t* szWide )
{
   int    iLen;
   char*  szString;

   iLen = WideCharToMultiByte( CP_ACP, 0, szWide, -1, NULL, 0, NULL, NULL );
   szString = ( char* ) hb_xgrab( ( iLen + 1 ) * sizeof( char ) );
   WideCharToMultiByte( CP_ACP, 0, szWide, -1, szString, iLen, NULL, NULL );
   return szString;
}


/* Item <-> Variant conversion */

void hb_oleItemToVariant( VARIANT* pVariant, PHB_ITEM pItem )
{
   wchar_t*  szString;

   VariantClear( pVariant );  /* pVariant->n1.n2.vt = VT_EMPTY; */

   switch( hb_itemType( pItem ) )
   {
      case HB_IT_STRING:
      case HB_IT_MEMO:
         pVariant->n1.n2.vt = VT_BSTR;
         szString = AnsiToWide( hb_itemGetCPtr( pItem ) );
         pVariant->n1.n2.n3.bstrVal = SysAllocString( szString );
         hb_xfree( szString );
         break;

      case HB_IT_LOGICAL:
         pVariant->n1.n2.vt = VT_BOOL;
         pVariant->n1.n2.n3.boolVal = hb_itemGetL( pItem ) ? VARIANT_TRUE : VARIANT_FALSE;
         break;

      case HB_IT_INTEGER:
         pVariant->n1.n2.vt = VT_I4;
         pVariant->n1.n2.n3.lVal = hb_itemGetNL( pItem );
         break;

      case HB_IT_LONG:
         pVariant->n1.n2.vt = VT_I8;
#if HB_LONG_MAX == INT32_MAX || defined( HB_LONG_LONG_OFF )
         pVariant->n1.n2.n3.llVal = hb_itemGetNInt( pItem );
#else
         pVariant->n1.n2.n3.lVal = hb_itemGetNL( pItem );
#endif
         break;

      case HB_IT_DOUBLE:
         pVariant->n1.n2.vt = VT_R8;
         pVariant->n1.n2.n3.dblVal = hb_itemGetND( pItem );
         break;

      case HB_IT_DATE:
         pVariant->n1.n2.vt = VT_DATE;
         pVariant->n1.n2.n3.dblVal = ( double ) ( hb_itemGetDL( pItem ) - HB_OLE_DATE_BASE );
         break;

      case HB_IT_TIMESTAMP:
         pVariant->n1.n2.vt = VT_DATE;
         pVariant->n1.n2.n3.dblVal = hb_itemGetTD( pItem ) - HB_OLE_DATE_BASE;
         break;

      case HB_IT_OBJECT: /* or ARRAY */
         if( HB_IS_OBJECT( pItem ) )
         {
            if( hb_clsIsParent( hb_objGetClass( pItem ), "WIN_OLEAUTO" ) )
            {
               IDispatch * pDisp;

               hb_vmPushDynSym( s_pDyns_hObjAccess );
               hb_vmPush( pItem );
               hb_vmSend( 0 );

               pDisp = hb_oleParam( -1 );

               /* pVariant will be freed using VariantClear().
                  We increment reference count to keep OLE object alive */
               HB_VTBL( pDisp )->AddRef( HB_THIS( pDisp ) );
               pVariant->n1.n2.vt = VT_DISPATCH;
               pVariant->n1.n2.n3.pdispVal = pDisp;
            }
         }
         else
         {
            SAFEARRAY*      pSafeArray;
            SAFEARRAYBOUND  sabound[ 1 ];
            ULONG           ul, ulLen;

            sabound[ 0 ].lLbound = 0;
            sabound[ 0 ].cElements = ulLen = hb_arrayLen( pItem );

            pSafeArray = SafeArrayCreate( VT_VARIANT, 1, sabound );
            pVariant->n1.n2.vt = VT_VARIANT | VT_ARRAY;
            pVariant->n1.n2.n3.parray = pSafeArray;

            for( ul = 0; ul < ulLen; ul++ )
            {
               VARIANT  vItem;
               long     lIndex[ 1 ];

               hb_oleItemToVariant( &vItem, hb_arrayGetItemPtr( pItem, ul + 1 ) );
               lIndex[ 0 ] = ( long ) ul;
               SafeArrayPutElement( pSafeArray, lIndex, &vItem );
               VariantClear( &vItem );
            }
         }
         break;
   }
}


PHB_ITEM hb_oleItemPut( PHB_ITEM pItem, IDispatch* pDisp )
{
   IDispatch** ppDisp;

   ppDisp = ( IDispatch** ) hb_gcAlloc( sizeof( IDispatch* ), hb_ole_destructor );
   *ppDisp = pDisp;

   return hb_itemPutPtrGC( pItem, ppDisp );
}

void hb_oleVariantToItem( PHB_ITEM pItem, VARIANT* pVariant )
{
   switch( pVariant->n1.n2.vt )
   {
      case VT_BSTR:
      {
         char* szString = WideToAnsi( pVariant->n1.n2.n3.bstrVal );
         hb_itemPutCPtr( pItem, szString );
         break;
      }

      case VT_BOOL:
         hb_itemPutL( pItem, pVariant->n1.n2.n3.boolVal ? TRUE : FALSE );
         break;

      case VT_DISPATCH:
      {
         hb_itemClear( pItem );

         if( pVariant->n1.n2.n3.pdispVal )
         {
            PHB_ITEM    pObject, pPtrGC;
            IDispatch** ppDisp;

            if( hb_vmRequestReenter() )
            {
               hb_vmPushDynSym( s_pDyns_hb_oleauto );
               hb_vmPushNil();
               hb_vmDo( 0 );

               pObject = hb_itemNew( hb_stackReturnItem() );

               ppDisp = ( IDispatch** ) hb_gcAlloc( sizeof( IDispatch* ), hb_ole_destructor );
               *ppDisp = pVariant->n1.n2.n3.pdispVal;
               pPtrGC = hb_itemPutPtrGC( NULL, ppDisp );

               /* Item is one more copy of the object */
               HB_VTBL( *ppDisp )->AddRef( HB_THIS( *ppDisp ) );

               hb_vmPushDynSym( s_pDyns_hObjAssign );
               hb_vmPush( pObject );
               hb_vmPush( pPtrGC );
               hb_vmSend( 1 );
               hb_itemRelease( pPtrGC );
               hb_vmRequestRestore();

               /* We should store object to pItem after hb_vmRequestRestore(),
                * because pItem actualy can be stack's return item!
                */
               hb_itemMove( pItem, pObject );
               hb_itemRelease( pObject );
            }
         }
         break;
      }

      case VT_I1:
           hb_itemPutNI( pItem, ( int ) pVariant->n1.n2.n3.cVal );
           break;

      case VT_I2:
           hb_itemPutNI( pItem, ( int ) pVariant->n1.n2.n3.iVal );
           break;

      case VT_I4:
           hb_itemPutNL( pItem, pVariant->n1.n2.n3.lVal );
           break;

      case VT_I8:
#if HB_LONG_MAX == INT32_MAX || defined( HB_LONG_LONG_OFF )
           hb_itemPutNInt( pItem, pVariant->n1.n2.n3.llVal );
#else
           hb_itemPutNInt( pItem, ( HB_LONG ) pVariant->n1.n2.n3.lVal );
#endif
           break;

      case VT_UI1:
           hb_itemPutNI( pItem, ( int ) pVariant->n1.n2.n3.bVal );
           break;

      case VT_UI2:
           hb_itemPutNI( pItem, ( int ) pVariant->n1.n2.n3.uiVal );
           break;

      case VT_UI4:
           hb_itemPutNInt( pItem, ( HB_LONG ) pVariant->n1.n2.n3.ulVal );
           break;

      case VT_UI8:
           /* TODO: sign is lost. Convertion to double will lose significant digits. */
#if HB_LONG_MAX == INT32_MAX || defined( HB_LONG_LONG_OFF )
           hb_itemPutNInt( pItem, ( HB_LONG ) pVariant->n1.n2.n3.ullVal );
#else
           hb_itemPutNInt( pItem, ( HB_LONG ) pVariant->n1.n2.n3.ulVal );
#endif
           break;

      case VT_R4:
           hb_itemPutND( pItem, ( double ) pVariant->n1.n2.n3.fltVal );
           break;

      case VT_R8:
           hb_itemPutND( pItem, pVariant->n1.n2.n3.dblVal );
           break;

      case VT_CY:
           hb_itemPutND( pItem, ( double ) pVariant->n1.n2.n3.cyVal.int64 / 10000 );
           break;

      case VT_INT:
           hb_itemPutNI( pItem, pVariant->n1.n2.n3.intVal );
           break;

      case VT_UINT:
           hb_itemPutNInt( pItem, ( HB_LONG ) pVariant->n1.n2.n3.uintVal );
           break;

      case VT_DATE:
      {
           long lJulian, lMilliSec;

           hb_timeStampUnpackDT( pVariant->n1.n2.n3.dblVal + HB_OLE_DATE_BASE, &lJulian, &lMilliSec );
           if( lMilliSec )
              hb_itemPutTDT( pItem, lJulian, lMilliSec );
           else
              hb_itemPutDL( pItem, lJulian );
           break;
      }

      default:
         hb_itemClear( pItem );
   }
}


/* IDispatch parameters, return value handling */

static void GetParams( DISPPARAMS * dispparam )
{
   VARIANTARG*  pArgs = NULL;
   UINT         uiArgCount, uiArg;

   uiArgCount = ( UINT ) hb_pcount();

   if( uiArgCount > 0 )
   {
      pArgs = ( VARIANTARG* ) hb_xgrab( sizeof( VARIANTARG ) * uiArgCount );

      for( uiArg = 0; uiArg < uiArgCount; uiArg++ )
      {
         VariantInit( &( pArgs[ uiArg ] ) );
         hb_oleItemToVariant( & pArgs[ uiArg ], hb_param( uiArgCount - uiArg, HB_IT_ANY ) );
      }
   }

   dispparam->rgvarg = pArgs;
   dispparam->cArgs  = uiArgCount;
   dispparam->rgdispidNamedArgs = 0;
   dispparam->cNamedArgs = 0;
}

#ifdef _PUTPARAMS_
static void PutParams( DISPPARAMS * dispparam )
{
   UINT uiArg;

   for( uiArg = 0; uiArg < dispparam->cArgs; uiArg++ )
   {
      if( HB_ISBYREF( uiArg + 1 ) )
      {
         PHB_ITEM pItem = hb_itemNew( NULL );

         hb_oleVariantToItem( pItem, & dispparam->rgvarg[ uiArg ] );

         hb_itemParamStoreForward( ( USHORT ) uiArg + 1, pItem );
      }
   }
}
#endif

static void FreeParams( DISPPARAMS * dispparam )
{
   UINT  ui;

   if( dispparam->cArgs > 0 )
   {
      for( ui = 0; ui < dispparam->cArgs; ui++ )
         VariantClear( & ( dispparam->rgvarg[ ui ] ) );

      hb_xfree( dispparam->rgvarg );
   }
}


/* PRG level functions and methods */

HB_FUNC( __OLECREATEOBJECT ) /* ( cOleName | cCLSID  [, cIID ] ) */
{
   wchar_t*    cCLSID;
   GUID        ClassID, iid = IID_IDispatch;
   IDispatch*  pDisp = NULL;
   IDispatch** ppDisp;
   const char* cOleName = hb_parc( 1 );
   const char* cID = hb_parc( 2 );
   HRESULT     lOleError;

   if( cOleName )
   {
      cCLSID = AnsiToWide( cOleName );
      if( cOleName[ 0 ] == '{' )
         lOleError = CLSIDFromString( (LPOLESTR) cCLSID, &ClassID );
      else
         lOleError = CLSIDFromProgID( (LPCOLESTR) cCLSID, &ClassID );
      hb_xfree( cCLSID );

      if( cID )
      {
         if( cID[ 0 ] == '{' )
         {
            cCLSID = AnsiToWide( cID );
            lOleError = CLSIDFromString( ( LPOLESTR ) cCLSID, &iid );
            hb_xfree( cCLSID );
         }
         else if( hb_parclen( 2 ) == ( ULONG ) sizeof( iid ) )
         {
            memcpy( ( LPVOID ) &iid, cID, sizeof( iid ) );
         }
      }

      if( lOleError == S_OK )
         lOleError = CoCreateInstance( HB_ID_REF( ClassID ), NULL, CLSCTX_SERVER, HB_ID_REF( iid ), ( void** ) ( void * ) &pDisp );
   }
   else
      lOleError = CO_E_CLASSSTRING;

   hb_oleSetError( lOleError );
   if( lOleError == S_OK )
   {
      ppDisp = ( IDispatch** ) hb_gcAlloc( sizeof( IDispatch* ), hb_ole_destructor );
      *ppDisp = pDisp;
      hb_retptrGC( ppDisp );
   }
   else
      hb_ret();
}


HB_FUNC( __OLEGETACTIVEOBJECT ) /* ( cOleName | cCLSID  [, cIID ] ) */
{
   BSTR        wCLSID;
   IID         ClassID, iid = IID_IDispatch;
   IDispatch*  pDisp = NULL;
   IDispatch** ppDisp;
   IUnknown*   pUnk = NULL;
   const char* cOleName = hb_parc( 1 );
   const char* cID = hb_parc( 2 );
   HRESULT     lOleError;

   if( cOleName )
   {
      wCLSID = ( BSTR ) AnsiToWide( ( LPSTR ) cOleName );
      if( cOleName[ 0 ] == '{' )
         lOleError = CLSIDFromString( wCLSID, ( LPCLSID ) &ClassID );
      else
         lOleError = CLSIDFromProgID( wCLSID, ( LPCLSID ) &ClassID );
      hb_xfree( wCLSID );

      if( cID )
      {
         if( cID[ 0 ] == '{' )
         {
            wCLSID = ( BSTR ) AnsiToWide( ( LPSTR ) cID );
            lOleError = CLSIDFromString( wCLSID, &iid );
            hb_xfree( wCLSID );
         }
         else if( hb_parclen( 2 ) == ( ULONG ) sizeof( iid ) )
         {
            memcpy( ( LPVOID ) &iid, cID, sizeof( iid ) );
         }
      }

      if( lOleError == S_OK )
      {
         lOleError = GetActiveObject( HB_ID_REF( ClassID ), NULL, &pUnk );

         if ( lOleError == S_OK )
            lOleError = HB_VTBL( pUnk )->QueryInterface( HB_THIS_( pUnk ) HB_ID_REF( iid ), ( void** ) ( void * ) &pDisp );
      }
   }
   else
      lOleError = CO_E_CLASSSTRING;

   hb_oleSetError( lOleError );
   if( lOleError == S_OK )
   {
      ppDisp = ( IDispatch** ) hb_gcAlloc( sizeof( IDispatch* ), hb_ole_destructor );
      *ppDisp = pDisp;
      hb_retptrGC( ppDisp );
   }
   else
      hb_ret();
}


HB_FUNC( __OLEENUMCREATE ) /* ( __hObj ) */
{
   IDispatch *    pDisp = hb_oleParam( 1 );
   IEnumVARIANT * pEnum;
   VARIANTARG     variant;
   DISPPARAMS     dispparam;
   EXCEPINFO      excep;
   UINT           uiArgErr;
   HRESULT        lOleError;

   if( hb_parl( 2 ) )
   {
      hb_oleSetError( S_OK );
      hb_errRT_BASE_SubstR( EG_UNSUPPORTED, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
      return;
   }

   memset( &excep, 0, sizeof( excep ) );
   memset( &dispparam, 0, sizeof( dispparam ) ); /* empty parameters */
   VariantInit( &variant );

   lOleError = HB_VTBL( pDisp )->Invoke( HB_THIS_( pDisp ) DISPID_NEWENUM, HB_ID_REF( IID_NULL ),
                                         LOCALE_USER_DEFAULT,
                                         DISPATCH_PROPERTYGET,
                                         &dispparam, &variant, &excep, &uiArgErr );

   if( lOleError == S_OK )
   {
      if( variant.n1.n2.vt == VT_UNKNOWN )
         lOleError = HB_VTBL( variant.n1.n2.n3.punkVal )->QueryInterface(
                            HB_THIS_( variant.n1.n2.n3.punkVal )
                            HB_ID_REF( IID_IEnumVARIANT ), ( void** ) ( void * ) &pEnum );
      else if( variant.n1.n2.vt == VT_DISPATCH )
         lOleError = HB_VTBL( variant.n1.n2.n3.pdispVal )->QueryInterface(
                            HB_THIS_( variant.n1.n2.n3.pdispVal )
                            HB_ID_REF( IID_IEnumVARIANT ), ( void** ) ( void * ) &pEnum );
      else
      {
         hb_oleSetError( lOleError );
         hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
         return;
      }

      VariantClear( &variant );

      if( lOleError == S_OK )
      {
         IEnumVARIANT**  ppEnum;

         hb_oleSetError( S_OK );

         ppEnum = ( IEnumVARIANT** ) hb_gcAlloc( sizeof( IEnumVARIANT* ), hb_oleenum_destructor );
         *ppEnum = pEnum;
         hb_retptrGC( ppEnum );
         return;
      }
   }
   hb_oleSetError( lOleError );
   hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}


HB_FUNC( __OLEENUMNEXT )
{
   IEnumVARIANT * pEnum = hb_oleenumParam( 1 );
   VARIANTARG     variant;

   VariantInit( &variant );
   if( HB_VTBL( pEnum )->Next( HB_THIS_( pEnum ) 1, &variant, NULL ) == S_OK )
   {
      hb_oleVariantToItem( hb_stackReturnItem(), &variant );
      VariantClear( &variant );
      hb_storl( TRUE, 2 );
   }
   else
      hb_storl( FALSE, 2 );
}


HB_FUNC( WIN_OLEERROR )
{
   hb_retnl( hb_oleGetError() );
}


HB_FUNC( WIN_OLEERRORTEXT )
{
   HRESULT  lOleError;

   if( HB_ISNUM( 1 ) )
      lOleError = hb_parnl( 1 );
   else
      lOleError = hb_oleGetError();

   switch( lOleError )
   {
      case S_OK:                    hb_retc_null();                              break;
      case CO_E_CLASSSTRING:        hb_retc_const( "CO_E_CLASSSTRING" );         break;
      case OLE_E_WRONGCOMPOBJ:      hb_retc_const( "OLE_E_WRONGCOMPOBJ" );       break;
      case REGDB_E_CLASSNOTREG:     hb_retc_const( "REGDB_E_CLASSNOTREG" );      break;
      case REGDB_E_WRITEREGDB:      hb_retc_const( "REGDB_E_WRITEREGDB" );       break;
      case E_OUTOFMEMORY:           hb_retc_const( "E_OUTOFMEMORY" );            break;
      case E_INVALIDARG:            hb_retc_const( "E_INVALIDARG" );             break;
      case E_UNEXPECTED:            hb_retc_const( "E_UNEXPECTED" );             break;
      case DISP_E_UNKNOWNNAME:      hb_retc_const( "DISP_E_UNKNOWNNAME" );       break;
      case DISP_E_UNKNOWNLCID:      hb_retc_const( "DISP_E_UNKNOWNLCID" );       break;
      case DISP_E_BADPARAMCOUNT:    hb_retc_const( "DISP_E_BADPARAMCOUNT" );     break;
      case DISP_E_BADVARTYPE:       hb_retc_const( "DISP_E_BADVARTYPE" );        break;
      case DISP_E_EXCEPTION:        hb_retc_const( "DISP_E_EXCEPTION" );         break;
      case DISP_E_MEMBERNOTFOUND:   hb_retc_const( "DISP_E_MEMBERNOTFOUND" );    break;
      case DISP_E_NONAMEDARGS:      hb_retc_const( "DISP_E_NONAMEDARGS" );       break;
      case DISP_E_OVERFLOW:         hb_retc_const( "DISP_E_OVERFLOW" );          break;
      case DISP_E_PARAMNOTFOUND:    hb_retc_const( "DISP_E_PARAMNOTFOUND" );     break;
      case DISP_E_TYPEMISMATCH:     hb_retc_const( "DISP_E_TYPEMISMATCH" );      break;
      case DISP_E_UNKNOWNINTERFACE: hb_retc_const( "DISP_E_UNKNOWNINTERFACE" );  break;
      case DISP_E_PARAMNOTOPTIONAL: hb_retc_const( "DISP_E_PARAMNOTOPTIONAL" );  break;
      default:
      {
         char   buf[ 16 ];

         hb_snprintf( buf, 16, "0x%08x", ( UINT ) ( HB_PTRUINT ) lOleError );
         hb_retc( buf );
      }
   }
}


HB_FUNC( WIN_OLEAUTO___ONERROR )
{
   IDispatch*  pDisp;
   const char* szMethod;
   wchar_t*    szMethodWide;
   OLECHAR*    pMemberArray;
   DISPID      dispid;
   DISPPARAMS  dispparam;
   VARIANTARG  variant;
   EXCEPINFO   excep;
   UINT        uiArgErr;
   HRESULT     lOleError;

   /* Get object handle */
   hb_vmPushDynSym( s_pDyns_hObjAccess );
   hb_vmPush( hb_stackSelfItem() );
   hb_vmSend( 0 );

   pDisp = hb_oleParam( -1 );
   if( !pDisp )
      return;

   szMethod = hb_itemGetSymbol( hb_stackBaseItem() )->szName;
   szMethodWide = AnsiToWide( szMethod );

   /* Try property put */

   if( szMethod[ 0 ] == '_' && hb_pcount() > 0 )
   {
      pMemberArray = &szMethodWide[ 1 ];
      lOleError = HB_VTBL( pDisp )->GetIDsOfNames( HB_THIS_( pDisp ) HB_ID_REF( IID_NULL ), &pMemberArray,
                                                   1, LOCALE_USER_DEFAULT, &dispid );

      if( lOleError == S_OK )
      {
         DISPID     lPropPut = DISPID_PROPERTYPUT;

         memset( &excep, 0, sizeof( excep ) );
         GetParams( &dispparam );
         dispparam.rgdispidNamedArgs = &lPropPut;
         dispparam.cNamedArgs = 1;

         lOleError = HB_VTBL( pDisp )->Invoke( HB_THIS_( pDisp ) dispid, HB_ID_REF( IID_NULL ),
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_PROPERTYPUT, &dispparam,
                                               NULL, &excep, &uiArgErr );
         FreeParams( &dispparam );
         hb_xfree( szMethodWide );

         /* assign method should return assigned value */
         hb_itemReturn( hb_param( 1, HB_IT_ANY ) );

         hb_oleSetError( lOleError );
         if( lOleError != S_OK )
            hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
         return;
      }
   }

   /* Try property get and invoke */

   pMemberArray = szMethodWide;
   lOleError = HB_VTBL( pDisp )->GetIDsOfNames( HB_THIS_( pDisp ) HB_ID_REF( IID_NULL ),
                                                &pMemberArray, 1, LOCALE_USER_DEFAULT, &dispid );
   hb_xfree( szMethodWide );

   if( lOleError == S_OK )
   {
      memset( &excep, 0, sizeof( excep ) );
      VariantInit( &variant );
      GetParams( &dispparam );

      lOleError = HB_VTBL( pDisp )->Invoke( HB_THIS_( pDisp ) dispid, HB_ID_REF( IID_NULL ),
                                            LOCALE_USER_DEFAULT,
                                            DISPATCH_PROPERTYGET | DISPATCH_METHOD,
                                            &dispparam, &variant, &excep, &uiArgErr );

#ifdef _PUTPARAMS_
      PutParams( &dispparam );
#endif
      FreeParams( &dispparam );

      hb_oleVariantToItem( hb_stackReturnItem(), &variant );
      VariantClear( &variant );

      hb_oleSetError( lOleError );
      if( lOleError != S_OK )
         hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
      return;
   }

   hb_oleSetError( lOleError );

   /* TODO: add description containing TypeName of the object */
   if( szMethod[ 0 ] == '_' )
      hb_errRT_BASE_SubstR( EG_NOVARMETHOD, 1005, NULL, szMethod + 1, HB_ERR_ARGS_BASEPARAMS );
   else
      hb_errRT_BASE_SubstR( EG_NOMETHOD, 1004, NULL, szMethod, HB_ERR_ARGS_BASEPARAMS );
}


HB_CALL_ON_STARTUP_BEGIN( _hb_olecore_init_ )
   hb_vmAtInit( hb_olecore_init, NULL );
HB_CALL_ON_STARTUP_END( _hb_olecore_init_ )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup _hb_olecore_init_
#elif defined( HB_MSC_STARTUP )
   #if defined( HB_OS_WIN_64 )
      #pragma section( HB_MSC_START_SEGMENT, long, read )
   #endif
   #pragma data_seg( HB_MSC_START_SEGMENT )
   static HB_$INITSYM hb_vm_auto_olecore_init = _hb_olecore_init_;
   #pragma data_seg()
#endif
