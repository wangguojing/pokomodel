#ifndef __CUIWINDOW_H__
#define __CUIWINDOW_H__

#include <TCFoundation/TCAlertSender.h>
#include <windows.h>
#include <CommCtrl.h>
//#include <windowsx.h>
#include <CUI/CUIDefines.h>
#include <TCFoundation/TCTypedPointerArray.h>
#include <TCFoundation/TCStlIncludes.h>
#include <TCFoundation/mystring.h>

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED                 0x031A
#endif

typedef TCTypedPointerArray<HWND> CUIHwndArray;

typedef std::map<UINT, UINT> UIntUIntMap;

#define CUI_MAX_CHILDREN 100
#define NUM_SYSTEM_COLORS 25

class CUIWindowResizer;

#ifdef TC_NO_UNICODE
#define LPNMTTDISPINFOUC LPNMTTDISPINFOA
#define LPNMTBGETINFOTIPUC LPNMTBGETINFOTIPA

#define TTN_GETDISPINFOUC TTN_GETDISPINFOA
#define TBN_GETDISPINFOUC TBN_GETDISPINFOA
#define TBN_GETINFOTIPUC TBN_GETINFOTIPA
#define TBN_GETBUTTONINFOUC TBN_GETBUTTONINFOA
#define TVN_ITEMEXPANDINGUC TVN_ITEMEXPANDINGA
#define TVN_SELCHANGEDUC TVN_SELCHANGEDA
#define TVITEMEXUC TVITEMEXA
#define TVINSERTSTRUCTUC TVINSERTSTRUCTA
#define TVM_INSERTITEMUC TVM_INSERTITEMA
#define MENUITEMINFOUC MENUITEMINFOA

#define MENUITEMINFOUC MENUITEMINFOA
#define OPENFILENAMEUC OPENFILENAMEA
#define OPENFILENAME_NT4UC OPENFILENAME_NT4A
#define WC_BUTTONUC WC_BUTTONA
#define WC_STATICUC WC_STATICA
#define WC_EDITUC WC_EDITA
#define WC_COMBOBOXUC WC_COMBOBOXA
#define TOOLTIPS_CLASSUC TOOLTIPS_CLASSA
#define TOOLINFOUC TOOLINFOA
#define NMTOOLBARUC NMTOOLBARA
#define MAKEINTRESOURCEUC MAKEINTRESOURCEA
#define CreateDialogParamUC CreateDialogParamA
#define DialogBoxParamUC DialogBoxParamA
#define GetWindowLongPtrUC GetWindowLongPtrA
#define SetWindowLongPtrUC SetWindowLongPtrA
#define GetMenuItemInfoUC GetMenuItemInfoA
#define SetMenuItemInfoUC SetMenuItemInfoA
#else // TC_NO_UNICODE
#define LPNMTTDISPINFOUC LPNMTTDISPINFOW
#define LPNMTBGETINFOTIPUC LPNMTBGETINFOTIPW

#define TTN_GETDISPINFOUC TTN_GETDISPINFOW
#define TBN_GETDISPINFOUC TBN_GETDISPINFOW
#define TBN_GETINFOTIPUC TBN_GETINFOTIPW
#define TBN_GETBUTTONINFOUC TBN_GETBUTTONINFOW
#define TVN_ITEMEXPANDINGUC TVN_ITEMEXPANDINGW
#define TVN_SELCHANGEDUC TVN_SELCHANGEDW
#define TVITEMEXUC TVITEMEXW
#define TVINSERTSTRUCTUC TVINSERTSTRUCTW
#define TVM_INSERTITEMUC TVM_INSERTITEMW
#define MENUITEMINFOUC MENUITEMINFOW

#define MENUITEMINFOUC MENUITEMINFOW
#define OPENFILENAMEUC OPENFILENAMEW
#define OPENFILENAME_NT4UC OPENFILENAME_NT4W
#define WC_BUTTONUC WC_BUTTONW
#define WC_STATICUC WC_STATICW
#define WC_EDITUC WC_EDITW
#define WC_COMBOBOXUC WC_COMBOBOXW
#define TOOLTIPS_CLASSUC TOOLTIPS_CLASSW
#define TOOLINFOUC TOOLINFOW
#define NMTOOLBARUC NMTOOLBARW
#define MAKEINTRESOURCEUC MAKEINTRESOURCEW
#define CreateDialogParamUC CreateDialogParamW
#define DialogBoxParamUC DialogBoxParamW
#define GetWindowLongPtrUC GetWindowLongPtrW
#define SetWindowLongPtrUC SetWindowLongPtrW
#define GetMenuItemInfoUC GetMenuItemInfoW
#define SetMenuItemInfoUC SetMenuItemInfoW
#endif // TC_NO_UNICODE

