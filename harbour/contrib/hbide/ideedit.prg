/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *
 * Copyright 2009-2010 Pritpal Bedi <bedipritpal@hotmail.com>
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
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*
 *                                EkOnkar
 *                          ( The LORD is ONE )
 *
 *                            Harbour-Qt IDE
 *
 *                  Pritpal Bedi <pritpal@vouchcac.com>
 *                               27Dec2009
 */
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/

#include "common.ch"
#include "hbclass.ch"
#include "hbqt.ch"
#include "hbide.ch"
#include "xbp.ch"

/*----------------------------------------------------------------------*/

#define customContextMenuRequested                1
#define textChanged                               2
#define copyAvailable                             3
#define modificationChanged                       4
#define redoAvailable                             5
#define selectionChanged                          6
#define undoAvailable                             7
#define updateRequest                             8
#define cursorPositionChanged                     9

#define timerTimeout                              23

/*----------------------------------------------------------------------*/

CLASS IdeEdit INHERIT IdeObject

   DATA   oEditor

   DATA   qEdit
   DATA   qHLayout
   DATA   nOrient                                 INIT  0

   DATA   nMode                                   INIT  0
   DATA   nLineNo                                 INIT  -99
   DATA   nMaxDigits                              INIT  5       // Tobe
   DATA   nMaxRows                                INIT  100
   DATA   nLastLine                               INIT  -99
   DATA   nCurLineNo                              INIT  0
   DATA   nPrevLineNo                             INIT  -1
   DATA   nPrevLineNo1                            INIT  -1

   DATA   aBookMarks                              INIT  {}

   DATA   lModified                               INIT  .F.
   DATA   lIndentIt                               INIT  .f.
   DATA   lUpdatePrevWord                         INIT  .f.
   DATA   lCopyWhenDblClicked                     INIT  .f.
   DATA   cCurLineText                            INIT  ""

   DATA   cProto                                  INIT ""
   DATA   qTimer
   DATA   nProtoLine                              INIT -1
   DATA   nProtoCol                               INIT -1
   DATA   isSuspended                             INIT .f.

   DATA   fontFamily                              INIT "Courier New"
   DATA   pointSize                               INIT 10
   DATA   currentPointSize                        INIT 10
   DATA   qFont
   DATA   aBlockCopyContents                      INIT {}
   DATA   isLineSelectionMode                     INIT .f.
   DATA   aSelectionInfo                          INIT { -1,-1,-1,-1,0 }

   METHOD new( oEditor, nMode )
   METHOD create( oEditor, nMode )
   METHOD destroy()
   METHOD execEvent( nMode, oEdit, p, p1 )
   METHOD execKeyEvent( nMode, nEvent, p, p1 )
   METHOD connectEditSignals( oEdit )
   METHOD disconnectEditSignals( oEdit )

   METHOD redo()
   METHOD undo()
   METHOD cut()
   METHOD copy()
   METHOD paste()
   METHOD selectAll()
   METHOD toggleSelectionMode()

   METHOD setReadOnly()
   METHOD setNewMark()
   METHOD gotoMark( nIndex )
   METHOD duplicateLine()
   METHOD deleteLine()
   METHOD blockComment()
   METHOD streamComment()
   METHOD blockIndent( nMode )
   METHOD moveLine( nDirection )
   METHOD caseUpper()
   METHOD caseLower()
   METHOD caseInvert()
   METHOD convertQuotes()
   METHOD convertDQuotes()
   METHOD findLastIndent()
   METHOD reLayMarkButtons()
   METHOD presentSkeletons()
   METHOD handleCurrentIndent()
   METHOD handlePreviousWord( lUpdatePrevWord )
   METHOD loadFuncHelp()
   METHOD clickFuncHelp()
   METHOD goto( nLine )
   METHOD gotoFunction()
   METHOD toggleLineNumbers()
   METHOD toggleLineSelectionMode()

   METHOD getWord( lSelect )
   METHOD getLine( nLine, lSelect )
   METHOD getText()
   METHOD getSelectedText()
   METHOD getColumnNo()
   METHOD getLineNo()
   METHOD insertSeparator( cSep )
   METHOD insertText( cText )

   METHOD suspendPrototype()
   METHOD resumePrototype()
   METHOD showPrototype( cProto )
   METHOD hidePrototype()
   METHOD completeCode( p )

   METHOD setLineNumbersBkColor( nR, nG, nB )
   METHOD setCurrentLineColor( nR, nG, nB )
   METHOD getCursor()                             INLINE QTextCursor():from( ::qEdit:textCursor() )
   METHOD down()
   METHOD up()
   METHOD home()
   METHOD find( cText, nPosFrom )
   METHOD refresh()
   METHOD isModified()                            INLINE ::oEditor:qDocument:isModified()
   METHOD setFont()
   METHOD markCurrentFunction()
   METHOD copyBlockContents( aCord )
   METHOD pasteBlockContents( nMode )
   METHOD insertBlockContents( aCord )
   METHOD deleteBlockContents( aCord )
   METHOD zoom( nKey )

   ENDCLASS

/*----------------------------------------------------------------------*/

