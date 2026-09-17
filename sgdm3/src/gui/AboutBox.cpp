// AboutBox.cpp
//////////////////////////////////////////////////////////////////

#include <ConfigPch.h>
#include <ConfigDcl.h>
#include <util.h>
#include <rs.h>
#include <fim.h>
#include <poi.h>
#include <fs.h>
#include <fd.h>
#include <gui.h>

//////////////////////////////////////////////////////////////////

#define M 7
#define FIX 0
#define VAR 1

//////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(AboutBox,wxDialog)
	EVT_BUTTON(ID_BUTTON_SUPPORT,		AboutBox::onEventButton_Support)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////

AboutBox::AboutBox(wxFrame * pFrameParent,
				   gui_frame * pGuiFrameActive)
	: wxDialog(pFrameParent,-1,VER_ABOUTBOX_TITLE,
			   wxDefaultPosition,wxDefaultSize,
			   wxDEFAULT_DIALOG_STYLE),
	m_pFrameParent(pFrameParent),
	m_pGuiFrameActive(pGuiFrameActive)
{
#if defined(__WXMAC__)
	SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

	wxButton * pButtonClose = NULL;

	wxBoxSizer * vSizerTop = new wxBoxSizer(wxVERTICAL);
	{
		wxBoxSizer * hSizerField = new wxBoxSizer(wxHORIZONTAL);
		{
			hSizerField->Add( new wxStaticBitmap(this,wxID_ANY,wxBitmap(DiffMergeIcon_xpm)), FIX, wxALL | wxALIGN_LEFT, M);
			wxBoxSizer * vSizerText = new wxBoxSizer(wxVERTICAL);
			{
				wxString strBuildVersion = build_version_string();

				vSizerText->Add( new wxStaticText(this,wxID_ANY, VER_APP_TITLE),   FIX, wxALL, M);
				vSizerText->Add( new wxStaticText(this,wxID_ANY, strBuildVersion), FIX, wxALL, M);
				vSizerText->Add( new wxStaticText(this,wxID_ANY, VER_COPYRIGHT),   FIX, wxALL, M);
				vSizerText->Add( new wxStaticText(this,wxID_ANY, VER_COMPANY_URL), FIX, wxALL, M);
			}
			hSizerField->Add(vSizerText,FIX, wxALL,M);
		}
		vSizerTop->Add(hSizerField,FIX,0,0);

		// put horizontal line and set of ok/cancel buttons across the bottom of the dialog

		vSizerTop->AddStretchSpacer(VAR);
		vSizerTop->Add( new wxStaticLine(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxLI_HORIZONTAL), FIX, wxGROW| wxLEFT|wxRIGHT, M);
		wxBoxSizer * hSizerButtonBar = new wxBoxSizer(wxHORIZONTAL);
		{
			hSizerButtonBar->Add( new wxButton(this,ID_BUTTON_SUPPORT,_("&Support...")), FIX, 0,0);
			hSizerButtonBar->AddStretchSpacer(VAR);
			pButtonClose = new wxButton(this,wxID_OK,_("&Close"));
			hSizerButtonBar->Add( pButtonClose, FIX, 0,0);	// can't use CreateButtonSizer() because we have a different label for OK button
		}
		vSizerTop->Add(hSizerButtonBar,FIX,wxGROW|wxALL,M);

	}
	SetSizer(vSizerTop);
	vSizerTop->SetSizeHints(this);
	vSizerTop->Fit(this);
	Centre(wxBOTH);

	SetAffirmativeId(wxID_OK);
	SetEscapeId(wxID_ANY);			// causes ESC key to send affirmative-id

	pButtonClose->SetDefault();
	pButtonClose->SetFocus();
}

//////////////////////////////////////////////////////////////////

/*static*/wxString AboutBox::build_version_string(void)
{
	wxString strArchPackage(VER_ARCH_PACKAGE);
	wxString strBuildLabel(VER_BUILD_LABEL);
	wxString strCSID(VER_CSID);

	if (strBuildLabel.StartsWith(_T("@"))			// if optional BUILDLABEL was NOT set by script running the compiler
		|| strBuildLabel.StartsWith(_T("_BLANK_")))	// or if it contains a placeholder,
		strBuildLabel = _T("");						// then we don't want to include it in the version number line.

	strCSID.Truncate(10);	// just show the first 10 digits of the veracity changeset id.

	wxString strVersion = wxString::Format(_("Version %d.%d.%db%d [%s] %s"),
										   VER_MAJOR_VERSION,VER_MINOR_VERSION,VER_MINOR_SUBVERSION,
										   VER_BUILD_NUMBER,
										   strArchPackage.wc_str(),
										   strBuildLabel.wc_str());

#if defined(DEBUG) || defined(_DEBUG)
	strVersion += _T(" DEBUG");
#endif

	return strVersion;
}

//////////////////////////////////////////////////////////////////

void AboutBox::onEventButton_Support(wxCommandEvent & /*e*/)
{
	SupportDlg dlg(this, m_pGuiFrameActive);
	dlg.ShowModal();
}
