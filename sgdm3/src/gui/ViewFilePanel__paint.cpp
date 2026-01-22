// ViewFilePanel__paint.cpp
// paint-related portion of ViewFilePanel
//////////////////////////////////////////////////////////////////

#include <ConfigPch.h>
#include <ConfigDcl.h>
#include <util.h>
#include <fim.h>
#include <poi.h>
#include <fs.h>
#include <fl.h>
#include <rs.h>
#include <de.h>
#include <fd.h>
#include <gui.h>

//////////////////////////////////////////////////////////////////
// lines are drawn like this:
// [patch][lineNr][icon/gap] "|" [gap][content][patch]
//
// or:
//
// [patch][content][patch]

#define PIXELS_MARGIN_PATCH      4	// width of patch highlight beam
#define PIXELS_LEFT_MARGIN_GAP   4	// width of gaps
#define PIXELS_LEFT_MARGIN_BAR   1	// width of line between line numbers and content

#define PIXELS_HRULE			 1	// height of hrule between non-adjacent lines to indicate something hidden (like when showing diffs only)
#define PIXELS_CARET			 2

//////////////////////////////////////////////////////////////////

#ifdef DEBUGUTILPERF
static const wxChar * sszKey_paint = L"vfp_paint";
static const wxChar * sszKey_erase_bg = L"vfp_erase_bg";
static const wxChar * sszKey_attr_clr = L"vfp_attr_clr";
static const wxChar * sszKey_prep_str = L"vfp_prep_str";
static const wxChar * sszKey_ds = L"vfp_draw_string";
static const wxChar * sszKey_font = L"vfp_font";
static const wxChar * sszKey_brush2eol = L"vfp_brush2eol";
static const wxChar * sszKey_deRow = L"vfp_derow";
static const wxChar * sszKey_paint_init_1 = L"vfp_paint_init_1";
static const wxChar * sszKey_paint_init_2 = L"vfp_paint_init_2";
static const wxChar * sszKey_get_text_ext = L"vfp_get_text_ext";
static const wxChar * sszKey_draw_text = L"vfp_draw_text";
#endif

//////////////////////////////////////////////////////////////////

void ViewFilePanel::_recalc(void)
{
	//////////////////////////////////////////////////////////////////
	// recalculate the pixels-per-char width/height based upon our
	// (possibly new) font.  for this, we assume that we have a
	// fixed-pitch font -- that is, we keep pixels-per-char variables
	// for those places where that approximation is good-enough, like
	// the scrollbars -- it is NOT good enough for drawing tabs, though.
	//////////////////////////////////////////////////////////////////

	wxClientDC dc(this);
	dc.SetFont(*gpViewFileFont->getNormalFont());	// since this is an approximation, just use normal font.
	wxCoord xPixelsPerCol = dc.GetCharWidth();
	wxCoord yPixelsPerRow = dc.GetCharHeight();

	wxASSERT_MSG( ((xPixelsPerCol > 0) && (yPixelsPerRow > 0)), _T("Font Error!") );
	
#if defined(__WXMAC__)
	// WXBUG Font metrics are a little screwy on the MAC.  GetTextExtent()
	// WXBUG and GetCharWidth() report values that don't add up.  It's like
	// WXBUG DrawText() is doing kerning tricks -- even for fixed-pitch fonts,
	// WXBUG like Courier New and Monaco.  For example, GetTextExtent() on
	// WXBUG 12Pt Courier New reports 8, 15, and 73 for "X", "XX", and "XXXXXXXXXX".
#endif

	m_pixelsPerCol = xPixelsPerCol;
	m_pixelsPerRow = yPixelsPerRow;

	//wxLogTrace(wxTRACE_Messages,_T("VFP[%d]: pixels per [row %d][col %d]"),
	//		   m_kPanel,m_pixelsPerRow,m_pixelsPerCol);

	//////////////////////////////////////////////////////////////////
	// recalculate the left-margin parameters -- these are based upon
	// the size of the font and the number of lines in the document.
	// 
	// take number of lines in document (as opposed to rows in the de's
	// display list) and compute the number of digits required to print
	// a line number.
	//////////////////////////////////////////////////////////////////

	if (m_bShowLineNumbers)
	{
		m_nrDigitsLineNr = _computeDigitsRequiredForLineNumber();

		static const wxChar * gszZeroes = _T("00000000000000000000");
		wxString strTemp(gszZeroes,m_nrDigitsLineNr);
		wxCoord wText,hText;
		dc.GetTextExtent(strTemp,&wText,&hText);

		wxString strStar(_T("*"));
		wxCoord wStar,hStar;
		dc.GetTextExtent(strStar, &wStar, &hStar);

		m_xPixelLineNr      = PIXELS_MARGIN_PATCH;
		m_xPixelLineNrRight = m_xPixelLineNr + wText;
		m_xPixelIcon        = m_xPixelLineNrRight;
		m_xPixelBar         = m_xPixelLineNrRight + wStar;
		m_xPixelText        = m_xPixelBar + PIXELS_LEFT_MARGIN_BAR + PIXELS_LEFT_MARGIN_GAP;

//		wxLogTrace(wxTRACE_Messages, _T("VFP[%d]: LeftMargin [nrLines %d][ex %d][nrDigits %d][wText %d][%s]"),
//				   m_kPanel,m_pFlFl->getFormattedLineNrs(),ex,m_nrDigitsLineNr,wText,strTemp.wc_str());
	}
	else
	{
		m_xPixelLineNr		= 0;
		m_xPixelLineNrRight = 0;
		m_xPixelIcon        = 0;
		m_xPixelBar			= 0;
		m_xPixelText		= PIXELS_MARGIN_PATCH;
	}

	//////////////////////////////////////////////////////////////////
	// recalculate the number of rows/columns displayable in the main
	// portion of our client window -- these are based upon the size
	// of the font and the width of the left-margin stuff and the size
	// of our window.
	//
	// again, the column values are an approximation based upon the
	// assumption of a fixed-width font.
	//////////////////////////////////////////////////////////////////

	int xPixelsClientSize, yPixelsClientSize;
	GetClientSize(&xPixelsClientSize,&yPixelsClientSize);

	xPixelsClientSize -= m_xPixelText;
	
	m_colsDisplayable = (xPixelsClientSize / m_pixelsPerCol);	// truncate down to whole col/row
	m_rowsDisplayable = (yPixelsClientSize / m_pixelsPerRow);
	
//	wxLogTrace(wxTRACE_Messages,_T("VFP[%d]: displayable [row %d][col %d]"),
//			   m_kPanel,m_rowsDisplayable,m_colsDisplayable);

	m_bRecalc = false;
}

//////////////////////////////////////////////////////////////////

