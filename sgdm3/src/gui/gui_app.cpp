// gui_app.cpp
//////////////////////////////////////////////////////////////////

#include <ConfigPch.h>
#include <ConfigDcl.h>
#include <util.h>
#include <rs.h>
#include <fim.h>
#include <poi.h>
#include <fd.h>
#include <fs.h>
#include <fl.h>
#include <de.h>
#include <xt.h>
#include <gui.h>


//////////////////////////////////////////////////////////////////

/*extern*/ MyBusyInfo *					gpMyBusyInfo =				NULL;
/*extern*/ MyFindData *					gpMyFindData =				NULL;
/*extern*/ FrameFactory *				gpFrameFactory =			NULL;
/*extern*/ ViewFileFont *				gpViewFileFont =			NULL;
/*extern*/ ViewFolder_ImageList *		gpViewFolderImageList =		NULL;
/*extern*/ ViewFolder_ListItemAttr *	gpViewFolderListItemAttr =	NULL;
/*extern*/ wxPrintData *				gpPrintData	=				NULL;
/*extern*/ wxPageSetupData *			gpPageSetupData =			NULL;
/*extern*/ GlobalProps *				gpGlobalProps =				NULL;
/*extern*/ poi_item_table *				gpPoiItemTable =			NULL;
/*extern*/ fd_fd_table *				gpFdFdTable =				NULL;
/*extern*/ fs_fs_table *				gpFsFsTable =				NULL;
/*extern*/ fim_buf_table *				gpFimBufTable =				NULL;
/*extern*/ fim_ptable_table *			gpPTableTable =				NULL;
/*extern*/ rs_ruleset_table *			gpRsRuleSetTable =			NULL;
/*extern*/ xt_tool_table *				gpXtToolTable =				NULL;

#ifdef DEBUGMEMORYSTATS
DebugMemoryStats gDebugMemoryStats;
#endif

#ifdef DEBUGUTILPERF
util_perf gUtilPerf;
#endif

//////////////////////////////////////////////////////////////////

#if defined(__WXMAC__)
#include <ApplicationServices/ApplicationServices.h>
static void _hack__force_us_to_foreground(void)
{
	//wxLogTrace(wxTRACE_Messages, _T("Trying to bring to foreground..."));

	// when we get launched from a command line/terminal we don't get
	// brought to the foreground.  (the terminal is probably still in
	// front.)  this means that the user has to click on the main
	// diffmerge window before they can do anything (and before the
	// system menu bar changes to us).
	//
	// note: both wxApp::IsActive() and wxFrame::IsActive() lie to us.
	// note: and raiseFrame() doesn't do anything.
	//
	// the following is trick is to try to make us the foreground/active
	// application.  this isn't needed when we are launched using one
	// of the gui methods (such as finder or the dock).

	ProcessSerialNumber PSN;
	GetCurrentProcess(&PSN);
	TransformProcessType(&PSN,kProcessTransformToForegroundApplication);
	SetFrontProcess(&PSN);
}
#endif

//////////////////////////////////////////////////////////////////

IMPLEMENT_APP(gui_app);

//////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(gui_app, wxApp)

	EVT_QUERY_END_SESSION(gui_app::onQueryEndSession)
	EVT_END_SESSION(gui_app::onEndSession)

END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////

gui_app::gui_app(void)
	: m_pXtStartup(NULL)
	, m_pFrameXt(NULL)
	, m_exitStatusType(MY_EXIT_STATUS_TYPE__STATIC)
	, m_exitStatusValue(MY_EXIT_STATUS__OK)
	, m_pFrameMergeResult(NULL)
	, m_pPoiMergeResult(NULL)
{
	// the constructors we call here should be pretty minimal
	// since wxWidgets is not really up-n-running yet.  they
	// should just allocate (any maybe zero) and return.  save
	// the good stuff for OnInit() below.

#ifdef DEBUGMEMORYSTATS
	gDebugMemoryStats.ZeroStats();
#endif

#if defined(__WXMAC__)
	_hack__force_us_to_foreground();
#endif

}

gui_app::~gui_app(void)
{
	// WARNING: don't do cleanup here -- see OnExit().
}

//////////////////////////////////////////////////////////////////

