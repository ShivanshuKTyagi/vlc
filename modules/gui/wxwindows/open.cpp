/*****************************************************************************
 * open.cpp : wxWindows plugin for vlc
 *****************************************************************************
 * Copyright (C) 2000-2001 VideoLAN
 * $Id: open.cpp,v 1.11 2003/04/01 00:18:29 gbazin Exp $
 *
 * Authors: Gildas Bazin <gbazin@netcourrier.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include <stdlib.h>                                      /* malloc(), free() */
#include <errno.h>                                                 /* ENOMEM */
#include <string.h>                                            /* strerror() */
#include <stdio.h>

#include <vlc/vlc.h>

#ifdef WIN32                                                 /* mingw32 hack */
#undef Yield
#undef CreateDialog
#endif

/* Let vlc take care of the i18n stuff */
#define WXINTL_NO_GETTEXT_MACRO

#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>

#include <vlc/intf.h>

#include "wxwindows.h"

#ifndef wxRB_SINGLE
#   define wxRB_SINGLE 0
#endif

/*****************************************************************************
 * Event Table.
 *****************************************************************************/

/* IDs for the controls and the menu commands */
enum
{
    Notebook_Event = wxID_HIGHEST,
    MRL_Event,

    FileBrowse_Event,
    FileName_Event,

    DiscType_Event,
    DiscDevice_Event,
    DiscTitle_Event,
    DiscChapter_Event,

    NetType_Event,
    NetRadio1_Event, NetRadio2_Event, NetRadio3_Event, NetRadio4_Event,
    NetPort1_Event, NetPort2_Event, NetPort3_Event, NetPort4_Event,
    NetAddr1_Event, NetAddr2_Event, NetAddr3_Event, NetAddr4_Event,

    SoutEnable_Event,
    SoutSettings_Event,

    DemuxDump_Event,
    DemuxDumpEnable_Event,
    DemuxDumpBrowse_Event,
};

BEGIN_EVENT_TABLE(OpenDialog, wxDialog)
    /* Button events */
    EVT_BUTTON(wxID_OK, OpenDialog::OnOk)
    EVT_BUTTON(wxID_CANCEL, OpenDialog::OnCancel)

    EVT_NOTEBOOK_PAGE_CHANGED(Notebook_Event, OpenDialog::OnPageChange)

    EVT_TEXT(MRL_Event, OpenDialog::OnMRLChange) 

    /* Events generated by the file panel */
    EVT_TEXT(FileName_Event, OpenDialog::OnFilePanelChange)
    EVT_BUTTON(FileBrowse_Event, OpenDialog::OnFileBrowse)

    /* Events generated by the disc panel */
    EVT_RADIOBOX(DiscType_Event, OpenDialog::OnDiscTypeChange)
    EVT_TEXT(DiscDevice_Event, OpenDialog::OnDiscPanelChange)
    EVT_TEXT(DiscTitle_Event, OpenDialog::OnDiscPanelChange)
    EVT_SPINCTRL(DiscTitle_Event, OpenDialog::OnDiscPanelChange)
    EVT_TEXT(DiscChapter_Event, OpenDialog::OnDiscPanelChange)
    EVT_SPINCTRL(DiscChapter_Event, OpenDialog::OnDiscPanelChange)

    /* Events generated by the net panel */
    EVT_RADIOBUTTON(NetRadio1_Event, OpenDialog::OnNetTypeChange)
    EVT_RADIOBUTTON(NetRadio2_Event, OpenDialog::OnNetTypeChange)
    EVT_RADIOBUTTON(NetRadio3_Event, OpenDialog::OnNetTypeChange)
    EVT_RADIOBUTTON(NetRadio4_Event, OpenDialog::OnNetTypeChange)
    EVT_TEXT(NetPort1_Event, OpenDialog::OnNetPanelChange)
    EVT_SPINCTRL(NetPort1_Event, OpenDialog::OnNetPanelChange)
    EVT_TEXT(NetPort2_Event, OpenDialog::OnNetPanelChange)
    EVT_SPINCTRL(NetPort2_Event, OpenDialog::OnNetPanelChange)
    EVT_TEXT(NetPort3_Event, OpenDialog::OnNetPanelChange)
    EVT_SPINCTRL(NetPort3_Event, OpenDialog::OnNetPanelChange)
    EVT_TEXT(NetAddr2_Event, OpenDialog::OnNetPanelChange)
    EVT_TEXT(NetAddr3_Event, OpenDialog::OnNetPanelChange)
    EVT_TEXT(NetAddr4_Event, OpenDialog::OnNetPanelChange)

    /* Events generated by the stream output buttons */
    EVT_CHECKBOX(SoutEnable_Event, OpenDialog::OnSoutEnable)
    EVT_BUTTON(SoutSettings_Event, OpenDialog::OnSoutSettings)

    /* Events generated by the demux dump buttons */
    EVT_CHECKBOX(DemuxDumpEnable_Event, OpenDialog::OnDemuxDumpEnable)
    EVT_TEXT(DemuxDump_Event, OpenDialog::OnDemuxDumpChange)
    EVT_BUTTON(DemuxDumpBrowse_Event, OpenDialog::OnDemuxDumpBrowse)