METHOD IdeEdit:new( oEditor, nMode )

   ::oEditor := oEditor
   ::nMode   := nMode

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:create( oEditor, nMode )
   LOCAL nBlock

   DEFAULT oEditor TO ::oEditor
   DEFAULT nMode   TO ::nMode

   ::oEditor := oEditor
   ::nMode   := nMode
   ::oIde    := ::oEditor:oIde

   ::qEdit   := HBQPlainTextEdit():new()
   //
   ::qEdit:setLineWrapMode( QTextEdit_NoWrap )
   ::qEdit:ensureCursorVisible()
   ::qEdit:setContextMenuPolicy( Qt_CustomContextMenu )
   ::qEdit:installEventFilter( ::pEvents )
   ::qEdit:setTabChangesFocus( .f. )

   ::setFont()

   ::qEdit:hbHighlightCurrentLine( .t. )              /* Via user-setup */
   ::qEdit:hbSetSpaces( ::nTabSpaces )

   ::qEdit:hbSetCompleter( ::qCompleter )

   ::toggleLineNumbers()

   FOR EACH nBlock IN ::aBookMarks
      ::qEdit:hbBookMarks( nBlock )
   NEXT

   ::qHLayout := QHBoxLayout():new()
   ::qHLayout:setSpacing( 0 )

   ::qHLayout:addWidget( ::qEdit )

   ::connectEditSignals( Self )

   Qt_Events_Connect( ::pEvents, ::qEdit, QEvent_KeyPress           , {|p| ::execKeyEvent( 101, QEvent_KeyPress, p ) } )
   Qt_Events_Connect( ::pEvents, ::qEdit, QEvent_Wheel              , {|p| ::execKeyEvent( 102, QEvent_Wheel   , p ) } )
   Qt_Events_Connect( ::pEvents, ::qEdit, QEvent_FocusIn            , {| | ::execKeyEvent( 104, QEvent_FocusIn     ) } )
   Qt_Events_Connect( ::pEvents, ::qEdit, QEvent_FocusOut           , {| | ::execKeyEvent( 105, QEvent_FocusOut    ) } )
   Qt_Events_Connect( ::pEvents, ::qEdit, QEvent_MouseButtonDblClick, {|p| ::execKeyEvent( 103, QEvent_MouseButtonDblClick, p ) } )

   ::qEdit:hbSetEventBlock( {|p,p1| ::execKeyEvent( 115, 1001, p, p1 ) } )

   ::qTimer := QTimer():new()
   ::qTimer:setInterval( 2000 )
   ::connect( ::qTimer, "timeout()",  {|| ::execEvent( timerTimeout, Self ) } )

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:zoom( nKey )

   DEFAULT nKey TO 0

   IF nKey == 1
      IF ::currentPointSize + 1 < 30
         ::currentPointSize++
      ENDIF

   ELSEIF nKey == -1
      IF ::currentPointSize - 1 > 5
         ::currentPointSize--
      ENDIF

   ELSEIF nKey == 0
      ::currentPointSize := ::pointSize

   ELSEIF nKey >= 5 .AND. nKey <= 30
      ::currentPointSize := nKey

   ELSE
      RETURN Self

   ENDIF

   ::setFont()

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:setFont()

   ::qFont := QFont():new()
   ::qFont:setFamily( ::fontFamily )
   ::qFont:setFixedPitch( .t. )
   ::qFont:setPointSize( ::currentPointSize )

   ::qEdit:setFont( ::qFont )

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:destroy()

   ::disconnect( ::qTimer, "timeout()" )
   IF ::qTimer:isActive()
      ::qTimer:stop()
   ENDIF
   ::qTimer := NIL

   Qt_Events_DisConnect( ::pEvents, ::qEdit, QEvent_KeyPress            )
   Qt_Events_DisConnect( ::pEvents, ::qEdit, QEvent_Wheel               )
   Qt_Events_DisConnect( ::pEvents, ::qEdit, QEvent_FocusIn             )
   Qt_Events_DisConnect( ::pEvents, ::qEdit, QEvent_FocusOut            )
   Qt_Events_DisConnect( ::pEvents, ::qEdit, QEvent_MouseButtonDblClick )

   ::disconnectEditSignals( Self )

   ::oEditor:qLayout:removeItem( ::qHLayout )
   //
   ::qHLayout:removeWidget( ::qEdit )
   ::qEdit    := NIL
   ::qHLayout := NIL
   ::qFont    := NIL

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:disconnectEditSignals( oEdit )
   HB_SYMBOL_UNUSED( oEdit )

   ::disConnect( oEdit:qEdit, "customContextMenuRequested(QPoint)" )
   ::disConnect( oEdit:qEdit, "textChanged()"                      )
   ::disConnect( oEdit:qEdit, "selectionChanged()"                 )
   ::disConnect( oEdit:qEdit, "cursorPositionChanged()"            )
   ::disConnect( oEdit:qEdit, "copyAvailable(bool)"                )

   #if 0
   ::disConnect( oEdit:qEdit, "modificationChanged(bool)"          )
   ::disConnect( oEdit:qEdit, "updateRequest(QRect,int)"           )
   ::disConnect( oEdit:qEdit, "redoAvailable(bool)"                )
   ::disConnect( oEdit:qEdit, "undoAvailable(bool)"                )
   #endif

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:connectEditSignals( oEdit )
   HB_SYMBOL_UNUSED( oEdit )

   ::connect( oEdit:qEdit, "customContextMenuRequested(QPoint)", {|p   | ::execEvent( 1, oEdit, p     ) } )
   ::Connect( oEdit:qEdit, "textChanged()"                     , {|    | ::execEvent( 2, oEdit,       ) } )
   ::Connect( oEdit:qEdit, "selectionChanged()"                , {|p   | ::execEvent( 6, oEdit, p     ) } )
   ::Connect( oEdit:qEdit, "cursorPositionChanged()"           , {|    | ::execEvent( 9, oEdit,       ) } )
   ::Connect( oEdit:qEdit, "copyAvailable(bool)"               , {|p   | ::execEvent( 3, oEdit, p     ) } )

   #if 0
   ::Connect( oEdit:qEdit, "modificationChanged(bool)"         , {|p   | ::execEvent( 4, oEdit, p     ) } )
   ::Connect( oEdit:qEdit, "updateRequest(QRect,int)"          , {|p,p1| ::execEvent( 8, oEdit, p, p1 ) } )
   ::Connect( oEdit:qEdit, "redoAvailable(bool)"               , {|p   | ::execEvent( 5, oEdit, p     ) } )
   ::Connect( oEdit:qEdit, "undoAvailable(bool)"               , {|p   | ::execEvent( 7, oEdit, p     ) } )
   #endif

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:execEvent( nMode, oEdit, p, p1 )
   LOCAL pAct, qAct, n, qCursor, qEdit, oo, nLine

   HB_SYMBOL_UNUSED( p1 )

   qEdit   := oEdit:qEdit
   qCursor := QTextCursor():configure( qEdit:textCursor() )
   oEdit:nCurLineNo := qCursor:blockNumber()

   SWITCH nMode

   CASE customContextMenuRequested
      QAction():from( ::oEM:aActions[ 17, 2 ] ):setEnabled( !empty( qCursor:selectedText() ) )

      pAct := ::oEM:qContextMenu:exec_1( qEdit:mapToGlobal( p ) )
      IF !hbqt_isEmptyQtPointer( pAct )
         qAct := QAction():configure( pAct )
         DO CASE
         CASE qAct:text() == "Split Horizontally"
            ::oEditor:split( 1, oEdit )
         CASE qAct:text() == "Split Vertically"
            ::oEditor:split( 2, oEdit )
         CASE qAct:text() == "Close Split Window"
            IF ( n := ascan( ::oEditor:aEdits, {|o| o == oEdit } ) ) > 0  /* 1 == Main Edit */
               oo := ::oEditor:aEdits[ n ]
               hb_adel( ::oEditor:aEdits, n, .t. )
               oo:destroy()
               ::oEditor:relay()
               ::oEditor:qCqEdit := ::oEditor:qEdit
               ::oEditor:qCoEdit := ::oEditor:oEdit
               ::oIde:manageFocusInEditor()
            ENDIF
         CASE qAct:text() == "Save as Skeleton..."
            ::oSK:saveAs( ::getSelectedText() )
         CASE qAct:text() == "Apply Theme"
            ::oEditor:applyTheme()
         CASE qAct:text() == "Goto Function"
            ::gotoFunction()
         ENDCASE
      ENDIF
      EXIT

   CASE textChanged
      //HB_TRACE( HB_TR_ALWAYS, "textChanged()" )
      ::oEditor:setTabImage( qEdit )
      EXIT

   CASE selectionChanged
      //HB_TRACE( HB_TR_ALWAYS, "selectionChanged()" )
      ::oEditor:qCqEdit := qEdit
      ::oEditor:qCoEdit := oEdit

      qCursor := QTextCursor():configure( qEdit:TextCursor() )

      /* Book Marks reach-out buttons */
      ::relayMarkButtons()
      ::toggleLineNumbers()

      ::updateTitleBar()

      /* An experimental move but seems a lot is required to achieve column selection */
      qEdit:hbHighlightSelectedColumns( ::isColumnSelectionEnabled )

      ::oDK:setStatusText( SB_PNL_SELECTEDCHARS, len( qCursor:selectedText() ) )
      EXIT

   CASE cursorPositionChanged
      //HB_TRACE( HB_TR_ALWAYS, "cursorPositionChanged()", ::nProtoLine, ::nProtoCol, ::isSuspended, ::getLineNo(), ::getColumnNo(), ::cProto )
      ::oEditor:dispEditInfo( qEdit )
      ::handlePreviousWord( ::lUpdatePrevWord )
      ::handleCurrentIndent()

      ::markCurrentFunction()

      IF ::nProtoLine != -1
         nLine := ::getLineNo()
         IF ! ::isSuspended
            IF nLine != ::nProtoLine .OR. ::getColumnNo() <= ::nProtoCol
               ::suspendPrototype()
            ENDIF
         ELSE
            IF nLine == ::nProtoLine .AND. ::getColumnNo() >= ::nProtoCol
               ::resumePrototype()
            ENDIF
         ENDIF
      ENDIF

      EXIT

   CASE copyAvailable
      IF p .AND. ::lCopyWhenDblClicked
         ::qEdit:copy()
      ENDIF
      ::lCopyWhenDblClicked := .f.
      EXIT

   CASE timerTimeout
      IF empty( ::cProto )
         ::hidePrototype()
      ELSE
         ::showPrototype()
      ENDIF
      EXIT

   #if 0
   CASE modificationChanged
      //HB_TRACE( HB_TR_ALWAYS, "modificationChanged(bool)", p )
      EXIT
   CASE redoAvailable
      //HB_TRACE( HB_TR_ALWAYS, "redoAvailable(bool)", p )
      EXIT
   CASE undoAvailable
      //HB_TRACE( HB_TR_ALWAYS, "undoAvailable(bool)", p )
      EXIT
   CASE updateRequest
      EXIT
   #endif
   ENDSWITCH

   RETURN Nil