void gui_app::_cleanup(void)
{
	DELETEP(gpMyBusyInfo);
	DELETEP(gpMyFindData);
	DELETEP(gpPrintData);
	DELETEP(gpPageSetupData);
	DELETEP(gpPTableTable);
	DELETEP(gpFimBufTable);
	DELETEP(gpFrameFactory);
	DELETEP(gpFdFdTable);
	DELETEP(gpFsFsTable);
	DELETEP(gpPoiItemTable);
	DELETEP(gpViewFileFont);
	DELETEP(gpViewFolderImageList);
	DELETEP(gpViewFolderListItemAttr);
	DELETEP(gpRsRuleSetTable);
	DELETEP(gpXtToolTable);
	DELETEP(gpGlobalProps);		// must be last -- since objects may have GP Callbacks installed.
}

int gui_app::OnExit(void)
{
	// we are called after all windows and controls are destroyed,
	// but before wxWidgets starts cleaning up.

	// if we put something on the clipboard, leave it there (otherwise, by default when wxTheClipboard is
	// destroyed, it will clear the clipboard).  this may leak a little memory on windows.

    if (wxTheClipboard->IsOpened())
    {
        wxTheClipboard->Flush();
    }

	_cleanup_help();
	_cleanup();

	return wxApp::OnExit();
}

//////////////////////////////////////////////////////////////////

#include <Resources/Banner/banner.xpm>

//////////////////////////////////////////////////////////////////

bool gui_app::OnInit(void)
{
	wxString suppressSizerChecks;
	if (!wxGetEnv(_T("WXSUPPRESS_SIZER_FLAGS_CHECK"), &suppressSizerChecks) || suppressSizerChecks.IsEmpty())
		wxSetEnv(_T("WXSUPPRESS_SIZER_FLAGS_CHECK"), _T("TRUE"));

	wxLog::SetTimestamp(_T(""));	// turn off printing of timestamp in log messages (primarily for gtk)
#ifdef DEBUG
	// TODO see if we can move these AddTraceMask's to gui_app::gui_app()
	// TODO so that we get debug output during setup.
	wxLog::AddTraceMask(wxTRACE_Messages);
//#define TRACE_STRCONV _T("strconv")
//	wxLog::AddTraceMask(TRACE_STRCONV);
#ifdef __WXMSW__
	wxLog::AddTraceMask(wxTRACE_OleCalls);
#endif
	wxLog::AddTraceMask(TRACE_UTIL_ENC);
	wxLog::AddTraceMask(TRACE_BATCH);
	wxLog::AddTraceMask(TRACE_DE_DUMP);
	wxLog::AddTraceMask(TRACE_XT_DUMP);
	wxLog::AddTraceMask(TRACE_RS_DUMP);
	wxLog::AddTraceMask(TRACE_FLRUN_DUMP);
	wxLog::AddTraceMask(TRACE_FLLINE_DUMP);
	wxLog::AddTraceMask(TRACE_FLFL_DUMP);
	wxLog::AddTraceMask(TRACE_POI_DUMP);
	wxLog::AddTraceMask(TRACE_FD_DUMP);
	wxLog::AddTraceMask(TRACE_FS_DUMP);
	wxLog::AddTraceMask(TRACE_CREC_DUMP);
	wxLog::AddTraceMask(TRACE_FIM_DUMP);
	wxLog::AddTraceMask(TRACE_FRAG_DUMP);
	wxLog::AddTraceMask(TRACE_PTABLE_DUMP);
	wxLog::AddTraceMask(TRACE_UTIL_DUMP);
//	wxLog::AddTraceMask(TRACE_GUI_DUMP);
	wxLog::AddTraceMask(TRACE_GLOBALPROPS);
#ifdef DEBUGUTILPERF
	wxLog::AddTraceMask(TRACE_PERF);
#endif
#endif

#ifdef DEBUG
	wxLog::AddTraceMask(TRACE_LICENSE);
	wxLog::AddTraceMask(TRACE_NETWORK);
#endif

#if defined(DEBUG) && defined(__WXMSW__)
	wxLog::AddTraceMask(TRACE_LNK);
#endif

	SetAppName( VER_APP_NAME ); // use official exe name (used for 'Usage: %s [-h]...' and Win32 MessageBox() titles)

	// add support for PNG image types to wxImage (needed for toolbar icons)

	wxImage::AddHandler(new wxPNGHandler());

	wxFileSystem::AddHandler(new wxArchiveFSHandler);
	wxFileSystem::AddHandler(new wxMemoryFSHandler);

	// WARNING: wxMemoryFSHandler is a global/singleton/static class
	// WARNING: so there is only one instance and everything added to
	// WARNING: it is global.  So we have to be careful to use unique
	// WARNING: names for html and images that we add to it because
	// WARNING: there may be other dialogs open that are also using it.
	// WARNING: Here we preload the common stuff that lots of dialogs
	// WARNING: will use.

	gpGlobalProps = new GlobalProps(VER_REG_APP_NAME, VER_REG_VENDOR);
	gpGlobalProps->OnInit();

	if (!_OnInit_cl_args())		// this calls base class OnInit() and deals with command line processing.
	{
		// if we get a command line error, don't open first window.
		// by returning false, we tell wxApp to shutdown instead of
		// starting us up.  in that case, OnExit() DOES NOT get called.
		// so we need to do the cleanup here.
		//
		// NOTE: as of 3.3 this should not happen because the arg parser
		// always returns true (and sets m_cl_args.bParseError) so that
		// we can set a proper exit status.

		_cleanup();
		return false;	// this causes process to exit with -1 status.
	}

	gpRsRuleSetTable = new rs_ruleset_table();
	gpXtToolTable = new xt_tool_table();
	gpViewFileFont = new ViewFileFont();		// TODO see if we can lazy load this
	// gpViewFolderImageList -- lazy load if needed
	// gpViewFolderListItemAttr -- lazy load if needed
	// gpPrintData -- lazy load if needed
	// gpPageSetupData -- lazy load if needed
	gpFrameFactory = new FrameFactory();
	gpPoiItemTable = new poi_item_table();
	gpFdFdTable = new fd_fd_table();
	gpFsFsTable = new fs_fs_table();
	gpFimBufTable = new fim_buf_table();
	gpPTableTable = new fim_ptable_table();
	gpMyFindData = new MyFindData();
	gpMyBusyInfo = new MyBusyInfo();

	gpRsRuleSetTable->OnInit();
	gpXtToolTable->OnInit();
	gpViewFileFont->OnInit();
	gpPoiItemTable->OnInit();

	if (m_cl_args.bParseErrors)
	{
		// return true here ***WITHOUT CREATING ANY WINDOWS***
		// and let OnRun() exit with proper status.  it is a little
		// silly that we need to do all of the above OnInit() calls,
		// but it is safer.

		return true;
	}

#ifdef FEATURE_BATCHOUTPUT
	if (m_cl_args.bDumpDiffs)
	{
		// return true here ***WITHOUT CREATING ANY WINDOWS***
		// and let actual diff to be run during OnRun().
		// this is necessary so that we can set the exit status.
		return true;
	}
#endif

	// set various wxWindows global attributes

	wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);		// only send idle-events to windows that explicitly request them.

