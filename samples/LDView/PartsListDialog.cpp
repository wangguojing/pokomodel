#include "PartsListDialog.h"
#include <CUI/CUIWindow.h>
#include <LDLib/LDHtmlInventory.h>
#include <TCFoundation/mystring.h>
#include "Resource.h"

#if defined(_MSC_VER) && _MSC_VER >= 1400 && defined(_DEBUG)
#define new DEBUG_CLIENTBLOCK
#endif // _DEBUG

#ifndef ListView_SetCheckState
   #define ListView_SetCheckState(hwndLV, i, fCheck) \
      ListView_SetItemState(hwndLV, i, \
      INDEXTOSTATEIMAGEMASK((fCheck)+1), LVIS_STATEIMAGEMASK)
#endif

PartsListDialog::PartsListDialog(CUIWindow *parentWindow,
								 LDHtmlInventory *htmlInventory) :
	m_parentWindow(parentWindow),
	m_htmlInventory(htmlInventory),
	m_hDlg(NULL)
{
}

PartsListDialog::~PartsListDialog(void)
{
}

void PartsListDialog::dealloc(void)
{
	if (m_hDlg)
	{
		DestroyWindow(m_hDlg);
		m_hDlg = NULL;
	}
	TCObject::dealloc();
}

void PartsListDialog::createDialog(void)
{
	if (!m_hDlg)
	{
		registerWindowClass();
		m_hDlg = m_parentWindow->createDialog(IDD_PARTS_LIST, TRUE,
			staticDialogProc, (LPARAM)this);
	}
}

int PartsListDialog::runModal(void)
{
	createDialog();
	m_modalReturn = IDCANCEL;
	m_parentWindow->runDialogModal(m_hDlg);
	return m_modalReturn;
}

INT_PTR CALLBACK PartsListDialog::staticDialogProc(HWND hDlg, UINT message,
												   WPARAM wParam, LPARAM lParam)
{
	PartsListDialog* dialog;

	if (message == WM_INITDIALOG)
	{
		dialog = (PartsListDialog*)lParam;
		SetWindowLongPtr(hDlg, DWLP_USER, lParam);
	}
	else
	{
		dialog = (PartsListDialog*)GetWindowLongPtr(hDlg, DWLP_USER);
	}
	if (dialog)
	{
		return dialog->dialogProc(hDlg, message, wParam, lParam);
	}
	else
	{
		return FALSE;
	}
}

INT_PTR PartsListDialog::dialogProc(HWND hDlg, UINT message, WPARAM wParam,
									LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			return doInitDialog(hDlg, (HWND)wParam, lParam);
			break;
		case WM_CLOSE:
			m_parentWindow->doDialogClose(hDlg);
			break;
		case WM_COMMAND:
			return doCommand((int)(short)LOWORD(wParam),
				(int)(short)HIWORD(wParam), (HWND)lParam);
			break;
		case WM_NOTIFY:
			return doNotify((int)(short)LOWORD(wParam), (LPNMHDR)lParam);
			break;
/*
		case WM_VSCROLL:
			return doDialogVScroll(hDlg, (int)(short)LOWORD(wParam),
				(int)(short)HIWORD(wParam), (HWND)lParam);
			break;
		case WM_HSCROLL:
			return doDialogHScroll(hDlg, (int)(short)LOWORD(wParam),
				(int)(short)HIWORD(wParam), (HWND)lParam);
			break;
		case WM_SIZE:
			return doDialogSize(hDlg, wParam, (int)(short)LOWORD(lParam),
				(int)(short)HIWORD(lParam));
			break;
		case WM_GETMINMAXINFO:
			return doDialogGetMinMaxInfo(hDlg, (LPMINMAXINFO)lParam);
			break;
		case WM_CHAR:
			return doDialogChar(hDlg, (TCHAR)wParam, lParam);
			break;
		case WM_HELP:
			return doDialogHelp(hDlg, (LPHELPINFO)lParam);
			break;
		case WM_THEMECHANGED:
			return doDialogThemeChanged();
			break;
		case WM_CTLCOLORSTATIC:
			retValue = doDialogCtlColorStatic((HDC)wParam, (HWND)lParam);
			if (retValue)
			{
				return retValue;
			}
			break;
		case WM_CTLCOLORBTN:
			retValue = doDialogCtlColorBtn((HDC)wParam, (HWND)lParam);
			if (retValue)
			{
				return retValue;
			}
			break;
*/
		default:
			return FALSE;
			//return DefDlgProc(hDlg, message, wParam, lParam);
			break;
	}
	return FALSE;
}