class CUIExport CUIWindow : public TCAlertSender
{
	public:
		CUIWindow(void);
		CUIWindow(CUCSTR windowTitle, HINSTANCE hInstance, int x,
			int y, int width, int height);
		CUIWindow(CUIWindow* parentWindow, int x, int y, int width,
			int height);
		CUIWindow(HWND hParentWindow, HINSTANCE, int x, int y,
			int width, int height);

		HWND getHWindow(void) { return hWindow; }
		int getX(void) { return x; }
		int getY(void) { return y; }
		int getWidth(void) { return width; }
		int getHeight(void) { return height; }
		HINSTANCE getHInstance(void) { return hInstance; }
		BOOL isInitialized(void) { return initialized; }
		CUCSTR getWindowTitle(void) { return windowTitle; }
		virtual void showWindow(int nCmdShow);
		virtual BOOL initWindow(void);
		virtual void resize(int newWidth, int newHeight);
		virtual void setRBGFillColor(BYTE r, BYTE g, BYTE b);
		virtual void setFillColor(DWORD newColor);
		virtual void setTitle(CUCSTR value);
		void setMinWidth(int value) { minWidth = value; }
		void setMinHeight(int value) { minHeight = value; }
		void setMaxWidth(int value) { maxWidth = value; }
		void setMaxHeight(int value) { maxHeight = value; }
		virtual SIZE getDecorationSize(void);
		virtual void runDialogModal(HWND hDlg, bool allowMessages = false);
		virtual bool flushModal(HWND hWnd, bool isDialog, int maxFlush = -1);
		virtual bool flushDialogModal(HWND hDlg);
		virtual void closeWindow(void);
		virtual void maximize(void);
		virtual void minimize(void);
		virtual void restore(void);
		virtual UINT_PTR setTimer(UINT timerID, UINT elapse);
		virtual BOOL killTimer(UINT timerID);
		HWND getHParentWindow(void) { return hParentWindow; }
		virtual HWND createDialog(UCSTR templateName,
			BOOL asChildWindow = TRUE, DLGPROC dialogProc = staticDialogProc,
			LPARAM lParam = 0);
		virtual HWND createDialog(int templateNumber,
			BOOL asChildWindow = TRUE, DLGPROC dialogProc = staticDialogProc,
			LPARAM lParam = 0);
		virtual void doDialogClose(HWND hDlg);
		virtual LRESULT windowProc(HWND hWnd, UINT message,
			WPARAM wParam, LPARAM lParam);
		virtual INT_PTR dialogProc(HWND hDlg, UINT message,
			WPARAM wParam, LPARAM lParam);
		virtual void setMenuItemsEnabled(HMENU hMenu, bool enabled);
		virtual void setMenuEnabled(HMENU hParentMenu, int itemID,
			bool enabled, BOOL byPosition = FALSE);
		virtual HBITMAP createDIBSection(HDC hBitmapDC, int bitmapWidth,
			int bitmapHeight, int hDPI, int vDPI, BYTE **bmBuffer);

		static void setMenuCheck(HMENU hParentMenu, UINT uItem, bool checked,
			bool radio = false);
		static void setMenuRadioCheck(HMENU hParentMenu, UINT uItem,
			bool checked);
		static bool getMenuCheck(HMENU hParentMenu, UINT uItem);
		static void setArrowCursor(void);
		static void setWaitCursor(void);
		static HMENU findSubMenu(HMENU hParentMenu, int subMenuIndex,
			int *index = NULL);
		static HINSTANCE getLanguageModule(void);
		static HWND createWindowExUC(DWORD dwExStyle, CUCSTR lpClassName,
			CUCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth,
			int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
			LPVOID lpParam);
		static BOOL setWindowTextUC(HWND hWnd, CUCSTR text);
		static LRESULT sendMessageUC(HWND hWnd, UINT uMsg, WPARAM wParam,
			LPARAM lParam);
		static LRESULT sendDlgItemMessageUC(HWND hDlg, int nIDDlgItem,
			UINT uMsg, WPARAM wParam, LPARAM lParam);
		static HWND createStatusWindowUC(LONG style, CUCSTR lpszText,
			HWND hwndParent, UINT wID);
		static int messageBoxUC(HWND hWnd, CUCSTR lpText, CUCSTR lpCaption,
			UINT uType);
		static BOOL insertMenuItemUC(HMENU hmenu, UINT item, BOOL fByPosition,
			MENUITEMINFOUC *lpmi);
		static BOOL getOpenFileNameUC(OPENFILENAMEUC *lpofn);
		static BOOL getTextExtentPoint32UC(HDC hdc, CUCSTR lpString,
			int cbString, LPSIZE lpSize);
		static BOOL screenToClient(HWND hWnd, RECT *rect);
		static BOOL clientToScreen(HWND hWnd, RECT *rect);
		static LRESULT CALLBACK staticWindowProc(HWND hWnd,
			UINT message, WPARAM wParam, LPARAM lParam);
		static INT_PTR CALLBACK staticDialogProc(HWND hDlg,
			UINT message, WPARAM wParam, LPARAM lParam);