#if defined(__WXMSW__)
	// on MSW, the toolbar sometimes tries to remap colors within the tb icons
	// to the system colors.  disable this.  the toolbar page also describes a
	// setting to permit 32bit(w/ alpha) images to be used -- but only when on
	// XP.  try to use this.  this is documented in both wxWidgets-2.6.3 and 2.8.0.
	// but they do behave differently when this is omitted.  that is, 2.8.0 seems
	// to work without it, but 2.6.3 needs it.

	if (wxTheApp->GetComCtl32Version() >= 600  &&  ::wxDisplayDepth() >= 32)
		wxSystemOptions::SetOption(wxT("msw.remap"),2);
	else
		wxSystemOptions::SetOption(wxT("msw.remap"),0);
#endif

#if 0 // 2012/07/30 disable this for initial port to 2.9
#if defined(__WXMAC__)
	// on mac as of 2.8.0 we have 2 choices for wxListCtrl -- native or generic.
#define UseGeneric	1
#define UseNative	0
	wxSystemOptions::SetOption(wxMAC_ALWAYS_USE_GENERIC_LISTCTRL,UseNative);
#endif
#endif

	// create first top-level window and populate it with the files/folders.
	// ***BUT*** if we are configured to open files of this type in an external
	// tool, it returns information about the tool.

	gui_frame * pFrame = gpFrameFactory->openFirstFrame(&m_cl_args,&m_pXtStartup,m_strXtExe,m_strXtArgs);

	if (m_pXtStartup)
	{
		// we may or may not have a frame window (depends upon if openFirstFrame()
		// needed to raise a properly-parented dialog).  if we do, it is an "empty"
		// frame window.  remember it for now -- we have to wait until OnRun() to
		// dispose of it.

		m_pFrameXt = pFrame;
		return true;
	}

	// initialize the help sub-system -- we have exactly one of these

	_init_help();

	return true;
}