/*----------------------------------------------------------------------*/

METHOD IdeEdit:execKeyEvent( nMode, nEvent, p, p1 )
   LOCAL key, kbm, txt, qEvent
   LOCAL lAlt   := .f.
   LOCAL lCtrl  := .f.
   LOCAL lShift := .f.

   p1 := p1

   SWITCH nEvent
   CASE QEvent_KeyPress

      qEvent := QKeyEvent():configure( p )

      key := qEvent:key()
      kbm := qEvent:modifiers()
      txt := qEvent:text()

      IF hb_bitAnd( kbm, Qt_AltModifier     ) == Qt_AltModifier
         lAlt := .t.
      ENDIF
      IF hb_bitAnd( kbm, Qt_ControlModifier ) == Qt_ControlModifier
         lCtrl := .t.
      ENDIF
      IF hb_bitAnd( kbm, Qt_ShiftModifier   ) == Qt_ShiftModifier
         lShift := .t.
      ENDIF

      IF ::oSC:execKey( key, lAlt, lCtrl, lShift )
         RETURN .f.
      ENDIF

      SWITCH ( key )
      CASE Qt_Key_Space
         IF !lAlt .AND. !lShift .AND. !lCtrl
            ::lUpdatePrevWord := .t.
         ENDIF
         EXIT
      CASE Qt_Key_Return
      CASE Qt_Key_Enter
         ::handlePreviousWord( .t. )
         ::lIndentIt := .t.
         EXIT
      CASE Qt_Key_Tab
         IF lCtrl
            ::blockIndent( 1 )
            RETURN .T.
         ENDIF
         EXIT
      CASE Qt_Key_Backtab
         IF lCtrl
            ::blockIndent( -1 )
            RETURN .t.
         ENDIF
         EXIT
      CASE Qt_Key_Q                   /* All these actions will be pulled from user-setup */
         IF lCtrl .AND. lShift
            ::streamComment()
         ENDIF
         EXIT
      CASE Qt_Key_Slash
         IF lCtrl
            ::blockComment()
         ENDIF
         EXIT
      CASE Qt_Key_D
         IF lCtrl
            ::duplicateLine()
         ENDIF
         EXIT
      CASE Qt_Key_K
         IF lCtrl
            ::presentSkeletons()
         ENDIF
         EXIT
      CASE Qt_Key_Backspace
         hbide_justACall( txt, lAlt, lShift, lCtrl, qEvent, nMode )
         EXIT
      CASE Qt_Key_Delete
         IF lCtrl
            ::deleteLine()
            RETURN .t.
         ENDIF
         EXIT
      CASE Qt_Key_Up
         IF lCtrl .AND. lShift
            ::moveLine( -1 )
            RETURN .t.
         ENDIF
         EXIT
      CASE Qt_Key_Down
         IF lCtrl .AND. lShift
            ::moveLine( 1 )
            RETURN .t.
         ENDIF
         EXIT
      CASE Qt_Key_ParenLeft
         IF ! lCtrl .AND. ! lAlt
            ::loadFuncHelp()     // Also invokes prototype display
         ENDIF
         EXIT
      CASE Qt_Key_ParenRight
         IF ! lCtrl .AND. ! lAlt
            ::hidePrototype()
         ENDIF
         EXIT
      CASE Qt_Key_T
         IF lCtrl
            ::gotoFunction()
         ENDIF
         EXIT
      CASE Qt_Key_F1
         ::gotoFunction()
         EXIT
      ENDSWITCH

      EXIT

   CASE QEvent_Enter
   CASE QEvent_FocusIn
      ::resumePrototype()
      EXIT

   CASE QEvent_Leave
   CASE QEvent_FocusOut
      ::suspendPrototype()
      EXIT

   CASE QEvent_Wheel
      EXIT

   CASE QEvent_MouseButtonDblClick
      ::lCopyWhenDblClicked := .t.
      EXIT

   CASE 1001
      IF p == QEvent_MouseButtonDblClick
         ::lCopyWhenDblClicked := .f.       /* not intuitive */
         ::clickFuncHelp()

      ELSEIF p == QEvent_Paint
         // ::oIde:testPainter( p1 )

      ELSEIF p == 21000
         ::aSelectionInfo := p1

      ELSEIF p == 21001
         ::handlePreviousWord( .t. )

      ELSEIF p == 21011
         ::copyBlockContents( p1 )

      ELSEIF p == 21012
         ::pasteBlockContents( p1 )

      ELSEIF p == 21013
         ::insertBlockContents( p1 )

      ELSEIF p == 21014
         ::deleteBlockContents( p1 )

      ENDIF
      EXIT

   ENDSWITCH

   RETURN .F.  /* Important */

