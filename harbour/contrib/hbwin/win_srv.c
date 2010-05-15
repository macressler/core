/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    Windows Service
 *
 * Copyright 2010 Jose Luis Capel - <jlcapel at hotmail . com>
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

#include "hbapi.h"
#include "hbvm.h"
#include "hbwinuni.h"

#include "hbwapi.h"

#if ! defined( HB_OS_WIN_CE )

static SERVICE_STATUS        s_ServiceStatus;
static SERVICE_STATUS_HANDLE s_hStatus;
static char                  s_szHarbourEntryFunc[ 64 ];
static TCHAR                 s_lpServiceName[ 64 ];

/* Control handler function */
static void hbwin_ControlHandler( DWORD request )
{
   switch( request )
   {
      case SERVICE_CONTROL_STOP:
         s_ServiceStatus.dwWin32ExitCode = 0;
         s_ServiceStatus.dwCurrentState  = SERVICE_STOPPED;
         return;

      case SERVICE_CONTROL_SHUTDOWN:
         s_ServiceStatus.dwWin32ExitCode = 0;
         s_ServiceStatus.dwCurrentState  = SERVICE_STOPPED;
         return;
   }

   SetServiceStatus( s_hStatus, &s_ServiceStatus ); /* Report current status */
}

static void hbwin_SrvFunction( int argc, char** argv )
{
   HB_SYMBOL_UNUSED( argc );
   HB_SYMBOL_UNUSED( argv );

   s_ServiceStatus.dwServiceType             = SERVICE_WIN32;
   s_ServiceStatus.dwCurrentState            = SERVICE_START_PENDING;
   s_ServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
   s_ServiceStatus.dwWin32ExitCode           = 0;
   s_ServiceStatus.dwServiceSpecificExitCode = 0;
   s_ServiceStatus.dwCheckPoint              = 0;
   s_ServiceStatus.dwWaitHint                = 0;

   s_hStatus = RegisterServiceCtrlHandler( s_lpServiceName, ( LPHANDLER_FUNCTION ) hbwin_ControlHandler );

   if( s_hStatus != ( SERVICE_STATUS_HANDLE ) 0 )
   {
      PHB_DYNS pDynSym = hb_dynsymFindName( s_szHarbourEntryFunc );

      /* We report the running status to SCM. */
      s_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
      SetServiceStatus( s_hStatus, &s_ServiceStatus );

      if( pDynSym )
      {
         if( hb_vmRequestReenter() )
         {
            hb_vmPushSymbol( hb_dynsymSymbol( pDynSym ) );
            hb_vmPushNil();
            hb_vmDo( 0 );
            hb_vmRequestRestore();
         }
      }
   }
   else
   {
      HB_TRACE( HB_TR_DEBUG, ("Error registering service") );
   }
}

#endif

HB_FUNC( WIN_SERVICEGETSTATUS )
{
#if ! defined( HB_OS_WIN_CE )
   hb_retnl( s_ServiceStatus.dwCurrentState );
#else
   hb_retnl( 0 );
#endif
}

HB_FUNC( WIN_SERVICESETSTATUS ) /* dwStatus */
{
#if ! defined( HB_OS_WIN_CE )
   s_ServiceStatus.dwCurrentState = ( DWORD ) hb_parnl( 1 );
   hb_retl( SetServiceStatus( s_hStatus, &s_ServiceStatus ) );
#else
   hb_retl( HB_FALSE );
#endif
}

HB_FUNC( WIN_SERVICESETEXITCODE ) /* dwExitCode */
{
#if ! defined( HB_OS_WIN_CE )
   s_ServiceStatus.dwWin32ExitCode = ( DWORD ) hb_parnl( 1 );
   hb_retl( SetServiceStatus( s_hStatus, &s_ServiceStatus ) );
#else
   hb_retl( HB_FALSE );
#endif
}

HB_FUNC( WIN_SERVICESTOP )
{
#if ! defined( HB_OS_WIN_CE )
   s_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
   SetServiceStatus( s_hStatus, &s_ServiceStatus );
#endif
}

