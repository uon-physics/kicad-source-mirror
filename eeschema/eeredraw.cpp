/*****************************************************************************
*   Program to draw EE diagrams.                                             *
* This module redraw/draw all structs.                                       *
*****************************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "appl_wxstruct.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "sch_bus_entry.h"
#include "sch_component.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_polyline.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"

#include "build_version.h"


void DrawDanglingSymbol( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& pos, int Color )
{
    BASE_SCREEN* screen = panel->GetScreen();

    if( !screen->m_IsPrinting ) /* Draw but do not print the Dangling Symbol */
    {
        GRRect( &panel->m_ClipBox, DC,
                pos.x - DANGLING_SYMBOL_SIZE, pos.y - DANGLING_SYMBOL_SIZE,
                pos.x + DANGLING_SYMBOL_SIZE, pos.y + DANGLING_SYMBOL_SIZE,
                0, Color );
    }
}


/*
 * Redraws only the active window which is assumed to be whole visible.
 */
void SCH_EDIT_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    wxString title;

    if( GetScreen() == NULL )
        return;

    ActiveScreen = GetScreen();

    DrawPanel->DrawBackGround( DC );

    GetScreen()->Draw( DrawPanel, DC, GR_DEFAULT_DRAWMODE );

    TraceWorkSheet( DC, GetScreen(), g_DrawDefaultLineThickness );

    GetScreen()->ClrRefreshReq();

    if( DrawPanel->ManageCurseur )
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    DrawPanel->DrawCursor( DC );

    // Display the sheet filename, and the sheet path, for non root sheets
    if( GetScreen()->m_FileName == m_DefaultSchematicFileName )
    {
        wxString msg = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();
        title.Printf( wxT( "%s [%s]" ), GetChars( msg), GetChars( GetScreen()->m_FileName ) );
        SetTitle( title );
    }
    else
    {
#if 0
        title = wxT( "[" );
        title << GetScreen()->m_FileName << wxT( "]  " ) << _( "Sheet" );
        title << wxT( " " ) << m_CurrentSheet->PathHumanReadable();

#else
        // Window title format:
        // [filename sheetpath] (/path/to/filedir)

        // Often the /path/to/filedir is blank because of the FullFileName argument
        // passed to LoadOneEEFile() which currently omits the path on non-root schematics.
        wxFileName t( GetScreen()->m_FileName );

        title = wxChar( '[' );
        title << t.GetName() << wxChar( ' ' );
        title << m_CurrentSheet->PathHumanReadable() << wxChar( ']' );

        title << wxChar( ' ' );
        title << wxChar( '(' ) << t.GetPath() << wxChar( ')' );
#endif

        SetTitle( title );
    }
}