/*----------------------------------------------------------------------*/

METHOD IdeEdit:copyBlockContents( aCord )
   LOCAL nT, nL, nB, nR, nW, i, cLine, nMode
   LOCAL cClip := ""

   hbide_normalizeRect( aCord, @nT, @nL, @nB, @nR )
   nMode := aCord[ 5 ]

   nW := nR - nL
   FOR i := nT TO nB
      cLine := ::getLine( i + 1 )

      IF nMode == 1      /* Stream */
         IF i == 1
            cLine := substr( cLine, nL + 1 )
         ELSEIF i == nB
            cLine := substr( cLine, 1, nR + 1 )
         ENDIF

      ELSEIF nMode == 2  /* Column */
         cLine := pad( substr( cLine, nL + 1, nW ), nW )

      ELSEIF nMode == 3  /* Line   */
         // Nothing to do, complete line is already pulled

      ENDIF
      cClip += cLine + hb_osNewLine()
   NEXT

   QClipboard():new():setText( cClip )

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:pasteBlockContents( nMode )
   LOCAL i, nRow, nCol, qCursor, nMaxCol, aBlockCopyContents

   HB_SYMBOL_UNUSED( nMode )

   aBlockCopyContents := hbide_memoToArray( QClipboard():new():text() )
   IF empty( aBlockCopyContents )
      RETURN Self
   ENDIF

   qCursor := QTextCursor():from( ::qEdit:textCursor() )
   nCol    := qCursor:columnNumber()
   qCursor:beginEditBlock()
   //
   FOR i := 1 TO len( aBlockCopyContents )
      qCursor:insertText( aBlockCopyContents[ i ] )
      IF i < len( aBlockCopyContents )
         nRow := qCursor:blockNumber()
         qCursor:movePosition( QTextCursor_Down, QTextCursor_MoveAnchor )
         IF qCursor:blockNumber() == nRow
            qCursor:movePosition( QTextCursor_EndOfBlock, QTextCursor_MoveAnchor )
            qCursor:insertBlock()
            qCursor:movePosition( QTextCursor_NextBlock, QTextCursor_MoveAnchor )
         ENDIF
         qCursor:movePosition( QTextCursor_EndOfLine, QTextCursor_MoveAnchor )
         nMaxCol := qCursor:columnNumber()
         IF nMaxCol < nCol
            qCursor:insertText( replicate( " ", nCol - nMaxCol ) )
         ENDIF
         qCursor:movePosition( QTextCursor_StartOfBlock, QTextCursor_MoveAnchor )
         qCursor:movePosition( QTextCursor_Right, QTextCursor_MoveAnchor, nCol )
      ENDIF
   NEXT
   //
   qCursor:endEditBlock()

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:insertBlockContents( aCord )
   LOCAL nT, nL, nB, nR, nW, i, cLine, cKey, qCursor

   hbide_normalizeRect( aCord, @nT, @nL, @nB, @nR )

   nW := nR - nL

   cKey := chr( XbpQKeyEventToAppEvent( aCord[ 5 ] ) )

   qCursor := QTextCursor():from( ::qEdit:textCursor() )
   qCursor:beginEditBlock()

   IF nW == 0
      FOR i := nT TO nB
         cLine := ::getLine( i + 1 )
         cLine := pad( substr( cLine, 1, nL ), nL ) + cKey + substr( cLine, nL + 1 )

         qCursor:movePosition( QTextCursor_Start       , QTextCursor_MoveAnchor )
         qCursor:movePosition( QTextCursor_Down        , QTextCursor_MoveAnchor, i )
         qCursor:movePosition( QTextCursor_StartOfBlock, QTextCursor_MoveAnchor )
         qCursor:movePosition( QTextCursor_EndOfLine   , QTextCursor_KeepAnchor )
         qCursor:insertText( cLine )
      NEXT
      qCursor:movePosition( QTextCursor_Start, QTextCursor_MoveAnchor )
      qCursor:movePosition( QTextCursor_Down , QTextCursor_MoveAnchor, nB )
      qCursor:movePosition( QTextCursor_Right, QTextCursor_MoveAnchor, nR + 1 )
   ELSE
      FOR i := nT TO nB
         cLine := ::getLine( i + 1 )
         cLine := pad( substr( cLine, 1, nL ), nL ) + replicate( cKey, nW ) + substr( cLine, nR + 1 )

         qCursor:movePosition( QTextCursor_Start       , QTextCursor_MoveAnchor )
         qCursor:movePosition( QTextCursor_Down        , QTextCursor_MoveAnchor, i )
         qCursor:movePosition( QTextCursor_StartOfBlock, QTextCursor_MoveAnchor )
         qCursor:movePosition( QTextCursor_EndOfLine   , QTextCursor_KeepAnchor )
         qCursor:insertText( cLine )
      NEXT
      qCursor:movePosition( QTextCursor_Start, QTextCursor_MoveAnchor )
      qCursor:movePosition( QTextCursor_Down , QTextCursor_MoveAnchor, nB )
      qCursor:movePosition( QTextCursor_Right, QTextCursor_MoveAnchor, nR )
   ENDIF
   //
   ::qEdit:setCursorWidth( 1 )
   ::qEdit:setTextCursor( qCursor )
   qCursor:endEditBlock()

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:deleteBlockContents( aCord )
   LOCAL nT, nL, nB, nR, nW, i, cLine, qCursor, k

   hbide_normalizeRect( aCord, @nT, @nL, @nB, @nR )
   k  := aCord[ 5 ]

   IF k == Qt_Key_X
      ::copyBlockContents( aCord )
   ENDIF

   nW := nR - nL

   qCursor := QTextCursor():from( ::qEdit:textCursor() )
   qCursor:beginEditBlock()
   IF nW == 0 .AND. k == Qt_Key_Backspace
      FOR i := nT TO nB
         cLine := ::getLine( i + 1 )
         cLine := pad( substr( cLine, 1, nL - 1 ), nL - 1 ) + substr( cLine, nL + 1 )

         qCursor:movePosition( QTextCursor_Start       , QTextCursor_MoveAnchor )
         qCursor:movePosition( QTextCursor_Down        , QTextCursor_MoveAnchor, i )
         qCursor:movePosition( QTextCursor_StartOfLine , QTextCursor_MoveAnchor )
         qCursor:movePosition( QTextCursor_EndOfLine   , QTextCursor_KeepAnchor )
         qCursor:insertText( cLine )
      NEXT
      qCursor:movePosition( QTextCursor_Start, QTextCursor_MoveAnchor )
      qCursor:movePosition( QTextCursor_Down , QTextCursor_MoveAnchor, nB )
      qCursor:movePosition( QTextCursor_Right, QTextCursor_MoveAnchor, nR - 1 )
   ELSE
      IF k == Qt_Key_Delete .OR. k == Qt_Key_X
         FOR i := nT TO nB
            cLine := ::getLine( i + 1 )
            cLine := pad( substr( cLine, 1, nL ), nL ) + substr( cLine, nR + 1 )

            qCursor:movePosition( QTextCursor_Start       , QTextCursor_MoveAnchor )
            qCursor:movePosition( QTextCursor_Down        , QTextCursor_MoveAnchor, i )
            qCursor:movePosition( QTextCursor_StartOfLine , QTextCursor_MoveAnchor )
            qCursor:movePosition( QTextCursor_EndOfLine   , QTextCursor_KeepAnchor )
            qCursor:insertText( cLine )
         NEXT
         qCursor:movePosition( QTextCursor_Start, QTextCursor_MoveAnchor )
         qCursor:movePosition( QTextCursor_Down , QTextCursor_MoveAnchor, nT )
         qCursor:movePosition( QTextCursor_Right, QTextCursor_MoveAnchor, nL )
      ENDIF
   ENDIF
   //
   ::qEdit:setTextCursor( qCursor )
   qCursor:endEditBlock()

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:markCurrentFunction()
   LOCAL n, nCurLine

   IF ::nPrevLineNo1 != ::getLineNo()
      ::nPrevLineNo1 := ::getLineNo()

      IF !empty( ::aTags )
         nCurLine := ::getLineNo()
         IF len( ::aTags ) == 1
            n := 1
         ELSEIF ( n := ascan( ::aTags, {|e_| e_[ 3 ] >= nCurLine } ) ) == 0
            n := len( ::aTags )
         ELSEIF n > 0
            n--
         ENDIF
         IF n > 0
            ::oIde:oFuncList:setItemColorFG( n, { 255,0,0 } )
         ENDIF
      ENDIF
   ENDIF
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:presentSkeletons()
   ::oSK:selectByMenuAndPostText( ::qEdit )
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:toggleLineNumbers()
   ::qEdit:hbNumberBlockVisible( ::lLineNumbersVisible )
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:toggleSelectionMode()
   ::qEdit:hbHighlightSelectedColumns( ::isColumnSelectionEnabled )
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:toggleLineSelectionMode()
   ::isLineSelectionMode := ! ::isLineSelectionMode
   ::qEdit:hbSetSelectionMode( 3, ::isLineSelectionMode )
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:redo()
   ::qEdit:redo()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:undo()
   ::qEdit:undo()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:cut()
   ::qEdit:hbCut()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:copy()
   ::qEdit:hbCopy()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:paste()
   ::qEdit:hbPaste()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:selectAll()
   ::qEdit:selectAll()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:setReadOnly()
   ::qEdit:setReadOnly( .t. ) // ! ::qEdit:isReadOnly() )
   ::oEditor:setTabImage()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:gotoMark( nIndex )
   IF len( ::aBookMarks ) >= nIndex
      ::qEdit:hbGotoBookmark( ::aBookMarks[ nIndex ] )
      ::qEdit:centerCursor()
   ENDIF
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:relayMarkButtons()
   LOCAL oBtn
   FOR EACH oBtn IN ::aMarkTBtns
      oBtn:hide()
   NEXT
   FOR EACH oBtn IN ::aBookMarks
      ::aMarkTBtns[ oBtn:__enumIndex() ]:show()
   NEXT
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:setNewMark()
   LOCAL qCursor, nBlock, n

   IF !( qCursor := QTextCursor():configure( ::qEdit:textCursor() ) ):isNull()
      nBlock := qCursor:blockNumber() + 1

      IF ( n := ascan( ::aBookMarks, nBlock ) ) > 0
         hb_adel( ::aBookMarks, n, .t. )
         ::aMarkTBtns[ len( ::aBookMarks ) + 1 ]:hide()
      ELSE
         IF len( ::aBookMarks ) == 6
            RETURN Self
         ENDIF
         aadd( ::aBookMarks, nBlock )
         n := len( ::aBookMarks )
         ::aMarkTBtns[ n ]:show()
      ENDIF

      ::qEdit:hbBookMarks( nBlock )
   ENDIF
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:setLineNumbersBkColor( nR, nG, nB )
   ::qEdit:hbSetLineAreaBkColor( QColor():new( nR, nG, nB ) )
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:setCurrentLineColor( nR, nG, nB )
   ::qEdit:hbSetCurrentLineColor( QColor():new( nR, nG, nB ) )
   RETURN Self