		static void windowGetText(HWND hWnd, ucstring &text);
#ifndef TC_NO_UNICODE
		static void windowGetText(HWND hWnd, std::string &text);
#endif // TC_NO_UNICODE
		static bool checkGet(HWND hWnd);
		static void checkSet(HWND hWnd, bool value);

		static void comboAddString(HWND hWnd, CUCSTR string);
		static LRESULT comboSelectItem(HWND hWnd, int index);
		static int comboGetSelectedItem(HWND hWnd);
		static const std::string notificationName(UINT code);

#ifndef TC_NO_UNICODE
		static void addFileType(char *fileTypes, const char *description,
			const char *filter);
#endif // TC_NO_UNICODE
		static void addFileType(UCSTR fileTypes, CUCSTR description,
			CUCSTR filter);
		static int getOpenFilenameSize(bool uc);

	protected:
		~CUIWindow(void);
		virtual void dealloc(void);
		void init(void);
		virtual void registerWindowClass(void);
		virtual BOOL createWindow(void);
		virtual BOOL createMainWindow(void);
		virtual BOOL createSubWindow(void);
		virtual const char* windowClassName(void);
		virtual WNDCLASSEX getWindowClass(void);
		virtual void addChild(CUIWindow* childWindow);
		virtual void removeChild(CUIWindow* childWindow);
		virtual void updateSystemColors(void);
		void initSystemColors(void);
		void initUcMessages(void);
		virtual void deleteSystemColorPens(void);
		virtual void redrawChildren(BOOL recurse);
		virtual HBRUSH getBackgroundBrush(void);

		virtual LRESULT doSize(WPARAM sizeType, int newWidth,
			int newHeight);
		virtual LRESULT doMove(int newX, int newY);
		virtual LRESULT doEraseBackground(RECT* updateRect = NULL);
		virtual LRESULT doCreate(HWND hWnd,
			LPCREATESTRUCT createStruct);
		virtual LRESULT doClose(void);
		virtual LRESULT doDestroy(void);
		virtual LRESULT doNCDestroy(void);
		virtual void doPaint(void);
		virtual void doPostPaint(void);
		virtual void doSystemColorChange(void);
		virtual LRESULT doLButtonDown(WPARAM keyFlags, int xPos,
			int yPos);
		virtual LRESULT doLButtonUp(WPARAM keyFlags, int xPos,
			int yPos);
		virtual LRESULT doLButtonDoubleClick(WPARAM keyFlags,
			int xPos, int yPos);
		virtual LRESULT doRButtonDown(WPARAM keyFlags, int xPos,
			int yPos);
		virtual LRESULT doRButtonUp(WPARAM keyFlags, int xPos,
			int yPos);
		virtual LRESULT doRButtonDoubleClick(WPARAM keyFlags,
			int xPos, int yPos);
		virtual LRESULT doMButtonDown(WPARAM keyFlags, int xPos,
			int yPos);
		virtual LRESULT doMButtonUp(WPARAM keyFlags, int xPos,
			int yPos);
		virtual LRESULT doMButtonDoubleClick(WPARAM keyFlags,
			int xPos, int yPos);
		virtual LRESULT doMouseMove(WPARAM keyFlags, int xPos,
			int yPos);
		virtual LRESULT doMouseWheel(short keyFlags, short zDelta,
			int xPos, int yPos);
		virtual LRESULT doCaptureChanged(HWND hNewWnd);
		virtual LRESULT doKeyDown(int keyCode, LPARAM keyData);
		virtual LRESULT doKeyUp(int keyCode, LPARAM keyData);
		virtual LRESULT doDropFiles(HDROP hDrop);
		virtual LRESULT doChar(TCHAR characterCode, LPARAM keyData);
		virtual LRESULT doShowWindow(BOOL showFlag, LPARAM status);
		virtual LRESULT doActivateApp(BOOL activateFlag,
			DWORD threadId);
		virtual LRESULT doActivate(int activateFlag, BOOL minimized,
			HWND previousHWindow);
		virtual LRESULT doGetMinMaxInfo(HWND hWnd,
			LPMINMAXINFO minMaxInfo);
		virtual LRESULT doCommand(int itemId, int notifyCode,
			HWND controlHWnd);
		virtual LRESULT doTimer(UINT_PTR timerID);
		virtual LRESULT doHelp(LPHELPINFO helpInfo);
		virtual LRESULT doMenuSelect(UINT menuID, UINT flags, HMENU hMenu);
		virtual LRESULT doEnterMenuLoop(bool isTrackPopupMenu);
		virtual LRESULT doExitMenuLoop(bool isTrackPopupMenu);
		virtual LRESULT doInitMenuPopup(HMENU hPopupMenu, UINT uPos,
			BOOL fSystemMenu);
		virtual LRESULT doDrawItem(HWND hControlWnd,
			LPDRAWITEMSTRUCT drawItemStruct);
		virtual LRESULT doThemeChanged(void);
		virtual LRESULT doNotify(int controlId, LPNMHDR notification);
		virtual BOOL doDialogThemeChanged(void);
		virtual BOOL doDialogCtlColorStatic(HDC hdcStatic, HWND hwndStatic);
		virtual BOOL doDialogCtlColorBtn(HDC hdcStatic, HWND hwndStatic);
		virtual BOOL doDialogCommand(HWND hDlg, int controlId,
			int notifyCode, HWND controlHWnd);
		virtual BOOL doDialogVScroll(HWND hDlg, int scrollCode,
			int position, HWND hScrollBar);
		virtual BOOL doDialogHScroll(HWND hDlg, int scrollCode,
			int position, HWND hScrollBar);
		virtual BOOL doDialogInit(HWND hDlg, HWND hFocusWindow, LPARAM lParam);
		virtual BOOL doDialogNotify(HWND hDlg, int controlId,
			LPNMHDR notification);
		virtual BOOL doDialogSize(HWND hDlg, WPARAM sizeType,
			int newWidth, int newHeight);
		virtual BOOL doDialogGetMinMaxInfo(HWND hDlg,
			LPMINMAXINFO minMaxInfo);
		virtual BOOL doDialogChar(HWND hDlg, TCHAR characterCode,
			LPARAM keyData);
		virtual BOOL doDialogHelp(HWND hDlg, LPHELPINFO helpInfo);
		virtual void setupDialogSlider(HWND hDlg, int controlId, short min,
			short max, WORD frequency, int value);
		virtual bool copyToClipboard(const char *value);
		virtual void processModalMessage(MSG msg);
#ifndef TC_NO_UNICODE
		virtual bool copyToClipboard(const wchar_t *value);
#endif // TC_NO_UNICODE