//////////////////////////////////////////////////////////////////

wxString gui_app::getExePathnameString(void) const
{
#if defined(__WXMSW__)

	// WARNING: argv[0] usually contains the pathname of the executable,
	//          but there are some times when it doesn't.  so, to isolate
	//          us from this weirdness, we get it the hard way. (sigh)

	wxChar bufPath[MAX_PATH+10];
	memset(bufPath,0,sizeof(bufPath));
	GetModuleFileNameW(NULL,bufPath,NrElements(bufPath)-1);
	wxString strPath(bufPath);

//	wxLogTrace(wxTRACE_Messages,_T("getExePathnameString: [argv[0] %s][gmfn %s]"),argv[0],bufPath);

	return strPath;
#endif

#if defined(__WXMAC__)
	// TODO try to use something based upon something equivalent
	// TODO to Win32's GetModuleFileName() to get the full pathname
	// TODO of the .app.
	//
	// TODO for now, we assume that argv[0] contains a full pathname
	// TODO to the exe.  that is, .../DiffMerge.app/Contents/MacOS/DiffMerge

	return argv[0];
#endif

#if defined(__WXGTK__)
	return argv[0];
#endif
}

//////////////////////////////////////////////////////////////////

void gui_app::_init_help(void)
{

#if defined(__WXMSW__)
	// we assume that DiffMerge.chm is installed in the same
	// folder as DiffMerge.exe (because we did the installation).
	//

	wxString strExe = getExePathnameString();
	wxFileName fn(strExe);
	m_strHelpManualPDF= fn.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR) + _T("DiffMerge.pdf");
#endif

#if defined(__WXMAC__)
	// we assume that DiffMergeManual.pdf is installed inside
	// our .app in Contents/Resources.  Spawn Preview.app to
	// display it.  This isn't as nice as a real html help
	// system, but it works for now.
	//
	// we assume that getExePathnameString() contains a full pathname
	// the exe.  that is, .../DiffMerge.app/Contents/MacOS/DiffMerge
	//
	// so we lop of /MacOS/DiffMerge and replace it with /Resouces/DiffMergeManual.pdf

	wxString strExe = getExePathnameString();
	wxFileName fn(strExe);
	fn.RemoveLastDir();
	fn.AppendDir( _T("Resources") );
	fn.SetFullName( _T("DiffMergeManual.pdf") );

	m_strHelpManualPDF = fn.GetFullPath();
#endif

#if defined(__WXGTK__)
	// we assume that DiffMergeManual.pdf is installed in /usr/share/doc/diffmerge.
	// spawn gnome's pdf viewer "evince" to display it.  This isn't as nice as a
	// real html help system, but it works.

	// on Ubuntu we put this in /usr/share/doc/diffmerge/*.pdf
	// on Fedora we put this in /usr/share/doc/diffmerge-VERSION/*.pdf    (eg. 3.3.0.1000.Preview)

	m_strHelpManualPDF = _T("/usr/share/doc/diffmerge/DiffMergeManual.pdf");
	if (!wxFile::Exists(m_strHelpManualPDF))
	{
		wxString strVersion;
		wxString strBuildLabel(VER_BUILD_LABEL);

		if (strBuildLabel.StartsWith(_T("@"))				// if optional BUILDLABEL was NOT set by script running compiler
			|| strBuildLabel.StartsWith(_T("_BLANK_"))		// of if it contains a placeholder.
			|| strBuildLabel.IsEmpty())
			strVersion.Printf(_T("%d.%d.%d.%d"),			// then we don't want to include it in the version number line.
							  VER_MAJOR_VERSION,VER_MINOR_VERSION,VER_MINOR_SUBVERSION,VER_BUILD_NUMBER);
		else
			strVersion.Printf(_T("%d.%d.%d.%d.%s"),			// otherwise we need it.
							  VER_MAJOR_VERSION,VER_MINOR_VERSION,VER_MINOR_SUBVERSION,VER_BUILD_NUMBER,strBuildLabel.wc_str());

		m_strHelpManualPDF.Printf(_T("/usr/share/doc/diffmerge-%s/DiffMergeManual.pdf"),strVersion.wc_str());
		if (!wxFile::Exists(m_strHelpManualPDF))
		{
			// ??? what to do.  maybe this is a debug build or they just extracted the files somewhere.
			// ??? but since we can't get the path of the exe on Linux, we are kinda stuck.  substitute
			// ??? a relative path and hope for the best....

			m_strHelpManualPDF = _T("DiffMergeManual.pdf");
		}
	}