/*----------------------------------------------------------------------*/
/* TO BE EXTENDED */
METHOD IdeEdit:find( cText, nPosFrom )
   LOCAL lFound, nPos
   LOCAL qCursor := ::getCursor()

   nPos := qCursor:position()
   IF hb_isNumeric( nPosFrom )
      qCursor:setPosition( nPosFrom )
   ENDIF
   ::qEdit:setTextCursor( qCursor )
   IF ( lFound := ::qEdit:find( cText, QTextDocument_FindCaseSensitively ) )
      ::qEdit:centerCursor()
   ELSE
      qCursor:setPosition( nPos )
      ::qEdit:setTextCursor( qCursor )
   ENDIF

   RETURN lFound

/*----------------------------------------------------------------------*/

METHOD IdeEdit:refresh()
   ::qEdit:hbRefresh()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:home()
   LOCAL qCursor := ::getCursor()

   qCursor:movePosition( QTextCursor_StartOfBlock )
   ::qEdit:setTextCursor( qCursor )

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:down()
   LOCAL qCursor := QTextCursor():configure( ::qEdit:textCursor() )

   qCursor:movePosition( QTextCursor_Down )
   ::qEdit:setTextCursor( qCursor )

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:up()
   LOCAL qCursor := QTextCursor():configure( ::qEdit:textCursor() )

   qCursor:movePosition( QTextCursor_Up )
   ::qEdit:setTextCursor( qCursor )

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:duplicateLine()
   ::qEdit:hbDuplicateLine()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:deleteLine()
   ::qEdit:hbDeleteLine()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:moveLine( nDirection )
   ::qEdit:hbMoveLine( nDirection )
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:blockComment()
   ::qEdit:hbBlockComment()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:streamComment()
   ::qEdit:hbStreamComment()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:blockIndent( nMode )
   ::qEdit:hbBlockIndent( nMode )
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:caseUpper()
   ::qEdit:hbCaseUpper()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:caseLower()
   ::qEdit:hbCaseLower()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:convertQuotes()
   ::qEdit:hbConvertQuotes()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:convertDQuotes()
   ::qEdit:hbConvertDQuotes()
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:caseInvert()
   LOCAL i, c, s, cBuffer, nLen

   IF !empty( cBuffer := ::getSelectedText() )
      s    := ""
      nLen := len( cBuffer )
      FOR i := 1 TO nLen
         c := substr( cBuffer, i, 1 )
         IF isAlpha( c )
            s += iif( isUpper( c ), lower( c ), upper( c ) )
         ELSE
            s += c
         ENDIF
      NEXT
      ::qEdit:hbReplaceSelection( s )
   ENDIF

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:getSelectedText()
   RETURN ::qEdit:hbGetSelectedText()