int ViewFilePanel::getPixelsPerCol(void)
{
	if (m_bRecalc) _recalc();
	return m_pixelsPerCol;
}

int ViewFilePanel::getPixelsPerRow(void)
{
	if (m_bRecalc) _recalc();
	return m_pixelsPerRow;
}

//////////////////////////////////////////////////////////////////

int ViewFilePanel::getXPixelLineNr(void)
{
	if (m_bRecalc) _recalc();
	return m_xPixelLineNr;
}

int ViewFilePanel::getXPixelLineNrRight(void)
{
	if (m_bRecalc) _recalc();
	return m_xPixelLineNrRight;
}

int ViewFilePanel::getXPixelIcon(void)
{
	if (m_bRecalc) _recalc();
	return m_xPixelIcon;
}

int ViewFilePanel::getXPixelBar(void)
{
	if (m_bRecalc) _recalc();
	return m_xPixelBar;
}

int ViewFilePanel::getXPixelText(void)
{
	if (m_bRecalc) _recalc();
	return m_xPixelText;
}

int ViewFilePanel::getDigitsLineNr(void)
{
	if (m_bRecalc) _recalc();
	return m_nrDigitsLineNr;
}

//////////////////////////////////////////////////////////////////

int ViewFilePanel::getColsDisplayable(void)
{
	if (m_bRecalc) _recalc();
	return m_colsDisplayable;
}

int ViewFilePanel::getRowsDisplayable(void)
{
	if (m_bRecalc) _recalc();
	return m_rowsDisplayable;
}

//////////////////////////////////////////////////////////////////

void _attr_to_color2_fg(de_attr attr, wxColour & fg)
{
	// map de_attr to a foreground color for text

	bool bUnimportant = DE_DOP__IS_SET(attr,DE_ATTR_UNIMPORTANT);

	de_attr type = (attr & DE_ATTR__TYPE_MASK);

	switch (type)
	{
	default:
		wxASSERT_MSG( (0), _T("Coding Error") );
	case DE_ATTR_EOF:
	case DE_ATTR_DIF_2EQ:
		if (bUnimportant)
			fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_ALL_EQ_UNIMP_FG);
		else
			fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_ALL_EQ_FG);
		return;

	case DE_ATTR_OMITTED:
		fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_OMIT_FG);
		return;

	case DE_ATTR_DIF_0EQ:
		if (bUnimportant)
			fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_NONE_EQ_UNIMP_FG);
		else
			fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_NONE_EQ_FG);
		return;
	}
}

bool _attr_to_color2_bg(bool bIntraLine, bool bIgnoreUnimportantChanges, de_attr attr, wxColour & bg)
{
	// map de_attr to a background color for text
	// return true if this non-equal (where intra-line distinction is significant)

	bool bUnimportant = DE_DOP__IS_SET(attr,DE_ATTR_UNIMPORTANT);

	de_attr type = (attr & DE_ATTR__TYPE_MASK);

	switch (type)
	{
	default:
		wxASSERT_MSG( (0), _T("Coding Error") );
	case DE_ATTR_EOF:
	case DE_ATTR_DIF_2EQ:
		bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_ALL_EQ_BG);
		return false;

	case DE_ATTR_OMITTED:
		bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_OMIT_BG);
		return false;

	case DE_ATTR_DIF_0EQ:
		if (bUnimportant && bIgnoreUnimportantChanges)
		{
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_ALL_EQ_BG);
			return false;
		}
		else
		{
			if (bIntraLine)
				bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_NONE_EQ_IL_BG);
			else
				bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_NONE_EQ_BG);
			return true;
		}
	}
}

//////////////////////////////////////////////////////////////////

void _attr_to_color3_fg(PanelIndex kPanel, de_attr attr, wxColour & fg)
{
	// map (kPanel,attr) to a foreground color for text

	bool bOdd;
	bool bUnimportant = DE_DOP__IS_SET(attr,DE_ATTR_UNIMPORTANT);

	de_attr type = (attr & DE_ATTR__TYPE_MASK);
	
	switch (type)
	{
	default:
		wxASSERT_MSG( (0), _T("Coding Error") );
	case DE_ATTR_EOF:
	case DE_ATTR_MRG_3EQ:
		if (bUnimportant)
			fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_ALL_EQ_UNIMP_FG);
		else
			fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_ALL_EQ_FG);
		return;

	case DE_ATTR_OMITTED:
		fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_OMIT_FG);
		return;

	case DE_ATTR_MRG_0EQ:
		if (bUnimportant)
			fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_CONFLICT_UNIMP_FG);
		else
			fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_CONFLICT_FG);
		return;

	// otherwise, 2 are equal and 1 is different -- the odd one

	case DE_ATTR_MRG_T0T2EQ:	bOdd = (kPanel == PANEL_T1);	break;
	case DE_ATTR_MRG_T1T2EQ:	bOdd = (kPanel == PANEL_T0);	break;
	case DE_ATTR_MRG_T1T0EQ:	bOdd = (kPanel == PANEL_T2);	break;
	}

	if (bUnimportant)
		fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_SUB_UNIMP_FG);
	else if (bOdd)
		fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_SUB_NOTEQUAL_FG);
	else
		fg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_SUB_EQUAL_FG);
}

bool _attr_to_color3_bg(bool bIntraLine, bool bIgnoreUnimportantChanges, PanelIndex kPanel, de_attr attr, wxColour & bg)
{
	// map (kPanel,attr) to a background color for text
	// return true if non-equal (where intra-line would be significant)

	bool bOdd;
	bool bConflict = DE_DOP__IS_SET(attr,DE_ATTR_CONFLICT);
	bool bUnimportant = DE_DOP__IS_SET(attr,DE_ATTR_UNIMPORTANT);

	de_attr type = (attr & DE_ATTR__TYPE_MASK);
	
	switch (type)
	{
	default:
		wxASSERT_MSG( (0), _T("Coding Error") );
	case DE_ATTR_EOF:
	case DE_ATTR_MRG_3EQ:
		//wxASSERT_MSG( (!bConflict), _T("Coding Error") );	// TODO with intra-line enabled, we shouldn't get this.
		if (bConflict)										// TODO we can remove this.
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_CONFLICT_BG);
		else
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_ALL_EQ_BG);
		return false;

	case DE_ATTR_OMITTED:
		bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_OMIT_BG);
		return false;

	case DE_ATTR_MRG_0EQ:
		wxASSERT_MSG( (bConflict), _T("Coding Error") );

		if (bUnimportant && bIgnoreUnimportantChanges)
		{
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_ALL_EQ_BG);
// see note[1] in TODO list		
//			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_CONFLICT_BG);
			return false;
		}
		else
		{
			if (bIntraLine)
				bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_CONFLICT_IL_BG);
			else
				bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_CONFLICT_BG);
			return true;
		}

	// otherwise, 2 are equal and 1 is different -- the odd one

	case DE_ATTR_MRG_T0T2EQ:	bOdd = (kPanel == PANEL_T1);	break;
	case DE_ATTR_MRG_T1T2EQ:	bOdd = (kPanel == PANEL_T0);	break;
	case DE_ATTR_MRG_T1T0EQ:	bOdd = (kPanel == PANEL_T2);	break;
	}

	if (bUnimportant && bIgnoreUnimportantChanges)
	{
// see note[1] in TODO list		
//		if (bConflict)
//			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_CONFLICT_BG);
//		else
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_ALL_EQ_BG);
		return false;
	}
	else if (bConflict)
		if (bIntraLine)
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_CONFLICT_IL_BG);
		else
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_CONFLICT_BG);
	else if (bOdd)
		if (bIntraLine)
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_SUB_NOTEQUAL_IL_BG);
		else
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_SUB_NOTEQUAL_BG);
	else
		if (bIntraLine)
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_SUB_EQUAL_IL_BG);
		else
			bg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_SUB_EQUAL_BG);
	return true;
}

