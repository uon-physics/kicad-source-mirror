/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file sch_text.h
 * @brief Implementation of the label properties dialog.
 */

#include <fctsys.h>
#include <wx/valgen.h>
#include <wx/valnum.h>
#include <sch_edit_frame.h>
#include <base_units.h>

#include <class_drawpanel.h>
#include <general.h>
#include <draw_graphic_text.h>
#include <confirm.h>
#include <sch_text.h>
#include <typeinfo>

#include <dialog_edit_label_base.h>

class SCH_EDIT_FRAME;
class SCH_TEXT;


class DIALOG_LABEL_EDITOR : public DIALOG_LABEL_EDITOR_BASE
{
public:
    DIALOG_LABEL_EDITOR( SCH_EDIT_FRAME* parent, SCH_TEXT* aTextItem );
    ~DIALOG_LABEL_EDITOR();

    void SetTitle( const wxString& aTitle ) override
    {
        // This class is shared for numerous tasks: a couple of
        // single line labels and multi-line text fields.
        // Often the desired size of the multi-line text field editor
        // is larger than is needed for the single line label.
        // Therefore the session retained sizes of these dialogs needs
        // to be class independent, make them title dependent.
        switch( m_CurrentText->Type() )
        {
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
        case SCH_LABEL_T:
            // labels can share retained settings probably.
            break;

        default:
            m_hash_key = TO_UTF8( aTitle );
            m_hash_key += typeid(*this).name();
        }

        DIALOG_LABEL_EDITOR_BASE::SetTitle( aTitle );
    }

private:
    void InitDialog( ) override;
    virtual void OnEnterKey( wxCommandEvent& aEvent ) override;
    virtual void OnOkClick( wxCommandEvent& aEvent ) override;
    virtual void OnCancelClick( wxCommandEvent& aEvent ) override;
    void OnCharHook( wxKeyEvent& aEvent );
    void TextPropertiesAccept( wxCommandEvent& aEvent );

    SCH_EDIT_FRAME* m_Parent;
    SCH_TEXT*       m_CurrentText;
    wxTextCtrl*     m_textLabel;
};



/* Edit the properties of the text (Label, Global label, graphic text).. )
 *  pointed by "aTextStruct"
 */
void SCH_EDIT_FRAME::EditSchematicText( SCH_TEXT* aTextItem )
{
    if( aTextItem == NULL )
        return;

    DIALOG_LABEL_EDITOR dialog( this, aTextItem );

    dialog.ShowModal();
}