/*----------------------------------------------------------------------*/

METHOD IdeEdit:getText()
   RETURN QTextCursor():from( ::qEdit:textCursor() ):selectedText()

/*----------------------------------------------------------------------*/

METHOD IdeEdit:getWord( lSelect )
   LOCAL cText, qCursor := QTextCursor():configure( ::qEdit:textCursor() )

   DEFAULT lSelect TO .F.

   qCursor:select( QTextCursor_WordUnderCursor )
   cText := qCursor:selectedText()

   IF lSelect
      ::qEdit:setTextCursor( qCursor )
   ENDIF
   RETURN cText

/*----------------------------------------------------------------------*/

METHOD IdeEdit:goto( nLine )
   LOCAL qCursor := QTextCursor():configure( ::qEdit:textCursor() )

   qCursor:movePosition( QTextCursor_Start )
   qCursor:movePosition( QTextCursor_Down, QTextCursor_MoveAnchor, nLine - 1 )
   ::qEdit:setTextCursor( qCursor )

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:getLine( nLine, lSelect )
   LOCAL cText, qCursor := QTextCursor():configure( ::qEdit:textCursor() )

   DEFAULT nLine   TO qCursor:blockNumber() + 1
   DEFAULT lSelect TO .F.

   IF nLine != qCursor:blockNumber() + 1
      qCursor:movePosition( QTextCursor_Start )
      qCursor:movePosition( QTextCursor_Down, QTextCursor_MoveAnchor, nLine - 1 )
   ENDIF

   qCursor:select( QTextCursor_LineUnderCursor )
   cText := qCursor:selectedText()
   IF lSelect
      ::qEdit:setTextCursor( qCursor )
   ENDIF

   RETURN cText

/*----------------------------------------------------------------------*/

METHOD IdeEdit:getColumnNo()
   RETURN QTextCursor():from( ::qEdit:textCursor() ):columnNumber() + 1

/*----------------------------------------------------------------------*/

METHOD IdeEdit:getLineNo()
   RETURN QTextCursor():from( ::qEdit:textCursor() ):blockNumber() + 1

/*----------------------------------------------------------------------*/