//////////////////////////////////////////////////////////////////

void ViewFilePanel::_prepareStringForDrawing(const wxChar * sz, long len,
											 bool bDisplayInvisibles, int cColTabWidth, int & col,
											 wxString & strOut)
{
//	wxLogTrace(wxTRACE_Messages,_T("VFP:_prepString [len %d]"),len);
	UTIL_PERF_START_CLOCK(sszKey_prep_str);

	// prepare the given string for drawing.  we deal with
	// draw-invisibles and/or tabstops and/or CRLF.
	//
	// we assume that we are starting the string in column "col" and
	// update it with the new column number afterwards.
	//
	// we build the output string into strOut -- i know this seems
	// wasteful, but the dc.DrawText and dc.GetTextExtents only knows
	// how to use wxString's not "wxChar *"'s.

	//strOut.Alloc(strOut.Length() + len*2);	// don't bother with preallocate -- it just slows us down

	for (long k=0; k<len; k++)
	{
		switch (sz[k])
		{
		case 0x0009:	// TAB
			{
				static const wxChar * gszBlanks = _T("\x00bb                          ");			// draw a '>>' for a tab
				const wxChar * szBlanks = (bDisplayInvisibles) ? &gszBlanks[0] : &gszBlanks[1];

				long pad = cColTabWidth - (col % cColTabWidth);
				for (int j=0; j<pad; j++)
					strOut += szBlanks[j];
				col += pad;
			}
			break;
			
		case 0x0020:	// SPACE
			{
				wxChar ch = (bDisplayInvisibles) ? _T('\x00b7') : _T(' ');		// draw a middle dot for a space
				strOut += ch;
				col++;
			}
			break;
								
		case 0x000a:	// LF
			if (bDisplayInvisibles)
			{
				strOut += _T('\x00b6');			// draw a 'P' (paragraph) for a LF
				col++;
			}
			break;

		case 0x000d:	// CR
			if (bDisplayInvisibles)
			{
				strOut += _T('\x00ab');			// draw a '<<' character for CR
				col++;
			}
			break;

		default:								// use normal draw
			{
				strOut += sz[k];
				col++;
			}
			break;
		}
	}

	UTIL_PERF_STOP_CLOCK(sszKey_prep_str);
}

void ViewFilePanel::_drawCaret(wxDC & dc, int x, int y)
{
	wxColour clrCaret( gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_CARET_FG) );

	wxCoord wText,hText;

	UTIL_PERF_START_CLOCK(sszKey_get_text_ext);
	dc.GetTextExtent( _T("X"), &wText,&hText);
	UTIL_PERF_STOP_CLOCK(sszKey_get_text_ext);

	wxPen pen(clrCaret);

	dc.SetPen(pen);
	for (int k=0; k<PIXELS_CARET; k++)
		dc.DrawLine(x+k,y, x+k,y+hText);
	dc.SetPen(wxNullPen);
}

static void _fillTextSpanBackground(wxDC & dc, wxCoord x, wxCoord y, wxCoord w, int rowHeight, const wxColour & bg)
{
	if ((w <= 0) || (rowHeight <= 0))
		return;

	// Fill the entire row height so whitespace doesn't leave visible gaps.
	dc.SetPen(wxNullPen);
	dc.SetBrush(wxBrush(bg));
	dc.DrawRectangle(x, y, w, rowHeight);
}