void PartsListDialog::populateColumnList(void)
{
	int i;
	LVCOLUMN column;
	DWORD exStyle = ListView_GetExtendedListViewStyle(m_hColumnList);
	const LDPartListColumnVector &columnOrder =
		m_htmlInventory->getColumnOrder();
	int count = 0;
	RECT lvClientRect;
	int lvWidth;

	exStyle |= (LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyle(m_hColumnList, exStyle);
	memset(&column, 0, sizeof(LVCOLUMN));
	column.mask = LVCF_FMT | LVCF_WIDTH;
	column.fmt = LVCFMT_LEFT;
	column.cx = 300;
	ListView_InsertColumn(m_hColumnList, 0, &column);
	for (i = 0; i < (int)columnOrder.size(); i++)
	{
		LVITEM item;
		LDPartListColumn column = columnOrder[i];

		memset(&item, 0, sizeof(item));
		item.mask = TVIF_TEXT | TVIF_PARAM;
		item.pszText = copyString(LDHtmlInventory::getColumnName(column));
		item.lParam = column;
		item.iItem = count;
		item.iSubItem = 0;
		ListView_InsertItem(m_hColumnList, &item);
		ListView_SetCheckState(m_hColumnList, item.iItem, 1);
		delete item.pszText;
		count++;
	}
	for (i = LDPLCFirst; i <= LDPLCLast; i++)
	{
		LDPartListColumn column = (LDPartListColumn)i;
		if (!m_htmlInventory->isColumnEnabled(column))
		{
			LVITEM item;

			memset(&item, 0, sizeof(item));
			item.mask = TVIF_TEXT | TVIF_PARAM;
			item.pszText = copyString(LDHtmlInventory::getColumnName(column));
			item.lParam = i;
			item.iItem = count;
			item.iSubItem = 0;
			ListView_InsertItem(m_hColumnList, &item);
			ListView_SetCheckState(m_hColumnList, item.iItem, 0);
			delete item.pszText;
			count++;
		}
	}
	ListView_SetColumnWidth(m_hColumnList, 0, LVSCW_AUTOSIZE);
	GetClientRect(m_hColumnList, &lvClientRect);
	lvWidth = lvClientRect.right - lvClientRect.left;
	if (ListView_GetColumnWidth(m_hColumnList, 0) < lvWidth)
	{
		ListView_SetColumnWidth(m_hColumnList, 0, lvWidth);
	}
}

void PartsListDialog::setupToolbar(void)
{
	RECT tbRect;
	TBADDBITMAP addBitmap;
	TBBUTTON buttons[2];
	char buttonTitle[128];
	int i;

	memset(buttonTitle, 0, sizeof(buttonTitle));
	GetClientRect(m_hToolbar, &tbRect);
	SendMessage(m_hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 
	addBitmap.hInst = CUIWindow::getLanguageModule();
	addBitmap.nID = IDB_EXTRA_DIRS;
	SendMessage(m_hToolbar, TB_SETINDENT, tbRect.right - tbRect.left - 50, 0);
	SendMessage(m_hToolbar, TB_SETBUTTONWIDTH, 0, MAKELONG(25, 25));
	SendMessage(m_hToolbar, TB_ADDBITMAP, 4, (LPARAM)&addBitmap);
	SendMessage(m_hToolbar, TB_ADDSTRING, 0, (LPARAM)buttonTitle);
	for (i = 0; i < 2; i++)
	{
		buttons[i].iBitmap = i + 2;
		buttons[i].idCommand = 42 + i;
		buttons[i].fsState = 0;//TBSTATE_ENABLED;
		buttons[i].fsStyle = TBSTYLE_BUTTON;
		buttons[i].dwData = (DWORD)this;
		buttons[i].iString = -1;
	}
	SendMessage(m_hToolbar, TB_ADDBUTTONS, 2, (LPARAM)buttons);
}

void PartsListDialog::updateOverwriteCheck(void)
{
	if (getCheck(IDC_SHOW_MODEL))
	{
		EnableWindow(m_hOverwrite, TRUE);
		setCheck(IDC_OVERWRITE, m_overwriteSnapshot);
	}
	else
	{
		EnableWindow(m_hOverwrite, FALSE);
		setCheck(IDC_OVERWRITE, false);
	}
}

INT_PTR PartsListDialog::doInitDialog(HWND hDlg, HWND /*hFocus*/,
									  LPARAM /*lParam*/)
{
	m_hDlg = hDlg;

	m_hOverwrite = GetDlgItem(m_hDlg, IDC_OVERWRITE);
	setCheck(IDC_SHOW_MODEL, m_htmlInventory->getShowModelFlag());
	m_overwriteSnapshot = m_htmlInventory->getOverwriteSnapshotFlag();
	updateOverwriteCheck();
	setCheck(IDC_EXTERNAL_CSS, m_htmlInventory->getExternalCssFlag());
	setCheck(IDC_SHOW_PART_IMAGES, m_htmlInventory->getPartImagesFlag());
	setCheck(IDC_SHOW_FILE, m_htmlInventory->getShowFileFlag());
	m_hColumnList = GetDlgItem(m_hDlg, IDC_COLUMN_ORDER_LIST);
	m_hToolbar = GetDlgItem(m_hDlg, IDC_TOOLBAR);
	populateColumnList();
	setupToolbar();
	return TRUE;
}

void PartsListDialog::setCheck(int controlId, bool value)
{
	SendDlgItemMessage(m_hDlg, controlId, BM_SETCHECK, value, 0);
}

bool PartsListDialog::getCheck(int controlId)
{
	return SendDlgItemMessage(m_hDlg, controlId, BM_GETCHECK, 0, 0) != 0;
}

void PartsListDialog::saveSettings(void)
{
	int i;
	int count = ListView_GetItemCount(m_hColumnList);
	LDPartListColumnVector columnOrder;

	if (getCheck(IDC_SHOW_MODEL))
	{
		m_htmlInventory->setShowModelFlag(true);
		m_overwriteSnapshot = getCheck(IDC_OVERWRITE);
		m_htmlInventory->setOverwriteSnapshotFlag(m_overwriteSnapshot);
	}
	else
	{
		m_htmlInventory->setShowModelFlag(false);
	}
	m_htmlInventory->setExternalCssFlag(getCheck(IDC_EXTERNAL_CSS));
	m_htmlInventory->setPartImagesFlag(getCheck(IDC_SHOW_PART_IMAGES));
	m_htmlInventory->setShowFileFlag(getCheck(IDC_SHOW_FILE));
	for (i = 0; i < count; i++)
	{
		if (ListView_GetCheckState(m_hColumnList, i))
		{
			LVITEM item;

			item.mask = TVIF_PARAM;
			item.iItem = i;
			item.iSubItem = 0;
			if (ListView_GetItem(m_hColumnList, &item))
			{
				columnOrder.push_back((LDPartListColumn)item.lParam);
			}
		}
	}
	m_htmlInventory->setColumnOrder(columnOrder);
}

INT_PTR PartsListDialog::doMoveColumn(int distance)
{
	int selectedIndex = ListView_GetNextItem(m_hColumnList, -1, LVNI_SELECTED);
	int count = ListView_GetItemCount(m_hColumnList);

	if (selectedIndex + distance < 0 ||
		selectedIndex + distance >= count)
	{
		// This shouldn't happen, but just in case...
		return 1;
	}
	else
	{
		LVITEM otherItem;
		LVITEM selectedItem;
		char otherItemText[1024];
		char selectedItemText[1024];
		BOOL otherCheckState;
		BOOL selectedCheckState;

		otherItem.mask = selectedItem.mask = LVIF_TEXT | LVIF_PARAM;
		otherItem.iItem = selectedIndex + distance;
		selectedItem.iItem = selectedIndex;
		otherItem.iSubItem = selectedItem.iSubItem = 0;
		otherItem.pszText = otherItemText;
		selectedItem.pszText = selectedItemText;
		otherItem.cchTextMax = selectedItem.cchTextMax =
			sizeof(otherItemText);
		ListView_GetItem(m_hColumnList, &otherItem);
		ListView_GetItem(m_hColumnList, &selectedItem);
		otherItem.iItem = selectedIndex;
		selectedItem.iItem = selectedIndex + distance;
		ListView_SetItem(m_hColumnList, &otherItem);
		ListView_SetItem(m_hColumnList, &selectedItem);
		otherCheckState = ListView_GetCheckState(m_hColumnList,
			selectedIndex + distance);
		selectedCheckState = ListView_GetCheckState(m_hColumnList,
			selectedIndex);
		if (otherCheckState != selectedCheckState)
		{
			// Swapping the states doesn't work right above; we apparently have
			// to use the set check macro.
			ListView_SetCheckState(m_hColumnList, selectedIndex + distance,
				selectedCheckState);
			ListView_SetCheckState(m_hColumnList, selectedIndex,
				otherCheckState);
		}
		// Select other item.  (Single select LV, so no need to deselect
		// the current item.
		ListView_SetItemState(m_hColumnList, selectedIndex + distance,
			LVIS_SELECTED, LVIS_SELECTED);
		return 0;
	}
}

INT_PTR PartsListDialog::doClick(int controlId, HWND /*hControl*/)
{
	switch (controlId)
	{
	case IDOK:
		m_modalReturn = IDOK;
		saveSettings();
		m_parentWindow->doDialogClose(m_hDlg);
		break;
	case IDCANCEL:
		m_parentWindow->doDialogClose(m_hDlg);
		break;
	case IDC_SHOW_MODEL:
		updateOverwriteCheck();
		break;
	case 42:	// Column order toolbar up button
		return doMoveColumn(-1);
		break;
	case 43:	// Column order toolbar down button
		return doMoveColumn(1);
		break;
	default:
		return 1;
		break;
	}
	return 0;
}

INT_PTR PartsListDialog::doCommand(int controlId, int notifyCode, HWND hControl)
{
	if (notifyCode == BN_CLICKED)
	{
		return doClick(controlId, hControl);
	}
	return FALSE;
}

void PartsListDialog::doListViewItemChanged(int controlId, LPNMLISTVIEW /*pnmlv*/)
{
	if (controlId == IDC_COLUMN_ORDER_LIST)
	{
		int count = ListView_GetItemCount(m_hColumnList);

		if (count > 1)
		{
			int selectedIndex = ListView_GetNextItem(m_hColumnList, -1,
				LVNI_SELECTED);

			if (selectedIndex > 0)
			{
				// Enable up button
				SendMessage(m_hToolbar, TB_SETSTATE, 42,
					MAKELONG(TBSTATE_ENABLED, 0));
			}
			else
			{
				// Disable up button
				SendMessage(m_hToolbar, TB_SETSTATE, 42, MAKELONG(0, 0));
			}
			if (selectedIndex < count - 1 && selectedIndex >= 0)
			{
				// Enable down button
				SendMessage(m_hToolbar, TB_SETSTATE, 43,
					MAKELONG(TBSTATE_ENABLED, 0));
			}
			else
			{
				// Disable up button
				SendMessage(m_hToolbar, TB_SETSTATE, 43, MAKELONG(0, 0));
			}
		}
	}
}

INT_PTR PartsListDialog::doNotify(int controlId, LPNMHDR pnmh)
{
	switch (pnmh->code)
	{
	case LVN_ITEMCHANGED:
		doListViewItemChanged(controlId, (LPNMLISTVIEW)pnmh);
		break;
	}
	return FALSE;
}

void PartsListDialog::registerWindowClass(void)
{
	WNDCLASSEX windowClass;
	windowClass.cbSize = sizeof(windowClass);

	if (!GetClassInfoEx(CUIWindow::getLanguageModule(), "LDViewPartsListWindow",
		&windowClass))
	{
		char progressClassName[1024];
		// The load progress dialog doesn't have a custom window class.  The
		// default window class for dialogs had a window proc that does a bunch
		// of work that we need, so create a dummy window and grab the
		// information out of the dummy window's window class.  Then, replace
		// the bits that we want to replace.
		HWND hProgressWindow = m_parentWindow->createDialog(IDD_LOAD_PROGRESS);

		GetClassName(hProgressWindow, progressClassName, 1024);
		windowClass.cbSize = sizeof(windowClass);
		GetClassInfoEx(CUIWindow::getLanguageModule(), progressClassName,
			&windowClass);
		windowClass.hIcon = LoadIcon(CUIWindow::getLanguageModule(),
			MAKEINTRESOURCE(IDI_APP_ICON));
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = "LDViewPartsListWindow";
		RegisterClassEx(&windowClass);
		DestroyWindow(hProgressWindow);
	}
}