METHOD IdeEdit:insertSeparator( cSep )
   LOCAL qCursor := QTextCursor():configure( ::qEdit:textCursor() )

   IF empty( cSep )
      cSep := ::cSeparator
   ENDIF
   qCursor:beginEditBlock()
   qCursor:movePosition( QTextCursor_StartOfBlock )
   qCursor:insertBlock()
   qCursor:movePosition( QTextCursor_PreviousBlock )
   qCursor:insertText( cSep )
   qCursor:movePosition( QTextCursor_NextBlock )
   qCursor:movePosition( QTextCursor_StartOfBlock )
   qCursor:endEditBlock()

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:insertText( cText )
   LOCAL qCursor, nL, nB

   IF !Empty( cText )
      qCursor := QTextCursor():configure( ::qEdit:textCursor() )

      nL := len( cText )
      nB := qCursor:position() + nL

      qCursor:beginEditBlock()
      qCursor:removeSelectedText()
      qCursor:insertText( cText )
      qCursor:setPosition( nB )
      qCursor:endEditBlock()
   ENDIF
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:handlePreviousWord( lUpdatePrevWord )
   LOCAL qCursor, qTextBlock, cText, cWord, nB, nL, qEdit, lPrevOnly, nCol, nSpace, nSpaces, nOff

 * HB_TRACE( HB_TR_ALWAYS, "IdeEdit:handlePreviousWord( lUpdatePrevWord )", lUpdatePrevWord )

   IF ! lUpdatePrevWord
      RETURN Self
   ENDIF
   ::lUpdatePrevWord := .f.

   qEdit := ::qEdit

   qCursor    := QTextCursor():configure( qEdit:textCursor() )
   qTextBlock := QTextBlock():configure( qCursor:block() )
   cText      := qTextBlock:text()
   nCol       := qCursor:columnNumber()
   IF ( substr( cText, nCol - 1, 1 ) == " " )
      RETURN nil
   ENDIF
   nSpace := iif( substr( cText, nCol, 1 ) == " ", 1, 0 )
   cWord  := hbide_getPreviousWord( cText, nCol + 1 )

   IF !empty( cWord ) .AND. hbide_isHarbourKeyword( cWord )
      lPrevOnly := left( lower( ltrim( cText ) ), len( cWord ) ) == lower( cWord )

      nL := len( cWord ) + nSpace
      nB := qCursor:position() - nL

      IF ::oEditor:cExt $ ".prg"
         qCursor:beginEditBlock()
         qCursor:setPosition( nB )
         qCursor:movePosition( QTextCursor_NextCharacter, QTextCursor_KeepAnchor, nL )
         qCursor:removeSelectedText()
         qCursor:insertText( upper( cWord ) + space( nSpace ) )
         qCursor:endEditBlock()
         qEdit:setTextCursor( qCursor )
      ENDIF

      IF hbide_isStartingKeyword( cWord )
         IF lPrevOnly
            qCursor:setPosition( nB )
            IF ( nCol := qCursor:columnNumber() ) > 0
               qCursor:beginEditBlock()
               qCursor:movePosition( QTextCursor_StartOfBlock )
               qCursor:movePosition( QTextCursor_NextCharacter, QTextCursor_KeepAnchor, nCol )
               qCursor:removeSelectedText()
               qCursor:movePosition( QTextCursor_NextCharacter, QTextCursor_MoveAnchor, nL )
               qCursor:endEditBlock()
               qEdit:setTextCursor( qCursor )
            ENDIF
         ENDIF

      ELSEIF hbide_isMinimumIndentableKeyword( cWord )
         IF lPrevOnly
            qCursor:setPosition( nB )
            IF ( nCol := qCursor:columnNumber() ) >= 0
               qCursor:beginEditBlock()
               qCursor:movePosition( QTextCursor_StartOfBlock )
               qCursor:movePosition( QTextCursor_NextCharacter, QTextCursor_KeepAnchor, nCol )
               qCursor:removeSelectedText()
               qCursor:insertText( space( ::nTabSpaces ) )
               qCursor:movePosition( QTextCursor_NextCharacter, QTextCursor_MoveAnchor, nL )
               qEdit:setTextCursor( qCursor )
               qCursor:endEditBlock()
            ENDIF
         ENDIF

      ELSEIF hbide_isIndentableKeyword( cWord )
         IF lPrevOnly
            nSpaces := hbide_getFrontSpacesAndWord( cText )
            IF nSpaces > 0 .AND. ( nOff := nSpaces % ::nTabSpaces ) > 0
               qCursor:setPosition( nB )
               qCursor:beginEditBlock()
               qCursor:movePosition( QTextCursor_PreviousCharacter, QTextCursor_KeepAnchor, nOff )
               qCursor:removeSelectedText()
               qCursor:movePosition( QTextCursor_NextCharacter, QTextCursor_MoveAnchor, nL )
               qEdit:setTextCursor( qCursor )
               qCursor:endEditBlock()
            ENDIF
         ENDIF
      ENDIF
   ENDIF

   RETURN .t.

/*----------------------------------------------------------------------*/

METHOD IdeEdit:findLastIndent()
   LOCAL qCursor, qTextBlock, cText, cWord
   LOCAL nSpaces := 0

   qCursor := QTextCursor():configure( ::qEdit:textCursor() )
   qTextBlock := QTextBlock():configure( qCursor:block() )

   qTextBlock := QTextBlock():configure( qTextBlock:previous() )
   DO WHILE .t.
      IF !( qTextBlock:isValid() )
         EXIT
      ENDIF
      IF !empty( cText := qTextBlock:text() )
         nSpaces := hbide_getFrontSpacesAndWord( cText, @cWord )
         IF !empty( cWord )
            IF hbide_isIndentableKeyword( cWord )
               nSpaces += ::nTabSpaces
            ENDIF
            EXIT
         ENDIF
      ENDIF
      qTextBlock := QTextBlock():configure( qTextBlock:previous() )
   ENDDO

   RETURN nSpaces

/*----------------------------------------------------------------------*/

METHOD IdeEdit:handleCurrentIndent()
   LOCAL qCursor, nSpaces

   IF ::lIndentIt
      ::lIndentIt := .f.
      IF ( nSpaces := ::findLastIndent() ) > 0
         qCursor := QTextCursor():configure( ::qEdit:textCursor() )
         qCursor:insertText( space( nSpaces ) )
      ENDIF
   ENDIF

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:gotoFunction()
   LOCAL cWord
   IF !empty( cWord := ::getWord( .f. ) )
      ::oFN:jumpToFunction( cWord, .t. )
   ENDIF
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:clickFuncHelp()
   LOCAL cWord
   IF !empty( cWord := ::getWord( .f. ) )
      IF ! empty( ::oHL )
         ::oHL:jumpToFunction( cWord )
      ENDIF
   ENDIF
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:loadFuncHelp()
   LOCAL qEdit, qCursor, qTextBlock, cText, cWord, nCol, cPro

   qEdit := ::qEdit
   qCursor    := QTextCursor():configure( qEdit:textCursor() )
   qTextBlock := QTextBlock():configure( qCursor:block() )
   cText      := qTextBlock:text()
   nCol       := qCursor:columnNumber()
   cWord      := hbide_getPreviousWord( cText, nCol )

   IF !empty( cWord )
      IF ! empty( ::oHL )
         ::oHL:jumpToFunction( cWord )
      ENDIF
      IF !empty( cPro := ::oFN:positionToFunction( cWord, .t. ) )
         IF empty( ::cProto )
            ::showPrototype( ::cProto := hbide_formatProto( cPro ) )
         ENDIF
      ENDIF
   ENDIF
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:resumePrototype()

   ::isSuspended := .f.
   IF !empty( ::qEdit )
      IF ::getLineNo() == ::nProtoLine .AND. ::getColumnNo() >= ::nProtoCol
         ::qEdit:hbShowPrototype( ::cProto )
      ENDIF
   ENDIF

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:suspendPrototype()

   ::isSuspended := .t.
   IF !empty( ::qEdit )
      ::qEdit:hbShowPrototype( "" )
   ENDIF

   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:showPrototype( cProto )

   IF ! ::isSuspended  .AND. !empty( ::qEdit )
      IF !empty( cProto )
         ::cProto     := cProto
         ::nProtoLine := ::getLineNo()
         ::nProtoCol  := ::getColumnNo()
         ::qTimer:start()
      ENDIF
      ::qEdit:hbShowPrototype( ::cProto )
   ENDIF
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:hidePrototype()

   IF !empty( ::qedit )
      ::nProtoLine := -1
      ::nProtoCol  := -1
      ::cProto     := ""
      ::qTimer:stop()
      ::qEdit:hbShowPrototype( "" )
   ENDIF
   RETURN Self