#endif

}

void gui_app::_cleanup_help(void)
{
#if defined(__WXMSW__)
	// the user can close Preview at their leisure.
#endif

#if defined(__WXMAC__)
	// the user can close Preview at their leisure.
#endif

#if defined(__WXGTK__)
	// the user can close "evince" at their leisure.
#endif
}

void gui_app::ShowHelpContents(void)
{
#if defined(__WXMSW__)
	// on the MAC, we exec "Preview DiffMergeManual.pdf" as a stand-alone application.
	// the mac takes care of duplicate invocations.

	wxString strCmd = wxString::Format(_T("start \"%s\""),m_strHelpManualPDF.wc_str());

	/*long lResult = */
	wxExecute(strCmd,wxEXEC_ASYNC,NULL);

	//wxLogTrace(wxTRACE_Messages,_T("Exec of [%s] [result %ld]"),strCmd.wc_str(),lResult);
#endif

#if defined(__WXMAC__)
	// on the MAC, we exec "Preview DiffMergeManual.pdf" as a stand-alone application.
	// the mac takes care of duplicate invocations.

	wxString strCmd = wxString::Format(_T("open \"%s\""),m_strHelpManualPDF.wc_str());

	/*long lResult = */
	wxExecute(strCmd,wxEXEC_ASYNC,NULL);

	//wxLogTrace(wxTRACE_Messages,_T("Exec of [%s] [result %ld]"),strCmd.wc_str(),lResult);
#endif

#if defined(__WXGTK__)
	// on Linux/ubuntu, we exec "/usr/bin/evince DiffMergeManual.pdf" as a stand-alone
	// application.
	//
	// TODO determine if we have to manage multiple instances.

	wxString strCmd = wxString::Format(_T("/usr/bin/evince \"%s\""),m_strHelpManualPDF.wc_str());

	/*long lResult = */
	wxExecute(strCmd,wxEXEC_ASYNC,NULL);

	//wxLogTrace(wxTRACE_Messages,_T("Exec of [%s] [result %ld]"),strCmd.wc_str(),lResult);
#endif
}

//////////////////////////////////////////////////////////////////

void gui_app::ShowWebhelp(void)
{
	wxString strURL = util_make_url(T__URL__WEBHELP, false);

	wxLaunchDefaultBrowser(strURL);
}

//////////////////////////////////////////////////////////////////

void gui_app::ShowVisitSourceGear(void)
{
	wxString strURL = util_make_url(T__URL__VISIT, true);

	wxLaunchDefaultBrowser(strURL);
}

//////////////////////////////////////////////////////////////////