void ViewFilePanel::_drawString(wxDC & dc,
								const wxChar * sz, long len,
								bool bDisplayInvisibles, int cColTabWidth,
								const wxColour & clrFg, const wxColour & clrBg,
								wxCoord & xArg, wxCoord y, int & colArg, int row)
{
	// draw the given string while dealing with draw-invisibles and/or tabstops and/or CRLF.
	// 
	// we update x with the pixel at the end of the string
	// and col with the new column number.

	UTIL_PERF_START_CLOCK(sszKey_ds);

	int colStart = colArg;
	int colEnd   = colArg;
	wxCoord xDraw = xArg;
		
	wxString strTemp;
	_prepareStringForDrawing(sz,len,bDisplayInvisibles,cColTabWidth,colEnd,strTemp);	// this updates colEnd and strTemp
	colArg = colEnd;	// update colArg in caller

	if (strTemp.Length() == 0)
	{
		UTIL_PERF_STOP_CLOCK(sszKey_ds);
		return;
	}
	
	int yPixelsPerRow = getPixelsPerRow();
	bool bPanelWithFocus = (m_kPanel == m_pViewFile->getPanelWithFocus());

	wxCoord wText,hText;
	UTIL_PERF_START_CLOCK(sszKey_get_text_ext);
	dc.GetTextExtent(strTemp,&wText,&hText);
	UTIL_PERF_STOP_CLOCK(sszKey_get_text_ext);
	xArg += wText;

	// if no selection, we can just draw our span
	// normally.  but we may have to draw the caret.

	if (!haveSelection())
	{
		// draw entire string first.  if necessary
		// overlay the caret (so that the character
		// background doesn't erase it).

		_fillTextSpanBackground(dc,xDraw,y,wText,yPixelsPerRow,clrBg);
		dc.SetTextForeground(clrFg);
		dc.SetTextBackground(clrBg);
		UTIL_PERF_START_CLOCK(sszKey_draw_text);
		dc.DrawText(strTemp,xDraw,y);
		UTIL_PERF_STOP_CLOCK(sszKey_draw_text);

		if (bPanelWithFocus  &&  _containsCaret(row,colStart,colEnd))
		{
			// draw caret somewhere within our span -- but only if we have focus.

			int len2 = m_vfcCaret.getCol() - colStart;
			wxString str = wxString(strTemp.wc_str(),len2);
			UTIL_PERF_START_CLOCK(sszKey_get_text_ext);
			dc.GetTextExtent(str,&wText,&hText);
			UTIL_PERF_STOP_CLOCK(sszKey_get_text_ext);

			_drawCaret(dc,xDraw+wText,y);
		}
		
		UTIL_PERF_STOP_CLOCK(sszKey_ds);
		return;
	}

	// otherwise we have a selection.

	if (!_intersectSelection(row,colStart,colEnd))
	{
		// our span does not cross/intersect the selection,
		// so we can just draw it normally.
		
		_fillTextSpanBackground(dc,xDraw,y,wText,yPixelsPerRow,clrBg);
		dc.SetTextForeground(clrFg);
		dc.SetTextBackground(clrBg);
		UTIL_PERF_START_CLOCK(sszKey_draw_text);
		dc.DrawText(strTemp,xDraw,y);
		UTIL_PERF_STOP_CLOCK(sszKey_draw_text);

		if ((m_vfcCaret.compare(m_vfcSelection1) == 0) && (m_vfcCaret.compare(row,colStart) == 0))
		{
			// we have a forward selection (caret > anchor) and our string starts immediately
			// after the end of the selection, we need to draw the caret where we started.
			// (because we have decided to also draw the caret when we have a selection.)

			_drawCaret(dc,xDraw,y);
		}
		
		UTIL_PERF_STOP_CLOCK(sszKey_ds);
		return;
	}

	// if our span of text somehow intersects the selection, we may have to draw it in parts.

	if (!_withinSelection(row,colStart))
	{
		// our span starts to the left of the selection and then enters it.
		// draw first portion normally, then switch to selection mode.
		
		wxASSERT_MSG( (row == m_vfcSelection0.getRow()), _T("Coding Error"));
		wxASSERT_MSG( (colStart < m_vfcSelection0.getCol()), _T("Coding Error"));
				
		int lenPart1 = m_vfcSelection0.getCol() - colStart;
		wxString strPart1 = wxString(strTemp.wc_str(),lenPart1);

		UTIL_PERF_START_CLOCK(sszKey_get_text_ext);
		dc.GetTextExtent(strPart1,&wText,&hText);
		UTIL_PERF_STOP_CLOCK(sszKey_get_text_ext);
		_fillTextSpanBackground(dc,xDraw,y,wText,yPixelsPerRow,clrBg);
		dc.SetTextForeground(clrFg);
		dc.SetTextBackground(clrBg);
		UTIL_PERF_START_CLOCK(sszKey_draw_text);
		dc.DrawText(strPart1,xDraw,y);
		UTIL_PERF_STOP_CLOCK(sszKey_draw_text);
		xDraw += wText;

		wxString strPart2 = wxString(strTemp.wc_str()+lenPart1);
		strTemp = strPart2;
		colStart += lenPart1;
	}

	// strTemp (now) begins within the selection

	if (_withinSelection(row,colStart))
	{
		wxColour colorSelectionFg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_SELECTION_FG);
		wxColour colorSelectionBg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_SELECTION_BG);
		
		dc.SetTextForeground(colorSelectionFg);
		dc.SetTextBackground(colorSelectionBg);

		int resultCompareEnds = m_vfcSelection1.compare(row,colEnd);
		switch (resultCompareEnds)
		{
		default:		// quiets compiler
		case +1:		// the selection extends past the end of our span
		case  0:		// the selection ends exactly at the end of our span.
			// draw the rest of the string using the selection highlight coloring.

			UTIL_PERF_START_CLOCK(sszKey_get_text_ext);
			dc.GetTextExtent(strTemp,&wText,&hText);
			UTIL_PERF_STOP_CLOCK(sszKey_get_text_ext);
			_fillTextSpanBackground(dc,xDraw,y,wText,yPixelsPerRow,colorSelectionBg);
			UTIL_PERF_START_CLOCK(sszKey_draw_text);
			dc.DrawText(strTemp,xDraw,y);
			UTIL_PERF_STOP_CLOCK(sszKey_draw_text);

			// lay the caret on top of the selection.  this is somewhat questionable.
			// some apps do and some don't draw the caret when there is a selection.
			// 
			// if we have a backward selection (caret < anchor) (and since our temp
			// string now starts at the start of the selection), we may need to draw
			// the caret where we just drew the string.  (we have to do it after we
			// draw the string, so it doesn't get overwritten.)
			//
			// (if we have a forward selection (caret > anchor) and if the selection
			// ends exactly at the end of our we can't draw it here because it will
			// get overwritten when the next span of text is drawn -- they must do it.)

			if ((m_vfcCaret.compare(m_vfcSelection0) == 0) && (m_vfcCaret.compare(row,colStart) == 0))
				_drawCaret(dc,xDraw,y);
				
			UTIL_PERF_STOP_CLOCK(sszKey_ds);
			return;

		case -1:		// the selection ends before the end of our span.
			// find the split point and draw the first part using the selection
			// highlight coloring and the second part normally.

			wxASSERT_MSG( (row == m_vfcSelection1.getRow()), _T("Coding Error") );
			wxASSERT_MSG( (m_vfcSelection1.getCol() < colEnd),  _T("Coding Error") );
			wxASSERT_MSG( (colStart < m_vfcSelection1.getCol()),  _T("Coding Error") );

			int lenPart1 = m_vfcSelection1.getCol() - colStart;
			wxString strPart1 = wxString(strTemp.wc_str(),lenPart1);
			UTIL_PERF_START_CLOCK(sszKey_get_text_ext);
			dc.GetTextExtent(strPart1,&wText,&hText);
			UTIL_PERF_STOP_CLOCK(sszKey_get_text_ext);
			_fillTextSpanBackground(dc,xDraw,y,wText,yPixelsPerRow,colorSelectionBg);
			UTIL_PERF_START_CLOCK(sszKey_draw_text);
			dc.DrawText(strPart1,xDraw,y);
			UTIL_PERF_STOP_CLOCK(sszKey_draw_text);

			// if we have a backward selection (caret < anchor) (and since our temp
			// string now starts at the start of the selection), we may need to draw
			// the caret where we just drew the string.  (we have to do it after we
			// draw the string, so it doesn't get overwritten.)

			if ((m_vfcCaret.compare(m_vfcSelection0) == 0) && (m_vfcCaret.compare(row,colStart) == 0))
				_drawCaret(dc,xDraw,y);

			xDraw += wText;

			wxString strPart2 = wxString(strTemp.wc_str()+lenPart1);
			strTemp = strPart2;
			colStart += lenPart1;
		}
	}

	// draw tail portion in normal colors

	UTIL_PERF_START_CLOCK(sszKey_get_text_ext);
	dc.GetTextExtent(strTemp,&wText,&hText);
	UTIL_PERF_STOP_CLOCK(sszKey_get_text_ext);
	_fillTextSpanBackground(dc,xDraw,y,wText,yPixelsPerRow,clrBg);
	dc.SetTextForeground(clrFg);
	dc.SetTextBackground(clrBg);
	UTIL_PERF_START_CLOCK(sszKey_draw_text);
	dc.DrawText(strTemp,xDraw,y);
	UTIL_PERF_STOP_CLOCK(sszKey_draw_text);

	// if we have a forward selection (caret > anchor), we have to draw the caret
	// at the beginning of the tail portion (after we draw the string).

	if ((m_vfcCaret.compare(m_vfcSelection1) == 0) && (m_vfcCaret.compare(row,colStart) == 0))
		_drawCaret(dc,xDraw,y);

	UTIL_PERF_STOP_CLOCK(sszKey_ds);
}