HB_FUNC( WIN_SERVICEINSTALL )
{
   HB_BOOL bRetVal = HB_FALSE;
#if ! defined( HB_OS_WIN_CE )
   TCHAR lpPath[ MAX_PATH ];

   if( GetModuleFileName( NULL, lpPath, HB_SIZEOFARRAY( lpPath ) ) )
   {
      SC_HANDLE schSCM = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

      if( schSCM )
      {
         SC_HANDLE schSrv;

         void * hServiceName;
         void * hDisplayName;

         LPCTSTR lpServiceName = HB_PARSTRDEF( 1, &hServiceName, NULL );
         LPCTSTR lpDisplayName = HB_PARSTRDEF( 2, &hDisplayName, NULL );

         schSrv = CreateService( schSCM,                       /* SCM database */
                                 lpServiceName,                /* name of service */
                                 lpDisplayName,                /* service name to display */
                                 SERVICE_ALL_ACCESS,           /* desired access */
                                 SERVICE_WIN32_OWN_PROCESS,    /* service type */
                                 SERVICE_DEMAND_START,         /* start type */
                                 SERVICE_ERROR_NORMAL,         /* error control type */
                                 lpPath,                       /* path to service's binary */
                                 NULL,                         /* no load ordering group */
                                 NULL,                         /* no tag identifier */
                                 NULL,                         /* no dependencies */
                                 NULL,                         /* LocalSystem account */
                                 NULL );                       /* no password */

         if( schSrv )
         {
            bRetVal = HB_TRUE;

            CloseServiceHandle( schSrv );
         }
         else
            hbwapi_SetLastError( GetLastError() );

         hb_strfree( hServiceName );
         hb_strfree( hDisplayName );

         CloseServiceHandle( schSCM );
      }
      else
         hbwapi_SetLastError( GetLastError() );
   }
   else
      hbwapi_SetLastError( GetLastError() );

#endif
   hb_retl( bRetVal );
}

HB_FUNC( WIN_SERVICEDELETE ) /* sServiceName */
{
   HB_BOOL bRetVal = HB_FALSE;
#if ! defined( HB_OS_WIN_CE )
   SC_HANDLE schSCM = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

   if( schSCM )
   {
      void * hServiceName;

      SC_HANDLE schSrv = OpenService( schSCM,
                                      HB_PARSTRDEF( 1, &hServiceName, NULL ),
                                      SERVICE_ALL_ACCESS );

      if( schSrv )
      {
         /* TODO: check if service is up and then stop it */

         bRetVal = ( HB_BOOL ) DeleteService( schSrv );

         CloseServiceHandle( schSrv );
      }
      else
         hbwapi_SetLastError( GetLastError() );

      hb_strfree( hServiceName );

      CloseServiceHandle( schSCM );
   }
   else
      hbwapi_SetLastError( GetLastError() );
#endif
   hb_retl( bRetVal );
}

HB_FUNC( WIN_SERVICESTART ) /* pszServiceName, pszPrgFunction */
{
   HB_BOOL bRetVal = HB_FALSE;
#if ! defined( HB_OS_WIN_CE )

   SERVICE_TABLE_ENTRY lpServiceTable[ 2 ];

   HB_TCHAR_COPYTO( s_lpServiceName, hb_parcx( 1 ), HB_SIZEOFARRAY( s_lpServiceName ) - 1 );
   hb_strncpy( s_szHarbourEntryFunc, hb_parcx( 2 ), HB_SIZEOFARRAY( s_szHarbourEntryFunc ) - 1 );

   lpServiceTable[ 0 ].lpServiceName = s_lpServiceName;
   lpServiceTable[ 0 ].lpServiceProc = ( LPSERVICE_MAIN_FUNCTION ) hbwin_SrvFunction;

   lpServiceTable[ 1 ].lpServiceName = NULL;
   lpServiceTable[ 1 ].lpServiceProc = NULL;

   if( StartServiceCtrlDispatcher( lpServiceTable ) )
      bRetVal = HB_TRUE;
   else
      hbwapi_SetLastError( GetLastError() );

#endif
   hb_retl( bRetVal );
}