END_EVENT_TABLE()

/*****************************************************************************
 * Constructor.
 *****************************************************************************/
OpenDialog::OpenDialog( intf_thread_t *_p_intf, Interface *_p_main_interface,
                        int i_access_method ):
    wxDialog( _p_main_interface, -1, _("Open Target"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_FRAME_STYLE )
{
    /* Initializations */
    p_intf = _p_intf;
    p_main_interface = _p_main_interface;

    /* Create a panel to put everything in */
    wxPanel *panel = new wxPanel( this, -1 );
    panel->SetAutoLayout( TRUE );

    /* Create MRL combobox */
    wxBoxSizer *mrl_sizer_sizer = new wxBoxSizer( wxHORIZONTAL );
    wxStaticBox *mrl_box = new wxStaticBox( panel, -1,
                               _("Media Resource Locator (MRL)") );
    wxStaticBoxSizer *mrl_sizer = new wxStaticBoxSizer( mrl_box,
                                                        wxHORIZONTAL );
    wxStaticText *mrl_label = new wxStaticText( panel, -1,
                                                _("Open Target:") );
    mrl_combo = new wxComboBox( panel, MRL_Event, mrl,
                                wxPoint(20,25), wxSize(120, -1),
                                0, NULL );
    mrl_sizer->Add( mrl_label, 0, wxEXPAND | wxALL, 5 );
    mrl_sizer->Add( mrl_combo, 1, wxEXPAND | wxALL, 5 );
    mrl_sizer_sizer->Add( mrl_sizer, 1, wxEXPAND | wxALL, 5 );


    /* Create Static Text */
    wxStaticText *label = new wxStaticText( panel, -1,
        _("Alternatively, you can build an MRL using one of the "
          "following predefined targets:") );

    /* Create Stream Output checkox */
    wxFlexGridSizer *sout_sizer = new wxFlexGridSizer( 2, 1, 20 );
    sout_checkbox = new wxCheckBox( panel, SoutEnable_Event,
                                           _("Stream output") );
    sout_checkbox->SetToolTip( _("Use VLC has a stream server") );
    sout_sizer->Add( sout_checkbox, 0,
                     wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );
    sout_button = new wxButton( panel, SoutSettings_Event, _("Settings...") );
    sout_button->Disable();

    char *psz_sout = config_GetPsz( p_intf, "sout" );
    if( psz_sout && *psz_sout )
    {
        sout_checkbox->SetValue(TRUE);
        sout_button->Enable();
    }
    if( psz_sout ) free( psz_sout );

    sout_sizer->Add( sout_button, 1, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );

    /* Create Demux Dump checkox */
    wxBoxSizer *demuxdump_sizer = new wxBoxSizer( wxHORIZONTAL );
    demuxdump_checkbox = new wxCheckBox( panel, DemuxDumpEnable_Event,
                               _("Capture input stream") );
    demuxdump_checkbox->SetToolTip(
                           _("Capture the stream you are playing to a file") );
    demuxdump_textctrl = new wxTextCtrl( panel, DemuxDump_Event,
                                         "", wxDefaultPosition, wxDefaultSize,
                                         wxTE_PROCESS_ENTER);
    demuxdump_button = new wxButton( panel, DemuxDumpBrowse_Event,
                                     _("Browse...") );

    char *psz_demuxdump = config_GetPsz( p_intf, "demuxdump-file" );
    if( psz_demuxdump && *psz_demuxdump )
    {
        demuxdump_textctrl->SetValue( psz_demuxdump );
    }
    if( psz_demuxdump ) free( psz_demuxdump );

    demuxdump_textctrl->Disable();
    demuxdump_button->Disable();
    demuxdump_sizer->Add( demuxdump_checkbox, 0,
                          wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 10 );
    demuxdump_sizer->Add( demuxdump_button, 0,
                          wxALL | wxALIGN_CENTER_VERTICAL, 10 );
    demuxdump_sizer->Add( demuxdump_textctrl, 1, wxALL, 10 );

    /* Separation */
    wxStaticLine *static_line = new wxStaticLine( panel, wxID_OK );

    /* Create the buttons */
    wxButton *ok_button = new wxButton( panel, wxID_OK, _("OK") );
    ok_button->SetDefault();
    wxButton *cancel_button = new wxButton( panel, wxID_CANCEL, _("Cancel") );

    /* Create notebook */
    wxNotebook *notebook = new wxNotebook( panel, Notebook_Event );
    wxNotebookSizer *notebook_sizer = new wxNotebookSizer( notebook );

    notebook->AddPage( FilePanel( notebook ), _("File"),
                       i_access_method == FILE_ACCESS );
    notebook->AddPage( DiscPanel( notebook ), _("Disc"),
                       i_access_method == DISC_ACCESS );
    notebook->AddPage( NetPanel( notebook ), _("Network"),
                       i_access_method == NET_ACCESS );
#ifndef WIN32
    notebook->AddPage( SatPanel( notebook ), _("Satellite"),
                       i_access_method == SAT_ACCESS );
#endif

    /* Update Disc panel */
    wxCommandEvent dummy_event;
    OnDiscTypeChange( dummy_event );

    /* Update Net panel */
    dummy_event.SetId( NetRadio1_Event );
    OnNetTypeChange( dummy_event );

    /* Update MRL */
    wxNotebookEvent event = wxNotebookEvent( wxEVT_NULL, 0, i_access_method );
    OnPageChange( event );

    /* Place everything in sizers */
    wxBoxSizer *button_sizer = new wxBoxSizer( wxHORIZONTAL );
    button_sizer->Add( ok_button, 0, wxALL, 5 );
    button_sizer->Add( cancel_button, 0, wxALL, 5 );
    button_sizer->Layout();
    wxBoxSizer *main_sizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer *panel_sizer = new wxBoxSizer( wxVERTICAL );
    panel_sizer->Add( mrl_sizer_sizer, 0, wxEXPAND, 5 );
    panel_sizer->Add( label, 0, wxEXPAND | wxALL, 5 );
    panel_sizer->Add( notebook_sizer, 1, wxEXPAND | wxALL, 5 );
    panel_sizer->Add( sout_sizer, 0, wxALIGN_LEFT | wxALL, 5 );
    panel_sizer->Add( demuxdump_sizer, 0, wxEXPAND | wxALIGN_LEFT | wxALL, 5 );
    panel_sizer->Add( static_line, 0, wxEXPAND | wxALL, 5 );
    panel_sizer->Add( button_sizer, 0, wxALIGN_LEFT | wxALL, 5 );
    panel_sizer->Layout();
    panel->SetSizerAndFit( panel_sizer );
    main_sizer->Add( panel, 1, wxGROW, 0 );
    main_sizer->Layout();
    SetSizerAndFit( main_sizer );
}

OpenDialog::~OpenDialog()
{
}

/*****************************************************************************
 * Private methods.
 *****************************************************************************/
wxPanel *OpenDialog::FilePanel( wxWindow* parent )
{
    wxPanel *panel = new wxPanel( parent, -1, wxDefaultPosition,
                                  wxSize(200, 200) );

    wxBoxSizer *sizer = new wxBoxSizer( wxHORIZONTAL );

    file_combo = new wxComboBox( panel, FileName_Event, "",
                                 wxPoint(20,25), wxSize(200, -1), 0, NULL );
    wxButton *browse_button = new wxButton( panel, FileBrowse_Event,
                                            _("Browse...") );
    sizer->Add( file_combo, 1, wxALL, 5 );
    sizer->Add( browse_button, 0, wxALL, 5 );

    panel->SetSizerAndFit( sizer );
    return panel;
}

wxPanel *OpenDialog::DiscPanel( wxWindow* parent )
{
    wxPanel *panel = new wxPanel( parent, -1, wxDefaultPosition,
                                  wxSize(200, 200) );

    wxBoxSizer *sizer_row = new wxBoxSizer( wxVERTICAL );
    wxFlexGridSizer *sizer = new wxFlexGridSizer( 2, 3, 20 );

    static const wxString disc_type_array[] =
    {
        _("DVD"),
        _("DVD (menus support)"),
        _("VCD")
    };

    disc_type = new wxRadioBox( panel, DiscType_Event, _("Disc type"),
                                wxDefaultPosition, wxDefaultSize,
                                WXSIZEOF(disc_type_array), disc_type_array,
                                WXSIZEOF(disc_type_array), wxRA_SPECIFY_COLS );
    sizer_row->Add( disc_type, 0, wxEXPAND | wxALL, 5 );

    wxStaticText *label = new wxStaticText( panel, -1, _("Device name") );
    disc_device = new wxTextCtrl( panel, DiscDevice_Event, "",
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_PROCESS_ENTER);

    sizer->Add( label, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    sizer->Add( disc_device, 1, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );

    label = new wxStaticText( panel, -1, _("Title") );
    disc_title = new wxSpinCtrl( panel, DiscTitle_Event );

    sizer->Add( label, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    sizer->Add( disc_title, 1, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );

    label = new wxStaticText( panel, -1, _("Chapter") );
    disc_chapter = new wxSpinCtrl( panel, DiscChapter_Event );
    sizer->Add( label, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    sizer->Add( disc_chapter, 1, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    sizer_row->Add( sizer, 0, wxEXPAND | wxALL, 5 );

    panel->SetSizerAndFit( sizer_row );
    return panel;
}

wxPanel *OpenDialog::NetPanel( wxWindow* parent )
{
    int i;
    wxPanel *panel = new wxPanel( parent, -1, wxDefaultPosition,
                                  wxSize(200, 200) );

    wxBoxSizer *sizer_row = new wxBoxSizer( wxVERTICAL );
    wxFlexGridSizer *sizer = new wxFlexGridSizer( 2, 4, 20 );

    static const wxString net_type_array[] =
    {
        _("UDP/RTP"),
        _("UDP/RTP Multicast"),
        _("Channel server"),
        _("HTTP/FTP/MMS")
    };

    for( i=0; i<4; i++ )
    {
        net_radios[i] = new wxRadioButton( panel, NetRadio1_Event + i,
                                           net_type_array[i],
                                           wxDefaultPosition, wxDefaultSize,
                                           wxRB_SINGLE );

        net_subpanels[i] = new wxPanel( panel, -1,
                                        wxDefaultPosition, wxDefaultSize );
    }

    /* UDP/RTP row */
    wxFlexGridSizer *subpanel_sizer;
    wxStaticText *label;
    int val = config_GetInt( p_intf, "server-port" );
    subpanel_sizer = new wxFlexGridSizer( 2, 1, 20 );
    label = new wxStaticText( net_subpanels[0], -1, _("Port") );
    net_ports[0] = new wxSpinCtrl( net_subpanels[0], NetPort1_Event,
                                   wxString::Format("%d", val),
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS,
                                   0, 16000, val);

    subpanel_sizer->Add( label, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );
    subpanel_sizer->Add( net_ports[0], 1,
                         wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    net_subpanels[0]->SetSizerAndFit( subpanel_sizer );
    net_radios[0]->SetValue( TRUE );

    /* UDP/RTP Multicast row */
    subpanel_sizer = new wxFlexGridSizer( 4, 1, 20 );
    label = new wxStaticText( net_subpanels[1], -1, _("Address") );
    net_addrs[1] = new wxTextCtrl( net_subpanels[1], NetAddr2_Event, "",
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_PROCESS_ENTER);
    subpanel_sizer->Add( label, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );
    subpanel_sizer->Add( net_addrs[1], 1,
                         wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );

    label = new wxStaticText( net_subpanels[1], -1, _("Port") );
    net_ports[1] = new wxSpinCtrl( net_subpanels[1], NetPort2_Event,
                                   wxString::Format("%d", val),
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS,
                                   0, 16000, val);

    subpanel_sizer->Add( label, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );
    subpanel_sizer->Add( net_ports[1], 1,
                         wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    net_subpanels[1]->SetSizerAndFit( subpanel_sizer );

    /* Channel server row */
    subpanel_sizer = new wxFlexGridSizer( 4, 1, 20 );
    label = new wxStaticText( net_subpanels[2], -1, _("Address") );
    net_addrs[2] = new wxTextCtrl( net_subpanels[2], NetAddr3_Event, "",
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_PROCESS_ENTER);
    subpanel_sizer->Add( label, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );
    subpanel_sizer->Add( net_addrs[2], 1,
                         wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );

    label = new wxStaticText( net_subpanels[2], -1, _("Port") );
    net_ports[2] = new wxSpinCtrl( net_subpanels[2], NetPort3_Event,
                                   wxString::Format("%d", val),
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS,
                                   0, 16000, val);

    subpanel_sizer->Add( label, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );
    subpanel_sizer->Add( net_ports[2], 1,
                         wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    net_subpanels[2]->SetSizerAndFit( subpanel_sizer );

    /* HTTP row */
    subpanel_sizer = new wxFlexGridSizer( 2, 1, 20 );
    label = new wxStaticText( net_subpanels[3], -1, _("URL") );
    net_addrs[3] = new wxTextCtrl( net_subpanels[3], NetAddr4_Event, "",
                                   wxDefaultPosition, wxSize( 200, -1 ),
                                   wxTE_PROCESS_ENTER);
    subpanel_sizer->Add( label, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );
    subpanel_sizer->Add( net_addrs[3], 1,
                         wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    net_subpanels[3]->SetSizerAndFit( subpanel_sizer );

    /* Stuff everything into the main panel */
    for( i=0; i<4; i++ )
    {
        sizer->Add( net_radios[i], 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
        sizer->Add( net_subpanels[i], 1,
                    wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    }

    sizer_row->Add( sizer, 0, wxEXPAND | wxALL, 5 );

    panel->SetSizerAndFit( sizer_row );
    return panel;
}

wxPanel *OpenDialog::SatPanel( wxWindow* parent )
{
    wxPanel *panel = new wxPanel( parent, -1, wxDefaultPosition,
                                  wxSize(200, 200) );
    return panel;
}

void OpenDialog::UpdateMRL( int i_access_method )
{
    wxString demux;

    i_current_access_method = i_access_method;

    /* Check if the user asked for demuxdump */
    if( demuxdump_checkbox->GetValue() )
    {
        demux = "/demuxdump";
    }

    switch( i_access_method )
    {
    case FILE_ACCESS:
        mrl = "file" + demux + "://" + file_combo->GetValue();
        break;
    case DISC_ACCESS:
        mrl = ( disc_type->GetSelection() == 0 ? "dvdold" :
                disc_type->GetSelection() == 1 ? "dvd" : "vcd" )
                  + demux + "://"
                  + disc_device->GetLineText(0)
                  + wxString::Format( "@%d:%d",
                                      disc_title->GetValue(),
                                      disc_chapter->GetValue() );
        break;
    case NET_ACCESS:
        switch( i_net_type )
        {
        case 0:
            if( net_ports[0]->GetValue() !=
                config_GetInt( p_intf, "server-port" ) )
            {
                mrl = "udp" + demux +
                       wxString::Format( "://@:%d", net_ports[0]->GetValue() );
            }
            else
            {
                mrl = "udp" + demux + "://";
            }
            break;

        case 1:
            mrl = "udp" + demux + "://@" + net_addrs[1]->GetLineText(0);
            if( net_ports[1]->GetValue() !=
                config_GetInt( p_intf, "server-port" ) )
            {
                mrl = mrl + wxString::Format( ":%d",
                                              net_ports[1]->GetValue() );
            }
            break;

        case 2:
            mrl = "udp" + demux + "://";
            break;

        case 3:
            /* http access */     
            mrl = "http" + demux + "://" + net_addrs[3]->GetLineText(0);
            break;
        }
        break;
    case SAT_ACCESS:
        mrl = "satellite" + demux + "://";
        break;
    default:
        break;
    }

    mrl_combo->SetValue( mrl );
}

/*****************************************************************************
 * Events methods.
 *****************************************************************************/
void OpenDialog::OnOk( wxCommandEvent& WXUNUSED(event) )
{
    EndModal( wxID_OK );
}

void OpenDialog::OnCancel( wxCommandEvent& WXUNUSED(event) )
{
    EndModal( wxID_CANCEL );
}

void OpenDialog::OnPageChange( wxNotebookEvent& event )
{
    UpdateMRL( event.GetSelection() );
}

void OpenDialog::OnMRLChange( wxCommandEvent& event )
{
    mrl = event.GetString();
}

/*****************************************************************************
 * File panel event methods.
 *****************************************************************************/
void OpenDialog::OnFilePanelChange( wxCommandEvent& WXUNUSED(event) )
{
    UpdateMRL( FILE_ACCESS );
}

void OpenDialog::OnFileBrowse( wxCommandEvent& WXUNUSED(event) )
{
    wxFileDialog dialog( this, _("Open file"), "", "", "*.*",
                         wxOPEN );

    if( dialog.ShowModal() == wxID_OK )
    {
        file_combo->SetValue( dialog.GetPath() );      
        UpdateMRL( FILE_ACCESS );
    }
}

/*****************************************************************************
 * Disc panel event methods.
 *****************************************************************************/
void OpenDialog::OnDiscPanelChange( wxCommandEvent& WXUNUSED(event) )
{
    UpdateMRL( DISC_ACCESS );
}

void OpenDialog::OnDiscTypeChange( wxCommandEvent& WXUNUSED(event) )
{
    char *psz_device;

    switch( disc_type->GetSelection() )
    {
    case 2:
        psz_device = config_GetPsz( p_intf, "vcd" );
        disc_device->SetValue( psz_device ? psz_device : "" );
        break;

    default:
        psz_device = config_GetPsz( p_intf, "dvd" );
        disc_device->SetValue( psz_device ? psz_device : "" );
        break;
    }

    if( psz_device ) free( psz_device );

    switch( disc_type->GetSelection() )
    {
    case 1:
        disc_title->SetRange( 0, 255 );
        disc_title->SetValue( 0 );
        break;

    default:
        disc_title->SetRange( 1, 255 );
        disc_title->SetValue( 1 );
        break;
    }

    disc_chapter->SetRange( 1, 255 );
    disc_chapter->SetValue( 1 );

    UpdateMRL( DISC_ACCESS );
}

/*****************************************************************************
 * Net panel event methods.
 *****************************************************************************/
void OpenDialog::OnNetPanelChange( wxCommandEvent& WXUNUSED(event) )
{
    UpdateMRL( NET_ACCESS );
}

void OpenDialog::OnNetTypeChange( wxCommandEvent& event )
{
    int i;

    i_net_type = event.GetId() - NetRadio1_Event;

    for(i=0; i<4; i++)
    {
        net_radios[i]->SetValue( event.GetId() == (NetRadio1_Event+i) );
        net_subpanels[i]->Enable( event.GetId() == (NetRadio1_Event+i) );
    }

    UpdateMRL( NET_ACCESS );
}

/*****************************************************************************
 * Stream output event methods.
 *****************************************************************************/
void OpenDialog::OnSoutEnable( wxCommandEvent& event )
{
    sout_button->Enable( event.GetInt() != 0 );
    if( !event.GetInt() )
    {
        config_PutPsz( p_intf, "sout", "" );
    }
    else
    {
        demuxdump_checkbox->SetValue( 0 );
        wxCommandEvent event = wxCommandEvent( wxEVT_NULL );
        event.SetInt( 0 );
        OnDemuxDumpEnable( event );
    }
}

void OpenDialog::OnSoutSettings( wxCommandEvent& WXUNUSED(event) )
{
    /* Show/hide the open dialog */
    SoutDialog dialog( p_intf, p_main_interface );

    if( dialog.ShowModal() == wxID_OK )
    {
        config_PutPsz( p_intf, "sout", (char *)dialog.mrl.c_str() );
    }
}

/*****************************************************************************
 * Demux dump event methods.
 *****************************************************************************/
void OpenDialog::OnDemuxDumpEnable( wxCommandEvent& event )
{
    demuxdump_textctrl->Enable( event.GetInt() != 0 );
    demuxdump_button->Enable( event.GetInt() != 0 );

    if( event.GetInt() )
    {
        sout_checkbox->SetValue( 0 );
        wxCommandEvent event = wxCommandEvent( wxEVT_NULL );
        event.SetInt( 0 );
        OnSoutEnable( event );
    }

    UpdateMRL( i_current_access_method );
}

void OpenDialog::OnDemuxDumpBrowse( wxCommandEvent& WXUNUSED(event) )
{
    wxFileDialog dialog( this, _("Save file"), "", "", "*.*", wxSAVE );

    if( dialog.ShowModal() == wxID_OK )
    {
        demuxdump_textctrl->SetValue( dialog.GetPath() );
        wxCommandEvent event = wxCommandEvent( wxEVT_NULL );
        OnDemuxDumpChange( event );
    }
}

void OpenDialog::OnDemuxDumpChange( wxCommandEvent& WXUNUSED(event) )
{
    config_PutPsz( p_intf, "demuxdump-file",
                   demuxdump_textctrl->GetValue() );
}