void ViewFilePanel::_drawLongString(wxDC & dc,
									const wxChar * sz, long len,
									bool bDisplayInvisibles, int cColTabWidth,
									const wxColour & clrFg, const wxColour & clrBg,
									wxCoord & xArg, wxCoord y, int & colArg, int row)
{
	// BUG: there is bug in Windows ::TextOut() routine on XP (at least).  if you
	// BUG: give it a unicode string longer than 4095 characters, it doesn't display
	// BUG: anything.  In the manual, it says that it is limited to 8192 in the ansi
	// BUG: build on Win95,98,ME but it doesn't say anything about XP....
	// NOTE: the problem *does not* seem to affect the text measuring functions.
	// NOTE: see item:13396.
	//
	// so we chop up extremely long strings and emit them in pieces.

	while (len > 0)
	{
		long lenChunk = MyMin(len,4095);
		_drawString(dc,sz,lenChunk,bDisplayInvisibles,cColTabWidth,clrFg,clrBg,xArg,y,colArg,row);

		sz += lenChunk;
		len -= lenChunk;
	}
}

void ViewFilePanel::_fakeDrawString(wxDC & dc,
								const wxChar * sz, long len,
								bool bDisplayInvisibles, int cColTabWidth,
								wxCoord & x, int & col)
{
	// pretend to draw the given string while dealing with draw-invisibles and/or tabstops and/or CRLF.
	// we do this for the side-effects -- to update x,col
	// 
	// we update x with the pixel at the end of the string
	// and col with the new column number.

	wxString strTemp;
	_prepareStringForDrawing(sz,len,bDisplayInvisibles,cColTabWidth,col,strTemp);
	
	if (strTemp.Length() > 0)
	{
		wxCoord wText,hText;
		UTIL_PERF_START_CLOCK(sszKey_get_text_ext);
		dc.GetTextExtent(strTemp,&wText,&hText);
		UTIL_PERF_STOP_CLOCK(sszKey_get_text_ext);
		x += wText;
	}
}

//////////////////////////////////////////////////////////////////

void ViewFilePanel::OnPaint(wxPaintEvent & e)
{
	UTIL_PERF_START_CLOCK(sszKey_paint);
	_OnPaint(e);
	UTIL_PERF_STOP_CLOCK(sszKey_paint);

	UTIL_PERF_DUMP_ALL( wxString::Format(_T("After Paint: [view %ld][panel %ld]"),m_kSync,m_kPanel) );
}

void ViewFilePanel::_OnPaint(wxPaintEvent & /*e*/)
{
	int xPixelsClientSize, yPixelsClientSize;
	GetClientSize(&xPixelsClientSize,&yPixelsClientSize);

#if defined(__WXGTK__)
	// WXBUG The GTK version crashes if the window has a negative
	// WXBUG size and we try to create a bitmap (via wxBufferedPaintDC)
	// WXBUG or sometimes if we try operate on it (dc.SetBackground).
	// WXBUG 
	// WXBUG One could ask why we're getting negative sizes on any
	// WXBUG window -- but that's a bigger question....
	// 
	// as a work-around, let's test it before try to use it.
	// even if negative, we need to create a trivial wxPaintDC
	// to satisfy the event.
	//
	// since this is fairly harmless, i'll let this code run on all
	// platforms -- rather than ifdef it.
#endif

	if ((xPixelsClientSize <= 0) || (yPixelsClientSize <= 0))
	{
		wxPaintDC dc(this);
		return;
	}

	wxAutoBufferedPaintDC dc(this);

	UTIL_PERF_START_CLOCK(sszKey_erase_bg);
	wxBrush brushWindowBG( gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_WINDOW_BG) );
	dc.SetBackground(brushWindowBG);
	dc.Clear();
	UTIL_PERF_STOP_CLOCK(sszKey_erase_bg);

	if (!m_pDeDe || !m_pFlFl)	// we need this incase we get a paint before
		return;					// the document and layout is completely loaded.

	if (m_pDeDe->isRunBusy())	// HACK -- if we get an ASSERT dialog while the diff-engine is running,
		return;					// HACK -- we get a paint message -- this is bad.

	//wxLogTrace(wxTRACE_Messages,_T("ViewFilePanel::_OnPaint[kSync %d][kPanel %d] calling prePaint()"),getSync(),m_kPanel);
	m_pViewFile->prePaint();

	UTIL_PERF_START_CLOCK(sszKey_paint_init_1);

	dc.SetBackgroundMode(wxSOLID);				// background of text will be drawn.

	int kSync = getSync();
	bool bMerge = (m_nrTopPanels==3);
	bool bIgnoreUnimportantChanges = (DE_DOP__IS_SET_IGN_UNIMPORTANT(m_pViewFile->getDisplayOps(kSync)));

	int xColThumb = m_pViewFile->getScrollThumbCharPosition(m_kSync,wxHORIZONTAL);
	int yRowThumb = m_pViewFile->getScrollThumbCharPosition(m_kSync,wxVERTICAL);

	const TVector_Display * pDis = m_pDeDe->getDisplayList(kSync);

	int xPixelsPerCol = getPixelsPerCol();
	int yPixelsPerRow = getPixelsPerRow();
	
	int xPixelLineNrRight = getXPixelLineNrRight();		// x coordinate or right edge of line number
	int xPixelIcon        = getXPixelIcon();	// x coordinate of pre-line dirty icon
	
	int xPixelBar    = getXPixelBar();		// x coordinate of vertical line between line numbers and document area
	int xPixelText   = getXPixelText();		// x coordinate where document area begins

	int yRowEnd = yRowThumb + getRowsDisplayable() + 1;		// +1 to get partially visible row

	int xPixelThumb = xColThumb * xPixelsPerCol;	// convert thumb position from column number to pixel

	int yRowDisplayMax = (int)pDis->size();			// vector includes EOF row
	int yRowLimit      = MyMin(yRowDisplayMax, yRowEnd);

	UTIL_PERF_STOP_CLOCK(sszKey_paint_init_1);
	UTIL_PERF_START_CLOCK(sszKey_paint_init_2);

	wxColour colorEolUnknownFg( gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_EOL_UNKNOWN_FG) );
	wxColour colorLineNrFg(     gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_LINENR_FG) );
	wxColour colorLineNrBg(     gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_LINENR_BG) );
	wxColour colorTextFg;
	wxColour colorTextBg;
	wxColour colorSelectionBg = gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_SELECTION_BG);

	wxPen penHRule(colorLineNrFg, PIXELS_HRULE, wxSOLID);
	wxPen penNone(*wxBLACK,1,wxTRANSPARENT);		// a "no pen" pen -- needed for DrawRectangle() when we don't want borders

	wxBrush brushVoidHatch(gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_VOID_FG), wxBDIAGONAL_HATCH);	// hatched brush for filling voids
	wxBrush brushVoidBG   (gpGlobalProps->getColor(GlobalProps::GPL_FILE_COLOR_VOID_BG));	// background behind hatched brush

	wxBrush brushEOL;
	UTIL_PERF_STOP_CLOCK(sszKey_paint_init_2);

	UTIL_PERF_START_CLOCK(sszKey_font);
	const wxFont * pFontNormal = gpViewFileFont->getNormalFont();
	const wxFont * pFontBold   = gpViewFileFont->getBoldFont();
	bool bNormalFontInstalled  = true;
	dc.SetFont(*pFontNormal);