/*----------------------------------------------------------------------*/

METHOD IdeEdit:completeCode( p )
   LOCAL qCursor := QTextCursor():from( ::qEdit:textCursor() )

   qCursor:movePosition( QTextCursor_Left )

   qCursor:movePosition( QTextCursor_StartOfWord )
   qCursor:movePosition( QTextCursor_EndOfWord, QTextCursor_KeepAnchor )
   qCursor:insertText( p )
   qCursor:movePosition( QTextCursor_Left )
   qCursor:movePosition( QTextCursor_Right )

   ::qEdit:setTextCursor( qCursor )

   RETURN Self

/*----------------------------------------------------------------------*/

FUNCTION hbide_getPreviousWord( cText, nPos )
   LOCAL cWord, n

   cText := alltrim( substr( cText, 1, nPos ) )
   IF ( n := rat( " ", cText ) ) > 0
      cWord := substr( cText, n + 1 )
   ELSE
      cWord := cText
   ENDIF

   RETURN cWord

/*----------------------------------------------------------------------*/

FUNCTION hbide_getFirstWord( cText )
   LOCAL cWord, n

   cText := alltrim( cText )
   IF ( n := at( " ", cText ) ) > 0
      cWord := left( cText, n-1 )
   ELSE
      cWord := cText
   ENDIF

   RETURN cWord

/*----------------------------------------------------------------------*/

FUNCTION hbide_getFrontSpacesAndWord( cText, cWord )
   LOCAL n := 0

   DO WHILE .t.
      IF substr( cText, ++n, 1 ) != " "
         EXIT
      ENDIF
   ENDDO
   n--

   cWord := hbide_getFirstWord( cText )

   RETURN n

/*----------------------------------------------------------------------*/

FUNCTION hbide_isStartingKeyword( cWord )
   STATIC s_b_ := { ;
                    'function' => NIL,;
                    'class' => NIL,;
                    'method' => NIL }

   RETURN Lower( cWord ) $ s_b_

/*----------------------------------------------------------------------*/

FUNCTION hbide_isMinimumIndentableKeyword( cWord )
   STATIC s_b_ := { ;
                    'local' => NIL,;
                    'static' => NIL,;
                    'return' => NIL,;
                    'default' => NIL }

   RETURN Lower( cWord ) $ s_b_

/*----------------------------------------------------------------------*/

FUNCTION hbide_isIndentableKeyword( cWord )
   STATIC s_b_ := { ;
                    'if' => NIL,;
                    'else' => NIL,;
                    'elseif' => NIL,;
                    'docase' => NIL,;
                    'case' => NIL,;
                    'otherwise' => NIL,;
                    'do' => NIL,;
                    'while' => NIL,;
                    'switch' => NIL,;
                    'for' => NIL,;
                    'next' => NIL,;
                    'begin' => NIL,;
                    'sequence' => NIL,;
                    'try' => NIL,;
                    'catch' => NIL,;
                    'always' => NIL,;
                    'recover' => NIL,;
                    'finally' => NIL }

   RETURN Lower( cWord ) $ s_b_

/*----------------------------------------------------------------------*/

FUNCTION hbide_isHarbourKeyword( cWord )
   STATIC s_b_ := { ;
                    'function' => NIL,;
                    'return' => NIL,;
                    'static' => NIL,;
                    'local' => NIL,;
                    'default' => NIL,;
                    'if' => NIL,;
                    'else' => NIL,;
                    'elseif' => NIL,;
                    'endif' => NIL,;
                    'end' => NIL,;
                    'endswitch' => NIL,;
                    'docase' => NIL,;
                    'case' => NIL,;
                    'endcase' => NIL,;
                    'otherwise' => NIL,;
                    'switch' => NIL,;
                    'do' => NIL,;
                    'while' => NIL,;
                    'enddo' => NIL,;
                    'exit' => NIL,;
                    'for' => NIL,;
                    'each' => NIL,;
                    'next' => NIL,;
                    'step' => NIL,;
                    'to' => NIL,;
                    'class' => NIL,;
                    'endclass' => NIL,;
                    'method' => NIL,;
                    'data' => NIL,;
                    'var' => NIL,;
                    'destructor' => NIL,;
                    'inline' => NIL,;
                    'setget' => NIL,;
                    'assign' => NIL,;
                    'access' => NIL,;
                    'inherit' => NIL,;
                    'init' => NIL,;
                    'create' => NIL,;
                    'virtual' => NIL,;
                    'message' => NIL,;
                    'begin' => NIL,;
                    'sequence' => NIL,;
                    'try' => NIL,;
                    'catch' => NIL,;
                    'always' => NIL,;
                    'recover' => NIL,;
                    'hb_symbol_unused' => NIL,;
                    'error' => NIL,;
                    'handler' => NIL,;
                    'nil' => NIL,;
                    'or' => NIL,;
                    'and' => NIL }

   RETURN Lower( cWord ) $ s_b_

/*----------------------------------------------------------------------*/

FUNCTION hbide_formatProto( cProto )
   LOCAL n, n1, cArgs

   n  := at( "(", cProto )
   n1 := at( ")", cProto )

   IF n > 0 .AND. n1 > 0
      cArgs  := substr( cProto, n + 1, n1 - n - 1 )
      cArgs  := strtran( cArgs, ",", "<font color=red><b>" + "," + "</b></font>" )
      cProto := "<p style='white-space:pre'>" + "<b>" + substr( cProto, 1, n - 1 ) + "</b>" + ;
                   "<font color=red><b>" + "(" + "</b></font>" + ;
                      cArgs + ;
                         "<font color=red><b>" + ")" + "</font>" + "</b></p>"
   ENDIF
   RETURN cProto

/*----------------------------------------------------------------------*/

STATIC FUNCTION hbide_normalizeRect( aCord, nT, nL, nB, nR )
   nT := iif( aCord[ 1 ] > aCord[ 3 ], aCord[ 3 ], aCord[ 1 ] )
   nB := iif( aCord[ 1 ] > aCord[ 3 ], aCord[ 1 ], aCord[ 3 ] )
   nL := iif( aCord[ 2 ] > aCord[ 4 ], aCord[ 4 ], aCord[ 2 ] )
   nR := iif( aCord[ 2 ] > aCord[ 4 ], aCord[ 2 ], aCord[ 4 ] )
   RETURN NIL

/*----------------------------------------------------------------------*/