int gui_app::OnRun(void)
{
	// the main run loop.

	if (m_cl_args.bParseErrors)
	{
		// we had some type of command line error (syntax, file not found, etc).
		// exit with proper status.  if the parser wrote a message to the log,
		// it may get put up in a msgbox before we actually exit.

		return MY_EXIT_STATUS__CLARG_SYNTAX;
	}

#ifdef FEATURE_BATCHOUTPUT
	// if --diff used on command line, then we didn't create an initial
	// window in OnInit() and we need to actually do the batchoutput and
	// exit with a proper exit code.

	if (m_cl_args.bDumpDiffs)
	{
		// TODO cause stock/builtin rulesets to be loaded so that test results
		// TODO aren't biased by interactive user's settings.
		//
		// TODO force detail-level to lines-only
		//
		// TODO force us to not omit anything
		//
		// TODO force us to not hide unimportant

		// when dump-diffs is given we dump diff output to a file rather than opening a window.

		bool bHadChanges = false;
		util_error ue = _dumpDiffs(&bHadChanges);

		// our return code becomes the exit status.
		//
		// our exit status is defined as:
		//
		// if we got an error parsing the command line, we exit with 3 (above).
		//
		// if we get an error trying to do the diff (files read-only, bogus char enc,
		// etc), then we exit with 2.
		//
		// if there were changes between the 2 input files, exit with 1.
		//
		// if the 2 files were identical, we exit with 0.

		if (ue.isErr())
			return MY_EXIT_STATUS__FILE_ERROR;
		if (bHadChanges)
			return MY_EXIT_STATUS__DIFFERENT;

		return MY_EXIT_STATUS__OK;
	}
#endif

	if (m_pXtStartup)
	{
		// invoke external tool and exit with proper exit status.

		if (m_pFrameXt)
		{
			// if a frame was created (for parenting dialogs), we need to close it
			// before we spawn the external tool.  (it kinda looks stupid to have
			// a stuck/dead/empty window on screen.)
			//
			// we issue a close and then run a cheap version of the event loop
			// so that the defered-close stuff has a chance to run during an
			// idle event.

			m_pFrameXt->Close(true);
			wxApp::OnRun();
		}

		// now that the window is off the screen, synchronously spawn the
		// external tool and wait for it's exit status.  we return it as
		// ours.  this keeps shell scripts happy (hopefully).

		int exitStatusXt = gui_app::spawnSyncXT(m_pXtStartup,m_strXtExe,m_strXtArgs);
		return exitStatusXt;
	}

	// do the real windowing event loop.

	wxApp::OnRun();

	//wxLogTrace(wxTRACE_Messages,_T("gui_app::OnRun() exiting with status %d"),m_exitStatusValue);
	return m_exitStatusValue;
}

//////////////////////////////////////////////////////////////////

/*static*/ int gui_app::spawnSyncXT(const xt_tool * /*pxt*/,
									const wxString & rStrXtExe,
									const wxString & rStrXtArgs)
{
	// spawn the external tool and wait and return the exit
	// status from the child process.  this allows the
	// main program to block until the child exits (and
	// keep any shell scripts that started us happy).
	//
	// NOTE: it is up to the external tool to behave nicely
	// NOTE: and exit when the window we requested is closed.
	// NOTE: Notepad and Wordpad do this properly.  But, on
	// NOTE: windows at least, emacs seems to do an extra
	// NOTE: fork and return immediately while the window is
	// NOTE: still up.  so we can only guarantee that we'll
	// NOTE: ***TRY*** to wait for the child and collect
	// NOTE: the status code.

	wxString strCmd = rStrXtExe + _T(" ") + rStrXtArgs;

	// divert error log to local variable so that we
	// can control any error dialogs.

	util_error ue;
	long result;

	{
		util_logToString uLog(&ue.refExtraInfo());
		result = ::wxExecute(strCmd,wxEXEC_SYNC);
	}

	if (result == -1)
	{
		ue.set(util_error::UE_CANNOT_SPAWN_TOOL);
		//wxLogTrace(TRACE_XT_DUMP,_T("%s"), ue.getMBMessage().wc_str());

		wxMessageDialog dlg(NULL,ue.getMBMessage().wc_str(),VER_APP_TITLE,wxOK|wxICON_ERROR);
		dlg.ShowModal();
		return result;
	}

	// otherwise, result is the exit code from the child process.
	// normally, this is zero.

	//wxLogTrace(TRACE_XT_DUMP,_T("External tool spawned [exit code %ld]"),result);
	return result;
}

//////////////////////////////////////////////////////////////////