		virtual void positionResizeGrip(HWND hSizeGrip, int parentWidth,
			int parentHeight);
		virtual void positionResizeGrip(HWND hSizeGrip);

		virtual void setAutosaveName(const char *value);
		virtual bool readAutosaveInfo(int &saveX, int &saveY, int &saveWidth,
			int &saveHeight, int &saveMaximized);
		virtual void writeAutosaveInfo(int saveX, int saveY, int saveWidth,
			int saveHeight, int saveMaximized);

		static void printMessageName(UINT message);
		static std::string getMessageName(UINT message);
		static BOOL CALLBACK disableNonModalWindow(HWND hWnd,
			LPARAM hModalDialog);
		static BOOL CALLBACK enableNonModalWindow(HWND hWnd,
			LPARAM hModalDialog);
		static void calcSystemSizes(void);
		static void populateAppVersion(void);

		UCSTR windowTitle;
		int x;
		int y;
		int width;
		int height;
		int minWidth;
		int minHeight;
		int maxWidth;
		int maxHeight;
		HINSTANCE hInstance;
		HWND hWindow;
		HMENU hWindowMenu;
		HDC hdc;
		CUIWindow* parentWindow;
		HWND hParentWindow;
		DWORD exWindowStyle;
		DWORD windowStyle;
		UINT windowClassStyle;
		BOOL initialized;
		CUIWindow* children[CUI_MAX_CHILDREN];
		int numChildren;
		BOOL created;
		DWORD fillColor;
		DWORD* systemColors;
		HPEN* systemColorPens;
		HBRUSH hBackgroundBrush;
		PAINTSTRUCT* paintStruct;
		char *autosaveName;

		static int systemMaxWidth;
		static int systemMaxHeight;
		static int systemMinTrackWidth;
		static int systemMinTrackHeight;
		static int systemMaxTrackWidth;
		static int systemMaxTrackHeight;
		static HCURSOR hArrowCursor;
		static HCURSOR hWaitCursor;
		static bool appVersionPopulated;
		static DWORD appVersionMS;
		static DWORD appVersionLS;
	private:
		static void populateLanguageModule(HINSTANCE hDefaultModule);
		static bool loadLanguageModule(LCID lcid, bool includeSub = true);

		static HINSTANCE hLanguageModule;	// Always use getLanguageModule()
		static UIntUIntMap ucMessages;
};

#endif