DIALOG_LABEL_EDITOR::DIALOG_LABEL_EDITOR( SCH_EDIT_FRAME* aParent, SCH_TEXT* aTextItem ) :
    DIALOG_LABEL_EDITOR_BASE( aParent )
{
    m_Parent = aParent;
    m_CurrentText = aTextItem;
    InitDialog();

    // Conservative limits 0.0 to 10.0 inches
    const int minSize = 0;  // a value like 0.01 is better, but if > 0, creates
                            // annoying issues when trying to enter a value starting by 0 or .0
    const int maxSize = 10 * 1000 * IU_PER_MILS;

    wxFloatingPointValidator<double> textSizeValidator( NULL, wxNUM_VAL_NO_TRAILING_ZEROES );
    textSizeValidator.SetPrecision( 4 );
    textSizeValidator.SetRange( To_User_Unit( g_UserUnit, minSize ),
                                To_User_Unit( g_UserUnit, maxSize ) );

    m_TextSize->SetValidator( textSizeValidator );

    // wxTextCtrls fail to generate wxEVT_CHAR events when the wxTE_MULTILINE flag is set,
    // so we have to listen to wxEVT_CHAR_HOOK events instead.
    m_textLabelMultiLine->Connect( wxEVT_CHAR_HOOK,
                                   wxKeyEventHandler( DIALOG_LABEL_EDITOR::OnCharHook ),
                                   NULL, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


DIALOG_LABEL_EDITOR::~DIALOG_LABEL_EDITOR()
{
    m_textLabelMultiLine->Disconnect( wxEVT_CHAR_HOOK,
                                      wxKeyEventHandler( DIALOG_LABEL_EDITOR::OnCharHook ),
                                      NULL, this );
}


void DIALOG_LABEL_EDITOR::InitDialog()
{
    wxString msg;
    bool multiLine = false;

    if( m_CurrentText->IsMultilineAllowed() )
    {
        m_textLabel = m_textLabelMultiLine;
        m_textLabelSingleLine->Show( false );
        multiLine = true;
    }
    else
    {
        m_textLabel = m_textLabelSingleLine;
        m_textLabelMultiLine->Show( false );
        wxTextValidator* validator = (wxTextValidator*) m_textLabel->GetValidator();
        wxArrayString excludes;

        // Add invalid label characters to this list.
        excludes.Add( wxT( " " ) );
        validator->SetExcludes( excludes );
    }

    m_textLabel->SetValue( m_CurrentText->GetText() );
    m_textLabel->SetFocus();

    switch( m_CurrentText->Type() )
    {
    case SCH_GLOBAL_LABEL_T:
        SetTitle( _( "Global Label Properties" ) );
        break;

    case SCH_HIERARCHICAL_LABEL_T:
        SetTitle( _( "Hierarchical Label Properties" ) );
        break;

    case SCH_LABEL_T:
        SetTitle( _( "Label Properties" ) );
        break;

    case SCH_SHEET_PIN_T:
        SetTitle( _( "Hierarchical Sheet Pin Properties." ) );
        break;

    default:
        SetTitle( _( "Text Properties" ) );
        break;
    }

    const int MINTEXTWIDTH = 40;    // M's are big characters, a few establish a lot of width

    int max_len = 0;

    if ( !multiLine )
    {
        max_len = m_CurrentText->GetText().Length();
    }
    else
    {
        // calculate the length of the biggest line
        // we cannot use the length of the entire text that has no meaning
        int curr_len = MINTEXTWIDTH;
        int imax = m_CurrentText->GetText().Length();

        for( int count = 0; count < imax; count++ )
        {
            if( m_CurrentText->GetText()[count] == '\n' ||
                m_CurrentText->GetText()[count] == '\r' ) // new line
            {
                curr_len = 0;
            }
            else
            {
                curr_len++;

                if ( max_len < curr_len )
                    max_len = curr_len;
            }
        }
    }

    if( max_len < MINTEXTWIDTH )
        max_len = MINTEXTWIDTH;

    wxString textWidth;
    textWidth.Append( 'M', MINTEXTWIDTH );
    EnsureTextCtrlWidth( m_textLabel, &textWidth );

    // Set text options:
    m_TextOrient->SetSelection( m_CurrentText->GetLabelSpinStyle() );
    m_TextShape->SetSelection( m_CurrentText->GetShape() );

    int style = 0;

    if( m_CurrentText->IsItalic() )
        style = 1;

    if( m_CurrentText->IsBold() )
        style += 2;

    m_TextStyle->SetSelection( style );

    wxString units = ReturnUnitSymbol( g_UserUnit, wxT( "(%s)" ) );
    msg.Printf( _( "H%s x W%s" ), GetChars( units ), GetChars( units ) );
    m_staticSizeUnits->SetLabel( msg );

    msg = StringFromValue( g_UserUnit, m_CurrentText->GetTextWidth() );
    m_TextSize->SetValue( msg );

    if( m_CurrentText->Type() != SCH_GLOBAL_LABEL_T
     && m_CurrentText->Type() != SCH_HIERARCHICAL_LABEL_T )
    {
        m_TextShape->Show( false );
    }

    m_sdbSizer1OK->SetDefault();
}


/*!
 * wxEVT_COMMAND_ENTER event handler for m_textLabel
 */

void DIALOG_LABEL_EDITOR::OnEnterKey( wxCommandEvent& aEvent )
{
    TextPropertiesAccept( aEvent );
}


/*!
 * wxEVT_CHAR_HOOK event handler for m_textLabel
 */

void DIALOG_LABEL_EDITOR::OnCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_TAB )
    {
        int flags = 0;
        if( !aEvent.ShiftDown() )
            flags |= wxNavigationKeyEvent::IsForward;
        if( aEvent.ControlDown() )
            flags |= wxNavigationKeyEvent::WinChange;
        NavigateIn( flags );
    }
    else if( aEvent.GetKeyCode() == WXK_RETURN && aEvent.ShiftDown() )
    {
        wxCommandEvent cmdEvent( wxEVT_COMMAND_ENTER );
        TextPropertiesAccept( cmdEvent );
    }
    else
    {
        aEvent.Skip();
    }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DIALOG_LABEL_EDITOR::OnOkClick( wxCommandEvent& aEvent )
{
    TextPropertiesAccept( aEvent );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_LABEL_EDITOR::OnCancelClick( wxCommandEvent& aEvent )
{
    m_Parent->GetCanvas()->MoveCursorToCrossHair();
    EndModal( wxID_CANCEL );
}


void DIALOG_LABEL_EDITOR::TextPropertiesAccept( wxCommandEvent& aEvent )
{
    wxString text;
    int      value;

    /* save old text in undo list if not already in edit */
    /* or the label to be edited is part of a block */
    if( m_CurrentText->GetFlags() == 0 ||
        m_Parent->GetScreen()->m_BlockLocate.GetState() != STATE_NO_BLOCK )
        m_Parent->SaveCopyInUndoList( m_CurrentText, UR_CHANGED );

    m_Parent->GetCanvas()->RefreshDrawingRect( m_CurrentText->GetBoundingBox() );

    text = m_textLabel->GetValue();

    if( !text.IsEmpty() )
        m_CurrentText->SetText( text );
    else if( !m_CurrentText->IsNew() )
    {
        DisplayError( this, _( "Empty Text!" ) );
        return;
    }

    m_CurrentText->SetLabelSpinStyle( m_TextOrient->GetSelection() );
    text  = m_TextSize->GetValue();
    value = ValueFromString( g_UserUnit, text );
    m_CurrentText->SetTextSize( wxSize( value, value ) );

    if( m_TextShape )
        /// @todo move cast to widget
        m_CurrentText->SetShape( static_cast<PINSHEETLABEL_SHAPE>( m_TextShape->GetSelection() ) );

    int style = m_TextStyle->GetSelection();

    m_CurrentText->SetItalic( ( style & 1 ) );

    if( ( style & 2 ) )
    {
        m_CurrentText->SetBold( true );
        m_CurrentText->SetThickness( GetPenSizeForBold( m_CurrentText->GetTextWidth() ) );
    }
    else
    {
        m_CurrentText->SetBold( false );
        m_CurrentText->SetThickness( 0 );
    }

    m_Parent->OnModify();

    // Make the text size the new default size ( if it is a new text ):
    if( m_CurrentText->IsNew() )
        SetDefaultTextSize( m_CurrentText->GetTextWidth() );

    m_Parent->GetCanvas()->RefreshDrawingRect( m_CurrentText->GetBoundingBox() );
    m_Parent->GetCanvas()->MoveCursorToCrossHair();
    EndModal( wxID_OK );
}