/*static*/ void gui_app::spawnAsyncXT(const xt_tool * pxt,
									  const wxString & rStrXtExe,
									  const wxString & rStrXtArgs)
{
	// spawn the external tool and don't wait for it to exit.
	// this is used when we have one or more frame windows on
	// screen and they want to open a new window (either via
	// the open-interlude dialog or by double-clicking on a
	// set of files in a folder window).

	wxString strCmd = rStrXtExe + _T(" ") + rStrXtArgs;

	long result = ::wxExecute(strCmd,wxEXEC_ASYNC);
	if (result == 0)
	{
		wxString strError = wxString::Format(_("Could not start external tool [%s]."),
											 pxt->getName().wc_str());
		//wxLogTrace(TRACE_XT_DUMP,_T("%s"),strError.wc_str());

		wxMessageDialog dlg(NULL,strError,_("Error launching external tool!"),wxOK|wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
#if defined(__WXMSW__)
	if (result == -1)
	{
		// used dde to connect and send command to existing process.
		// we don't get process-id or any termination events.

		//wxLogTrace(TRACE_XT_DUMP,_T("External tool spawned via DDE"));
		return;
	}
#endif

	// result is pid of child process.

	//wxLogTrace(TRACE_XT_DUMP,_T("External tool spawned [pid %ld]"),result);
	return;
}

//////////////////////////////////////////////////////////////////

void gui_app::setExitStatusStatic(int status)
{
	m_exitStatusType = MY_EXIT_STATUS_TYPE__STATIC;
	m_exitStatusValue = status;
	m_pFrameMergeResult = NULL;
	m_pPoiMergeResult = NULL;
}

void gui_app::setExitStatusWaitForMergeResult(gui_frame * pFrameResult, poi_item * pPoiResult)
{
	m_exitStatusType = MY_EXIT_STATUS_TYPE__MERGE;
	m_exitStatusValue = -1;
	m_pFrameMergeResult = pFrameResult;
	m_pPoiMergeResult = pPoiResult;
}

//////////////////////////////////////////////////////////////////

#if defined(__WXMAC__)
//void gui_app::MacNewFile(void)
//{
//	wxMessageDialog dlg(NULL, _T("MacNewFile"), _T("MacNewFile"), wxOK);
//	dlg.ShowModal();
//}

void gui_app::MacOpenFiles(const wxArrayString & asFilenames)
{
#if defined(DEBUG)
	int kLimit = asFilenames.GetCount();
	int k;
	wxString strDebug = wxString::Format(_T("MacOpenFiles: [count %d]:\n"), kLimit);
	for (k=0; k<kLimit; k++)
		strDebug += wxString::Format( _T("\t[%d]: '%s'\n"), k, asFilenames[k].wc_str());

	wxMessageDialog dlg(NULL, strDebug, _T("MacOpenFiles"), wxOK);
	dlg.ShowModal();
#endif

	gpFrameFactory->openFrameRequestedByFinderOrDND(NULL, asFilenames);

}

//void gui_app::MacOpenURL(const wxString & strUrl)
//{
//	wxMessageDialog dlg(NULL, strUrl, _T("MacOpenURL"), wxOK);
//	dlg.ShowModal();
//}
//
//void gui_app::MacPrintFile(const wxString & strFilename)
//{
//	wxMessageDialog dlg(NULL, strFilename, _T("MacPrintFile"), wxOK);
//	dlg.ShowModal();
//}
//
//void gui_app::MacReopenApp(void)
//{
//	wxMessageDialog dlg(NULL, _T("MacReopenApp"), _T("MacReopenApp"), wxOK);
//	dlg.ShowModal();
//}

void gui_app::SG__MacOpenFilesViaService(const wxArrayString &asFilenames)
{
#if defined(DEBUG)
	int kLimit = asFilenames.GetCount();
	int k;
	wxString strDebug = wxString::Format(_T("SG__MacOpenFilesViaService: [count %d]:\n"), kLimit);
	for (k=0; k<kLimit; k++)
		strDebug += wxString::Format( _T("\t[%d]: '%s'\n"), k, asFilenames[k].wc_str());

	wxMessageDialog dlg(NULL, strDebug, _T("SG__MacOpenFilesViaService"), wxOK);
	dlg.ShowModal();
#endif

	gpFrameFactory->openFrameRequestedByFinderOrDND(NULL, asFilenames);

}
#endif

//////////////////////////////////////////////////////////////////

void gui_app::onQueryEndSession(wxCloseEvent & /*e*/)
{
	wxLogTrace(wxTRACE_Messages, _T("gui_app:onQueryEndSession()"));

	// TODO 2013/10/21 Do we want to poll the open windows
	// TODO            and see if they have any dirt that
	// TODO            should be saved?  And if so, veto it.
}

void gui_app::onEndSession(wxCloseEvent & /*e*/)
{
	wxLogTrace(wxTRACE_Messages, _T("gui_app:onEndSession() start..."));
	gpFrameFactory->closeAllFrames(true);
	wxLogTrace(wxTRACE_Messages, _T("gui_app:onEndSession() end..."));
}