#define CONDITIONALLY_SET_NORMAL_FONT()	Statement( if (!bNormalFontInstalled) { dc.SetFont(*pFontNormal); bNormalFontInstalled=true;  } )
#define CONDITIONALLY_SET_BOLD_FONT()	Statement( if ( bNormalFontInstalled) { dc.SetFont(*pFontBold  ); bNormalFontInstalled=false; } )
#define CONDITIONALLY_SET_FONT(frProp)	Statement( if (FR_PROP_TEST((frProp),FR_PROP__INSERTED))		\
												   		CONDITIONALLY_SET_BOLD_FONT();					\
												   else													\
												   		CONDITIONALLY_SET_NORMAL_FONT();				)
	UTIL_PERF_STOP_CLOCK(sszKey_font);

	// lookup how EOL's were handled.  if globally-respected (included in matching),
	// then the intra-line sync-lists will include nodes for the EOL char(s) before
	// the last/end node (and we don't need to tack them onto the line when drawing
	// invisibles)..

	const rs_ruleset * pRS           = m_pViewFile->getDoc()->getFsFs()->getRuleSet();
	rs_context_attrs matchStripAttrs = pRS->getMatchStripAttrs();
	bool bGlobalRespectEOL           = RS_ATTRS_RespectEOL(matchStripAttrs);

	int cColTabWidth = m_pViewFile->getTabStop();

	bool bDisplayInvisibles = m_pViewFile->getPilcrow();
	bool bPanelWithFocus = (m_kPanel == m_pViewFile->getPanelWithFocus());

	// we start the loop on the first display row that is visible
	// given the current scrollbar settings.


	//////////////////////////////////////////////////////////////////
	// begin pseudo-clip context.  clipping seems broken (or at least
	// quirky on mac as of 2.9.5) as i prepare 4.2.  so i'm going to
	// reorder the drawing so that i don't need to use it.
	//////////////////////////////////////////////////////////////////
	// wxDCClipper dcClip(dc,xPixelText,0,xPixelsClientSize-xPixelText,yPixelsClientSize);

	for (int row=yRowThumb; row<yRowLimit; row++)
	{
		wxCoord y = (row-yRowThumb) * yPixelsPerRow;

		UTIL_PERF_START_CLOCK(sszKey_deRow);
		const de_row &  rDeRow  = (*pDis)[row];
		const de_line * pDeLine = rDeRow.getPanelLine(m_kPanel);
		UTIL_PERF_STOP_CLOCK(sszKey_deRow);

		if (pDeLine)
		{
			const fl_line * pFlLine = pDeLine->getFlLine();

			// get the overall line background color based upon the line-oriented sync
			// node's attributes.  (this lets us show that the entire line was omitted
			// or that it has a change on it somewhere.)
			//
			// also get the default foreground color for the text.  this will be used
			// unless we have intra-line info to override it.

			UTIL_PERF_START_CLOCK(sszKey_attr_clr);
			if (bMerge)
			{
				_attr_to_color3_bg(false,bIgnoreUnimportantChanges,m_kPanel,rDeRow.getSync()->getAttr(),colorTextBg);
				_attr_to_color3_fg(m_kPanel,rDeRow.getSync()->getAttr(),colorTextFg);
			}
			else
			{
				_attr_to_color2_bg(false,bIgnoreUnimportantChanges,rDeRow.getSync()->getAttr(),colorTextBg);
				_attr_to_color2_fg(rDeRow.getSync()->getAttr(),colorTextFg);
			}
			UTIL_PERF_STOP_CLOCK(sszKey_attr_clr);

			// since font metrics aren't reliable (in that GetCharWidth() and GetTextExtent()
			// don't always come up with the same answer, we DONOT use xPixelsPerCol, in placing
			// text -- we have to do it the hard/expensive way -- start at the beginning of the
			// line and draw and measure each piece of text.

			wxCoord x = xPixelText - xPixelThumb;
			int col = 0;
			int lenEOL = 0;

			const de_sync *      pDeSyncLine   = rDeRow.getSync();
			if (pDeSyncLine->haveIntraLineSyncInfo())
			{
				// we have intra-line highlight info -- use it (and the intra-line sync list)
				// to draw the line.

				long                 offsetInSync  = rDeRow.getOffsetInSync();
				const de_sync_list * pDeSyncListIL = pDeSyncLine->getIntraLineSyncList(offsetInSync);

				// walk this line's run-list and the intra-line-sync-list
				// in parallel so that we get both the fr_prop content
				// attributes *AND* the diff-engine attributes.

				const de_sync * pDeSyncIL = pDeSyncListIL->getHead();
				const fl_run *  pFlRun    = pFlLine->getFirstRunOnLine();
				int runOffset  = 0;
				int syncOffset = 0;
				while ( (pDeSyncIL && !pDeSyncIL->isEND())  &&  (pFlRun && (pFlRun->getLine()==pFlLine)) )
				{
					if (pDeSyncIL->getLen(m_kPanel) == 0)		// if on the void side of intra-line-sync
					{
						pDeSyncIL = pDeSyncIL->getNext();
						syncOffset = 0;
						continue;
					}
							
					wxASSERT_MSG( (runOffset < (int)pFlRun->getLength()), _T("Coding Error") );
					wxASSERT_MSG( (syncOffset < pDeSyncIL->getLen(m_kPanel)), _T("Coding Error") );

					int lenRun  = (int)pFlRun->getLength() - runOffset;
					int lenSync = pDeSyncIL->getLen(m_kPanel) - syncOffset;
					int len     = MyMin(lenRun,lenSync);

					wxASSERT_MSG( (len > 0), _T("Coding Error") );

					// draw inserted text in BOLD; everything else in NORMAL font.

					UTIL_PERF_START_CLOCK(sszKey_font);
					CONDITIONALLY_SET_FONT(pFlRun->getFragProp());
					UTIL_PERF_STOP_CLOCK(sszKey_font);

					// we have 2 possible background colors for this span of text:
					// [1] the overall color of the line (this is based upon the status
					// of the line as a whole)
					// [2] the intra-line-diff color for this span.  only non-equal spans
					// have an intra-line-bg color.  (equal spans within a change line
					// get the change color bg.)

					wxColour colorTextIlBg;
					bool bBgSignificant;

					UTIL_PERF_START_CLOCK(sszKey_attr_clr);
					if (bMerge)
					{
						_attr_to_color3_fg(m_kPanel,pDeSyncIL->getAttr(),colorTextFg);
						bBgSignificant = _attr_to_color3_bg(true,bIgnoreUnimportantChanges,m_kPanel,pDeSyncIL->getAttr(),colorTextIlBg);
					}
					else
					{
						_attr_to_color2_fg(pDeSyncIL->getAttr(),colorTextFg);
						bBgSignificant = _attr_to_color2_bg(true,bIgnoreUnimportantChanges,pDeSyncIL->getAttr(),colorTextIlBg);
					}
					if (!bBgSignificant)
						colorTextIlBg = colorTextBg;
					UTIL_PERF_STOP_CLOCK(sszKey_attr_clr);

					const wxChar * sz = pFlRun->getContent() + runOffset;
					_drawLongString(dc,
									sz,len,
									bDisplayInvisibles,cColTabWidth,
									colorTextFg,colorTextIlBg,
									x,y,col,
									row);

					// TODO 20131004 if x coord of string is to the right of the xPixelsClientSize
					// TODO          we should be able to skip the rest of the text on this line.

					runOffset += len;
					if (runOffset >= (int)pFlRun->getLength())
					{
						pFlRun    = pFlRun->getNext();
						runOffset = 0;
					}
						
					syncOffset += len;
					if (syncOffset >= pDeSyncIL->getLen(m_kPanel))
					{
						pDeSyncIL = pDeSyncIL->getNext();
						syncOffset = 0;
					}
				}

				// if we're drawing invisibles and the EOL chars are not in the
				// intra-line sync-list, then we need to explicitly draw them now.

				if (!bGlobalRespectEOL && bDisplayInvisibles)
				{
					// since the EOL chars were not included in the comparison, we don't
					// know if they match the EOL chars on the other panels.  so we don't
					// know if we should draw them as _ALL_EQ or what.  also, to blindly
					// color them based upon the overall line state (such as a change)
					// gives a false impression that the EOL chars were also something
					// that changed.  furthermore, to use a bright color, such as the
					// blue, green, or magenta that we're using for text and changes
					// calls too much attention to them.  what we really need is a dim
					// color (like our omitted text color) that indicates that they are
					// present, but down-plays their importance.

					while (pFlRun && pFlRun->getLine()==pFlLine)
					{
						wxASSERT_MSG( (pFlRun->isCR() || pFlRun->isLF()), _T("Coding Error") );

						// draw inserted text in BOLD; everything else in NORMAL font.

						UTIL_PERF_START_CLOCK(sszKey_font);
						CONDITIONALLY_SET_FONT(pFlRun->getFragProp());
						UTIL_PERF_STOP_CLOCK(sszKey_font);

						_drawString(dc,
									pFlRun->getContent(), (long)pFlRun->getLength(),
									bDisplayInvisibles,cColTabWidth,
									colorEolUnknownFg,colorTextBg,
									x,y,col,
									row);
						lenEOL += (int)pFlRun->getLength();

						pFlRun = pFlRun->getNext();
					}
				}
			}
			else
			{
				// no intra-line info -- use layout to draw the line.

				for (const fl_run * pFlRun=pFlLine->getFirstRunOnLine(); (pFlRun && (pFlRun->getLine()==pFlLine)); pFlRun=pFlRun->getNext())
				{
					const wxChar * sz = pFlRun->getContent();
					long len = (long)pFlRun->getLength();

					UTIL_PERF_START_CLOCK(sszKey_font);
					CONDITIONALLY_SET_FONT(pFlRun->getFragProp());
					UTIL_PERF_STOP_CLOCK(sszKey_font);

					if (pFlRun->isLF() || pFlRun->isCR())
					{
						wxASSERT_MSG( (len==1), _T("Coding Error") );
						if (bDisplayInvisibles)
						{
							_drawString(dc,
										sz,1,
										bDisplayInvisibles,cColTabWidth,
										((bGlobalRespectEOL) ? colorTextFg : colorEolUnknownFg),colorTextBg,
										x,y,col,
										row);
							lenEOL++;
						}
					}
					else if (pFlRun->isTAB())
					{
						wxASSERT_MSG( (len==1), _T("Coding Error") );
						_drawString(dc,
									sz,1,
									bDisplayInvisibles,cColTabWidth,
									colorTextFg,colorTextBg,
									x,y,col,
									row);
					}
					else
					{
						_drawLongString(dc,
										sz,len,
										bDisplayInvisibles,cColTabWidth,
										colorTextFg,colorTextBg,
										x,y,col,
										row);
					}
				}
			}

			// flood fill the rest of the line with our background color

			// TODO 20131004 Likewise, also can skip background fill if x is
			// TODO          already off the screen to the right.

			UTIL_PERF_START_CLOCK(sszKey_brush2eol);
			if (_withinSelection(row,col-lenEOL))
				brushEOL.SetColour(colorSelectionBg);
			else
				brushEOL.SetColour(colorTextBg);
			dc.SetPen(penNone);
			dc.SetBrush(brushEOL);
			dc.DrawRectangle(MyMax(x,xPixelText),y,xPixelsClientSize,yPixelsPerRow);
			UTIL_PERF_STOP_CLOCK(sszKey_brush2eol);

			if (/*!bDisplayInvisibles &&*//* !haveSelection() &&*/ (row == m_vfcCaret.getRow()) && (col == m_vfcCaret.getCol())
				&& bPanelWithFocus)
			{
				// if the caret is at the left edge of the EOL and we're not displaying invisibles,
				// we need to force the drawing of the caret (because the caret gets drawn during
				// _drawString()).
				// 
				// but we only draw caret in panel with focus

				_drawCaret(dc,x,y);
			}
		}		// end "if (pDeLine)..."
		else
		{
			// other non-text lines are now handled in the next loop.
		}
	}
	// erase the area to the left of the text that would have been
	// excluded by the dcClip if we could use it.
	dc.SetBrush(brushWindowBG);
	dc.DrawRectangle(0, 0, xPixelText, yPixelsClientSize);
	//////////////////////////////////////////////////////////////////
	// end pseudo-clip context
	//////////////////////////////////////////////////////////////////

	// draw special non-text rows that may paint on
	// both sides of the bar separating content and
	// the line number region.

	for (int row=yRowThumb; row<yRowLimit; row++)
	{
		wxCoord y = (row-yRowThumb) * yPixelsPerRow;

		const de_row &  rDeRow  = (*pDis)[row];
		const de_line * pDeLine = rDeRow.getPanelLine(m_kPanel);

		if (pDeLine)
		{
			if (m_bShowLineNumbers)
			{
				const fl_line * pFlLine = pDeLine->getFlLine();

				// TODO deal with per-line icons & etc

				// draw line number -- we right justify it just left of the bar.
				// this lets us deal with some broken font metrics (where left
				// justifying with leading blanks doesn't work just right).

				CONDITIONALLY_SET_NORMAL_FONT();

				int lineNr = pFlLine->getLineNr();
				wxChar bufLineNr[32];
				::wxSnprintf(bufLineNr,NrElements(bufLineNr),_T("%d"),lineNr+1);
				wxString strLineNr(bufLineNr);
				wxCoord wLineNr, hLineNr;
				dc.GetTextExtent(strLineNr,&wLineNr,&hLineNr);
				dc.SetTextForeground(colorLineNrFg);
				dc.SetTextBackground(colorLineNrBg);
				dc.DrawText(strLineNr, xPixelLineNrRight-wLineNr, y);
				if (pFlLine->getEditOpCounter() != 0)
				{
					dc.DrawText( _T("*"), xPixelIcon, y);
				}
			}

			// actual text of line handled in the clipping loop above.
		}
		else if (rDeRow.isMARK())
		{
			// draw (manual-alignment) MARK as a full-height line with a pair of dashed lines thru the middle

			wxPen penDot(colorLineNrFg,1,wxDOT_DASH);
			dc.SetPen(penDot);

			wxCoord yMid = y + (yPixelsPerRow/2);
			wxCoord x0 = 1;
			wxCoord x1 = xPixelsClientSize - 1;

			dc.DrawLine(x0,yMid-2,x1,yMid-2);
			dc.DrawLine(x0,yMid+1,x1,yMid+1);
		}
		else if (rDeRow.isEOF())
		{
			if (/*!haveSelection() && */ m_vfcCaret.isSet() && (m_vfcCaret.getRow()==row) && (m_vfcCaret.getCol()==0)
				&& bPanelWithFocus)
			{
				// if the EOD is visible and the caret is just past the bottom
				// (like when the user hits ctrl-end), we need to force draw the
				// caret just below the last line we draw.  our choice of color
				// is sort of arbitrary.
				// 
				// but we only draw caret in panel with focus.

				wxCoord x = xPixelText - xPixelThumb;
		
				_drawCaret(dc,x,y);
			}
		}
		else // a void 
		{
			dc.SetPen(penNone);
			dc.SetBrush(brushVoidHatch);
			dc.SetBackground(brushVoidBG);
			if (m_bShowLineNumbers)
				dc.DrawRectangle(0,y,xPixelBar-PIXELS_LEFT_MARGIN_GAP,yPixelsPerRow);
			dc.DrawRectangle(xPixelText,y,xPixelsClientSize-xPixelText,yPixelsPerRow);
			dc.SetBrush(brushWindowBG);
		}

		// we are drawing row-by-row using the display list -- not the line list
		// in the document.  this allows us to show diffs-only or diffs-with-context.
		// if the row is marked "gapped", then the diff-engine hid some content
		// immediately prior to this row.  draw a line above our text (in the
		// leading above the characters).

		if (rDeRow.haveGap())
		{
			dc.SetPen(penHRule);
			dc.DrawLine(0,y,xPixelsClientSize,y);
		}

	}

	if (m_bShowLineNumbers)
	{
		// draw vertical line between line numbers and document area
	
		wxPen penBar(colorLineNrFg, PIXELS_LEFT_MARGIN_BAR, wxSOLID);
	
		dc.SetPen(penBar);
		dc.DrawLine(xPixelBar,0,xPixelBar,yPixelsClientSize);
	}

	// right-mouse-highlighting -- highlight the context of a change.

	long yRowPatchClick, yRowPatchFirst, yRowPatchLast;
	if (m_pDeDe->getPatchHighlight(kSync,&yRowPatchClick,&yRowPatchFirst,&yRowPatchLast))
	{
		// I'm going to use the selection-bg color to
		// draw the new "current patch" highlighting.
		// Not sure I need to add yet another new color.

		wxPen penDot(colorSelectionBg,1,wxDOT);
		dc.SetPen(penDot);
		
		wxCoord y0 = (yRowPatchFirst-yRowThumb) * yPixelsPerRow;
		wxCoord y1 = (yRowPatchLast -yRowThumb) * yPixelsPerRow - 1;
		wxCoord x0 = 0;
		wxCoord x1 = xPixelsClientSize;

		dc.DrawLine(x0,y0, x1,y0);
		dc.DrawLine(x0,y1, x1,y1);

		wxBrush brushTest(colorSelectionBg);
		dc.SetBrush(brushTest);
		dc.SetPen(penNone);
		dc.DrawRectangle(                     0,y0, PIXELS_MARGIN_PATCH,y1-y0+1);
		dc.DrawRectangle(x1-PIXELS_MARGIN_PATCH,y0, PIXELS_MARGIN_PATCH,y1-y0+1);

	}
	
	// this may not be necessary, but it de-selects the pens/brushes that
	// we created from the DC so that it doesn't matter in which order the
	// destructors get run for all the stack objects.

	dc.SetPen(wxNullPen);
	dc.SetBrush(wxNullBrush);
}

//////////////////////////////////////////////////////////////////

int ViewFilePanel::_computeDigitsRequiredForLineNumber(void) const
{
	int ex, nr;

	for (ex=1, nr=m_pFlFl->getFormattedLineNrs(); (nr >= 10); nr /= 10)
		ex++;

	return ex;
}
