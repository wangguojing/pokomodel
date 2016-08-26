#include "ModelWindow.h"
#include <LDLib/LDrawModelViewer.h>
#include <TCFoundation/TCMacros.h>
#include "LDVExtensionsSetup.h"
#include "LDViewWindow.h"
#include "JpegOptionsDialog.h"
#include "ExportOptionsDialog.h"
#include <TCFoundation/TCAutoreleasePool.h>
#include <TCFoundation/TCUserDefaults.h>
#include <TCFoundation/TCStringArray.h>
#include <TCFoundation/mystring.h>
#include <TCFoundation/TCImage.h>
#include <TCFoundation/TCAlertManager.h>
#include <TCFoundation/TCProgressAlert.h>
#include <TCFoundation/TCLocalStrings.h>
#include <LDLoader/LDLError.h>
#include <LDLoader/LDLModel.h>
#include <LDLib/LDLibraryUpdater.h>
#include <LDLib/LDHtmlInventory.h>
#include "Resource.h"
#include <Commctrl.h>
#include <LDLib/LDUserDefaultsKeys.h>
#include <CUI/CUIWindowResizer.h>
#include <TRE/TREMainModel.h>
#include <TRE/TREGLExtensions.h>
#include <windowsx.h>

#ifndef _NO_BOOST
#include <boost/bind.hpp>
#endif // !_NO_BOOST

#if defined(_MSC_VER) && _MSC_VER >= 1400 && defined(_DEBUG)
#define new DEBUG_CLIENTBLOCK
#endif // _DEBUG

#define DISTANCE_MULT 1.325f

#define POLL_INTERVAL 500
#define POLL_TIMER 1

#define FSAA_UPDATE_TIMER 2

#define PNG_IMAGE_TYPE_INDEX 1
#define BMP_IMAGE_TYPE_INDEX 2
#define JPG_IMAGE_TYPE_INDEX 3
#define SVG_IMAGE_TYPE_INDEX 4
#define EPS_IMAGE_TYPE_INDEX 5
#define PDF_IMAGE_TYPE_INDEX 6

#define MAX_SNAPSHOT_WIDTH 10000
#define MAX_SNAPSHOT_HEIGHT 10000

#ifndef ListView_SetCheckState
   #define ListView_SetCheckState(hwndLV, i, fCheck) \
      ListView_SetItemState(hwndLV, i, \
      INDEXTOSTATEIMAGEMASK((fCheck)+1), LVIS_STATEIMAGEMASK)
#endif

ControlInfo::ControlInfo(void)
{
#ifdef _LEAK_DEBUG
	strcpy(className, "ControlInfo");
#endif
}

ErrorInfo::ErrorInfo(void)
	:m_typeName(NULL)
{
#ifdef _LEAK_DEBUG
	strcpy(className, "ErrorInfo");
#endif
}

ErrorInfo::~ErrorInfo(void)
{
}

void ErrorInfo::dealloc(void)
{
	delete m_typeName;
	TCObject::dealloc();
}

void ErrorInfo::setTypeName(const char *typeName)
{
	if (typeName != m_typeName)
	{
		delete m_typeName;
		m_typeName = copyString(typeName);
	}
}

ModelWindow::ModelWindow(
	CUIWindow* parentWindow,
	int x,
	int y,
	int width,
	int height):
CUIOGLWindow(parentWindow, x, y, width, height),
modelViewer(new LDrawModelViewer(width, height)),
snapshotTaker(NULL),
numFramesSinceReference(0),
firstFPSPass(true),
//rotationSpeed(0.0f),
//lButtonDown(false),
//rButtonDown(false),
//mButtonDown(false),
hPrefsWindow(NULL),
captureCount(0),
redrawCount(0),
hProgressWindow(NULL),
lastProgressUpdate(0),
loading(false),
//needsRecompile(false),
hErrorWindow(NULL),
errors(new LDLErrorArray),
errorTreePopulated(false),
errorInfos(NULL),
prefs(new LDViewPreferences(parentWindow->getHInstance(), modelViewer)),
applyingPrefs(false),
offscreenActive(false),
hPBuffer(NULL),
hPBufferDC(NULL),
hPBufferGLRC(NULL),
hBitmapRenderDC(NULL),
hBitmapRenderGLRC(NULL),
hRenderBitmap(NULL),
hPrintDialog(NULL),
hStatusBar(NULL),
hProgressBar(NULL),
windowShown(false),
hCurrentDC(NULL),
hCurrentGLRC(NULL),
errorWindowResizer(NULL),
saveWindowResizer(NULL),
savingFromCommandLine(false),
skipErrorUpdates(false),
releasingMouse(false),
saveStepSuffix(NULL),
userLoad(false),
errorCount(0),
warningCount(0)
#ifndef _NO_BOOST
,remoteListener(true)
,remoteMessageID(0)
#endif // !_NO_BOOST
{
	char *programPath = LDViewPreferences::getLDViewPath();
	HRSRC hStudLogoResource = FindResource(NULL,
		MAKEINTRESOURCE(IDR_STUDLOGO_PNG), RT_RCDATA);
	HRSRC hFontResource = FindResource(NULL, MAKEINTRESOURCE(IDR_SANS_FONT),
		RT_RCDATA);

	loadSettings();
	if (hStudLogoResource)
	{
		HGLOBAL hStudLogo = LoadResource(NULL, hStudLogoResource);

		if (hStudLogo)
		{
			TCByte *data = (TCByte *)LockResource(hStudLogo);

			if (data)
			{
				DWORD length = SizeofResource(NULL, hStudLogoResource);

				if (length)
				{
					TREMainModel::setStudTextureData(data, length);
				}
			}
		}
	}
	if (hFontResource)
	{
		HGLOBAL hFont = LoadResource(NULL, hFontResource);

		if (hFont)
		{
			TCByte *data = (TCByte *)LockResource(hFont);

			if (data)
			{
				DWORD length = SizeofResource(NULL, hFontResource);

				if (length)
				{
					modelViewer->setFontData(data, length);
				}
			}
		}
	}
	windowStyle = windowStyle & ~WS_VISIBLE;
	inputHandler = modelViewer->getInputHandler();
	TCAlertManager::registerHandler(LDLError::alertClass(), this,
		(TCAlertCallback)&ModelWindow::ldlErrorCallback);
	TCAlertManager::registerHandler(TCProgressAlert::alertClass(), this,
		(TCAlertCallback)&ModelWindow::progressAlertCallback);
	TCAlertManager::registerHandler(LDrawModelViewer::alertClass(), this,
		(TCAlertCallback)&ModelWindow::modelViewerAlertCallback);
	TCAlertManager::registerHandler(LDrawModelViewer::redrawAlertClass(), this,
		(TCAlertCallback)&ModelWindow::redrawAlertCallback);
	TCAlertManager::registerHandler(LDrawModelViewer::loadAlertClass(), this,
		(TCAlertCallback)&ModelWindow::loadAlertCallback);
	TCAlertManager::registerHandler(LDInputHandler::captureAlertClass(), this,
		(TCAlertCallback)&ModelWindow::captureAlertCallback);
	TCAlertManager::registerHandler(LDInputHandler::releaseAlertClass(), this,
		(TCAlertCallback)&ModelWindow::releaseAlertCallback);
	TCAlertManager::registerHandler(LDSnapshotTaker::alertClass(), this,
		(TCAlertCallback)&ModelWindow::snapshotTakerAlertCallback);
	if (programPath)
	{
		modelViewer->setProgramPath(programPath);
		TCUserDefaults::setStringForKey(programPath, INSTALL_PATH_KEY, false);
		TCUserDefaults::setStringForKey(programPath, INSTALL_PATH_4_1_KEY, false);
		delete programPath;
	}
#ifndef _NO_BOOST
	if (remoteListener)
	{
		launchRemoteListener();
	}
#endif // !_NO_BOOST
}

ModelWindow::~ModelWindow(void)
{
}

void ModelWindow::dealloc(void)
{
#ifndef _NO_BOOST
	if (remoteListener)
	{
		shutDownRemoteListener();
	}
#endif // !_NO_BOOST
	TCAlertManager::unregisterHandler(this);
	TCObject::release(errorInfos);
	TCObject::release(snapshotTaker);
	errorInfos = NULL;
	if (prefs)
	{
		prefs->release();
		prefs = NULL;
	}
	if (errors)
	{
		errors->release();
		errors = NULL;
	}
	if (modelViewer)
	{
		modelViewer->release();
		modelViewer = NULL;
	}
	if (errorWindowResizer)
	{
		errorWindowResizer->release();
	}
	delete saveStepSuffix;
	stopPolling();
	CUIOGLWindow::dealloc();
}

#ifndef _NO_BOOST

#define PIPE_BUFSIZE 4096
#define PIPE_FILENAME "\\\\.\\pipe\\LDViewRemoteControl"

void ModelWindow::shutDownRemoteListener(void)
{
	boost::mutex::scoped_lock lock(mutex);
	exiting = true;
	lock.unlock();
	// Connect to the pipe to pull the ConnectNamedPipe
	HANDLE hPipe = CreateFile(
		PIPE_FILENAME,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hPipe);
	}
	if (!listenerThread->timed_join(boost::posix_time::seconds(1)))
	{
		// If it hasn't shut down after 1 second, abandon it.
		listenerThread->detach();
	}
	delete listenerThread;
}

void ModelWindow::launchRemoteListener(void)
{
	remoteCommandMap["highlight_line"] = RCHighlightLine;
	remoteCommandMap["highlight_lines"] = RCHighlightLine;
	remoteCommandMap["get_version"] = RCGetVersion;
	ldviewVersion = ((LDViewWindow *)parentWindow)->getProductVersion();
	exiting = false;
	remoteMessageID = RegisterWindowMessage("LDViewRemoteControl");
	try
	{
		listenerThread = new boost::thread(
			boost::bind(&ModelWindow::listenerProc, this));
	}
	catch (...)
	{
		debugPrintf("error spawning listener thread");
	}
}

void ModelWindow::listenerProc(void)
{
	while (true)
	{
		boost::mutex::scoped_lock lock(mutex);
		if (exiting)
		{
			return;
		}
		lock.unlock();
		HANDLE hPipe = CreateNamedPipe(
			PIPE_FILENAME,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			PIPE_BUFSIZE,
			PIPE_BUFSIZE,
			NMPWAIT_USE_DEFAULT_WAIT,
			NULL);
		if (hPipe == INVALID_HANDLE_VALUE)
		{
			debugPrintf("Error creating listener pipe.\n");
			return;
		}
		if (!ConnectNamedPipe(hPipe, NULL))
		{
			if (GetLastError() != ERROR_PIPE_CONNECTED)
			{
				debugPrintf("Error connecting listener pipe to client.\n");
				CloseHandle(hPipe);
				return;
			}
		}
		lock.lock();
		if (exiting)
		{
			CloseHandle(hPipe);
			return;
		}
		boost::thread remoteThread(boost::bind(&ModelWindow::remoteProc, this,
			hPipe));
		remoteThread.detach();
	}
}

void ModelWindow::sendResponseMessage(HANDLE hPipe, const char *message)
{
	DWORD numWritten;
	DWORD messageLength = (DWORD)strlen(message);

	// Note null terminator on message is optional.
	if (WriteFile(hPipe, &messageLength, sizeof(messageLength), &numWritten,
		NULL) && numWritten == sizeof(messageLength))
	{
		WriteFile(hPipe, message, messageLength, &numWritten, NULL);
	}
}

void ModelWindow::sendVersionResponse(HANDLE hPipe)
{
	sendResponseMessage(hPipe, ldviewVersion.c_str());
}

void ModelWindow::remoteProc(HANDLE hPipe)
{
	while (true)
	{
		DWORD messageSize;
		DWORD readSize;
		char *message;

		if (ReadFile(hPipe, &messageSize, sizeof(messageSize), &readSize, NULL))
		{
			if (readSize != sizeof(messageSize))
			{
				debugPrintf("Remote message size failure.\n");
				break;
			}
		}
		else
		{
			break;
		}
		if (messageSize > 1000000)
		{
			// Don't even try to handle unreasonable messages.
			break;
		}
		message = new char[messageSize + 1];
		if (ReadFile(hPipe, message, messageSize, &readSize, NULL))
		{
			std::string command;
			std::string data;

			message[messageSize] = 0;
			switch (parseRemoteMessage(message, command, data))
			{
			case RCGetVersion:
				sendVersionResponse(hPipe);
				delete message;
				break;
			default:
				// Note: message is deleted by UI thread.
				PostMessage(hWindow, remoteMessageID, 0, (LPARAM)message);
				break;
			}
		}
		else
		{
			delete message;
			break;
		}
	}
	CloseHandle(hPipe);
}

void ModelWindow::highlightLines(const std::string &paths)
{
	modelViewer->setHighlightPaths(paths);
}

ModelWindow::RemoteCommands ModelWindow::parseRemoteMessage(
	const char *message,
	std::string &command,
	std::string &data)
{
	const char *colonSpot = strchr(message, ':');
	if (colonSpot != NULL)
	{
		command = message;
		command.resize(colonSpot - message);
		convertStringToLower(&command[0]);
		data = &colonSpot[1];
		return remoteCommandMap[command];
	}
	return RCUnknown;
}

void ModelWindow::processRemoteMessage(char *message)
{
	std::string command;
	std::string data;

	switch (parseRemoteMessage(message, command, data))
	{
	case RCHighlightLine:
		highlightLines(data);
		break;
	default:
		printf("Unrecognized remote command: %s(%s)\n", command.c_str(),
			data.c_str());
		break;
	}
	delete message;
}

#endif // !_NO_BOOST

void ModelWindow::ldlErrorCallback(LDLError *error)
{
	if (error && userLoad)
	{
		if (!errorCallback(error))
		{
			error->cancelLoad();
		}
	}
}

void ModelWindow::loadAlertCallback(TCAlert *alert)
{
	if (strcmp(alert->getMessage(), "ModelLoadCanceled") == 0)
	{
		parentWindow->setTitle(_UC("LDView"));
		stopPolling();
	}
	TCAlertManager::sendAlert(alertClass(), this, alert->getMessageUC());
}

void ModelWindow::redrawAlertCallback(TCAlert *alert)
{
	if (alert->getSender() == modelViewer)
	{
		redrawRequested = true;
		forceRedraw();
	}
}

void ModelWindow::captureAlertCallback(TCAlert *alert)
{
	if (alert->getSender() == inputHandler)
	{
		captureMouse();
	}
}

void ModelWindow::releaseAlertCallback(TCAlert *alert)
{
	if (alert->getSender() == inputHandler)
	{
		releaseMouse();
	}
}

void ModelWindow::snapshotTakerAlertCallback(TCAlert *alert)
{
	if (alert->getSender() == snapshotTaker)
	{
		if (strcmp(alert->getMessage(), "MakeCurrent") == 0)
		{
			makeCurrent();
		}
	}
}

void ModelWindow::modelViewerAlertCallback(TCAlert *alert)
{
	if (alert)
	{
		stopAnimation();
		MessageBox(hWindow, alert->getMessage(), "LDView",
			MB_OK | MB_ICONWARNING);
	}
}

void ModelWindow::progressAlertCallback(TCProgressAlert *alert)
{
	if (alert)
	{
#ifndef _NO_BOOST
		if (strcmp(alert->getSource(), LD_LIBRARY_UPDATER) != 0)
#endif //_NO_BOOST
		{
			bool showErrors = true;

			if (strcmp(alert->getSource(), "LDSnapshotTaker") == 0)
			{
				showErrors = false;
			}
			if (!progressCallback(alert->getMessageUC(), alert->getProgress(),
				strcmp(alert->getSource(), "TCImageFormat") == 0, showErrors))
			{
				alert->abort();
			}
//			printf("Progress message from %s:\n%s (%f)\n", alert->getSource(),
//				alert->getMessage(), alert->getProgress());
		}
	}
}

void ModelWindow::loadSettings(void)
{
	pollSetting = TCUserDefaults::longForKey(POLL_KEY, 0, false);
	viewMode = (LDInputHandler::ViewMode)TCUserDefaults::longForKey(
		VIEW_MODE_KEY, 0, false);
	loadPrintSettings();
	loadSaveSettings();
}

void ModelWindow::loadPrintSettings(void)
{
	printLeftMargin = TCUserDefaults::longForKey(LEFT_MARGIN_KEY, 500, false) /
		1000.0f;
	printRightMargin = TCUserDefaults::longForKey(RIGHT_MARGIN_KEY, 500,
		false) / 1000.0f;
	printTopMargin = TCUserDefaults::longForKey(TOP_MARGIN_KEY, 500, false) /
		1000.0f;
	printBottomMargin = TCUserDefaults::longForKey(BOTTOM_MARGIN_KEY, 500,
		false) / 1000.0f;
	printOrientation = TCUserDefaults::longForKey(ORIENTATION_KEY,
		DMORIENT_PORTRAIT, false);
	printPaperSize = TCUserDefaults::longForKey(PAPER_SIZE_KEY, DMPAPER_LETTER,
		false);
	usePrinterDPI = TCUserDefaults::longForKey(USE_PRINTER_DPI_KEY, 1) != 0;
	printDPI = TCUserDefaults::longForKey(PRINT_DPI_KEY, 300);
	printBackground = TCUserDefaults::longForKey(PRINT_BACKGROUND_KEY, 0, false)
		!= 0;
}

void ModelWindow::loadSaveSettings(void)
{
	saveActualSize = TCUserDefaults::longForKey(SAVE_ACTUAL_SIZE_KEY, 1, false)
		!= 0;
	saveWidth = TCUserDefaults::longForKey(SAVE_WIDTH_KEY, 1024, false);
	saveHeight = TCUserDefaults::longForKey(SAVE_HEIGHT_KEY, 768, false);
	saveZoomToFit = TCUserDefaults::longForKey(SAVE_ZOOM_TO_FIT_KEY, 1, false)
		!= 0;
	saveSeries = TCUserDefaults::longForKey(SAVE_SERIES_KEY, 1, false) != 0;
	saveDigits = TCUserDefaults::longForKey(SAVE_DIGITS_KEY, 1, false);
	ignorePBuffer = TCUserDefaults::longForKey(IGNORE_PBUFFER_KEY, 0, false)
		!= 0;
	saveImageType = TCUserDefaults::longForKey(SAVE_IMAGE_TYPE_KEY, 1, false);
	saveExportType = TCUserDefaults::longForKey(SAVE_EXPORT_TYPE_KEY,
		LDrawModelViewer::ETPov, false);
	saveAlpha = TCUserDefaults::longForKey(SAVE_ALPHA_KEY, 0, false) != 0;
	autoCrop = TCUserDefaults::boolForKey(AUTO_CROP_KEY, false, false);
	saveAllSteps = TCUserDefaults::boolForKey(SAVE_STEPS_KEY, false, false);
	delete saveStepSuffix;
	saveStepSuffix = TCUserDefaults::stringForKeyUC(SAVE_STEPS_SUFFIX_KEY,
		TCLocalStrings::get(_UC("DefaultStepSuffix")), false);
	saveStepsSameScale = TCUserDefaults::boolForKey(SAVE_STEPS_SAME_SCALE_KEY,
		true, false);
}

// Note: static function
int ModelWindow::roundUp(int value, int nearest)
{
	return (value + nearest - 1) / nearest * nearest;
}

void ModelWindow::setFilename(const char* value)
{
	modelViewer->setFilename(value);
	checkForPart();
}

char* ModelWindow::getFilename(void)
{
	return modelViewer->getFilename();
}

void ModelWindow::update(void)
{
	doPaint();
}

void ModelWindow::finalSetup(void)
{
}

bool ModelWindow::getFileTime(FILETIME* fileTime)
{
	char* filename;

	if (modelViewer && (filename = modelViewer->getFilename()) != NULL)
	{
		WIN32_FIND_DATA findBuf;
		HANDLE findHandle;

		findHandle = FindFirstFile(filename, &findBuf);
		if (findHandle != INVALID_HANDLE_VALUE)
		{
			*fileTime = findBuf.ftLastWriteTime;
			FindClose(findHandle);
			return true;
		}
	}
	return false;
}

void ModelWindow::checkFileForUpdates(void)
{
	if (pollTimerRunning)
	{
		FILETIME newWriteTime;

		stopPolling();
		if (getFileTime(&newWriteTime))
		{
			if (lastWriteTime.dwLowDateTime !=newWriteTime.dwLowDateTime ||
				lastWriteTime.dwHighDateTime != newWriteTime.dwHighDateTime)
			{
				bool update = true;

				lastWriteTime = newWriteTime;
				if (pollSetting == POLL_PROMPT)
				{
					char message[1024];

					sprintf(message, TCLocalStrings::get("PollReloadCheck"));
					if (captureCount)
					{
						while (captureCount)
						{
							releaseMouse();
						}
						inputHandler->cancelMouseDrag();
					}
					stopAnimation();
					if (MessageBox(hWindow, message,
						TCLocalStrings::get("PollFileUpdate"),
						MB_OKCANCEL | MB_APPLMODAL | MB_ICONQUESTION) !=
						IDOK)
					{
						update = false;
					}
				}
				if (update)
				{
					reload();
				}
			}
		}
		startPolling();
	}
}

void ModelWindow::openGlWillEnd(void)
{
	if (modelViewer)
	{
		modelViewer->openGlWillEnd();
	}
}

void ModelWindow::updateFSAA()
{
	applyingPrefs = true;
	uncompile();
	closeWindow();
	if (!((LDViewWindow*)parentWindow)->getFullScreen())
	{
		x -= 2;
		y -= 2;
		width += 4;
		height += 4;
	}
	initWindow();
	if (modelViewer)
	{
		modelViewer->uncompile();
	}
	showWindow(SW_SHOW);
	applyingPrefs = false;
	killTimer(FSAA_UPDATE_TIMER);
	forceRedraw();
	if (hStatusBar)
	{
		// For some unknown reason, the status bar only updates its text on the
		// fly while the model is rotating if it is created after the model
		// window.  Since we just destroyed and recreated the model window, we
		// need to destroy and recreate the status window.
		((LDViewWindow*)parentWindow)->switchStatusBar();
		((LDViewWindow*)parentWindow)->switchStatusBar();
	}
}

LRESULT ModelWindow::doTimer(UINT_PTR timerID)
{
	switch (timerID)
	{
	case POLL_TIMER:
		checkFileForUpdates();
		break;
	case FSAA_UPDATE_TIMER:
		updateFSAA();
		break;
	}
	return 0;
}

void ModelWindow::captureMouse(void)
{
	SetCapture(hWindow);
}

void ModelWindow::releaseMouse(void)
{
	releasingMouse = true;
	ReleaseCapture();
	releasingMouse = false;
}

// NOTE: Static method.
bool ModelWindow::altPressed(void)
{
	return (GetKeyState(VK_MENU) & 0x8000) != 0;
}

// Note: static method.
TCULong ModelWindow::convertKeyModifiers(TCULong osModifiers)
{
	TCULong retValue = 0;

	if (osModifiers & MK_SHIFT)
	{
		retValue |= LDInputHandler::MKShift;
	}
	if (osModifiers & MK_CONTROL)
	{
		retValue |= LDInputHandler::MKControl;
	}
	return retValue;
}

// Note: static method.
TCULong ModelWindow::getCurrentKeyModifiers(void)
{
	TCULong retValue = 0;

	if (GetKeyState(VK_SHIFT) & 0x8000)
	{
		retValue |= LDInputHandler::MKShift;
	}
	if (GetKeyState(VK_CONTROL) & 0x8000)
	{
		retValue |= LDInputHandler::MKControl;
	}
	return retValue;
}

// Note: static method.
LDInputHandler::KeyCode ModelWindow::convertKeyCode(TCULong osKeyCode)
{
	if (isalpha(osKeyCode))
	{
		return (LDInputHandler::KeyCode)(toupper(osKeyCode) - 'A' +
			LDInputHandler::KCA);
	}
	else
	{
		switch (osKeyCode)
		{
		case VK_RETURN:
			return LDInputHandler::KCReturn;
		case VK_SHIFT:
			return LDInputHandler::KCShift;
		case VK_CONTROL:
			return LDInputHandler::KCControl;
		case VK_MENU:
			return LDInputHandler::KCAlt;
		case VK_SPACE:
			return LDInputHandler::KCSpace;
		case VK_PRIOR:
			return LDInputHandler::KCPageUp;
		case VK_NEXT:
			return LDInputHandler::KCPageDown;
		case VK_END:
			return LDInputHandler::KCEnd;
		case VK_HOME:
			return LDInputHandler::KCHome;
		case VK_ESCAPE:
			return LDInputHandler::KCEscape;
		case VK_LEFT:
			return LDInputHandler::KCLeft;
		case VK_UP:
			return LDInputHandler::KCUp;
		case VK_RIGHT:
			return LDInputHandler::KCRight;
		case VK_DOWN:
			return LDInputHandler::KCDown;
		case VK_INSERT:
			return LDInputHandler::KCInsert;
		case VK_DELETE:
			return LDInputHandler::KCDelete;
		default:
			return LDInputHandler::KCUnknown;
		}
	}
}

LRESULT ModelWindow::doLButtonDown(WPARAM keyFlags, int xPos, int yPos)
{
	if (inputHandler->mouseDown(convertKeyModifiers(keyFlags),
		LDInputHandler::MBLeft, xPos, yPos))
	{
		return 0;
	}
	return 1;
}

LRESULT ModelWindow::doLButtonUp(WPARAM keyFlags, int xPos, int yPos)
{
	if (inputHandler->mouseUp(keyFlags, LDInputHandler::MBLeft, xPos, yPos))
	{
		return 0;
	}
	return 1;
}

LRESULT ModelWindow::doRButtonDown(WPARAM keyFlags, int xPos, int yPos)
{
	if (inputHandler->mouseDown(convertKeyModifiers(keyFlags),
		LDInputHandler::MBRight, xPos, yPos))
	{
		return 0;
	}
	return 1;
}

LRESULT ModelWindow::doRButtonUp(WPARAM keyFlags, int xPos, int yPos)
{
	if (inputHandler->mouseUp(keyFlags, LDInputHandler::MBRight, xPos, yPos))
	{
		return 0;
	}
	return 1;
}

LRESULT ModelWindow::doMButtonDown(WPARAM keyFlags, int xPos, int yPos)
{
	if (inputHandler->mouseDown(convertKeyModifiers(keyFlags),
		LDInputHandler::MBMiddle, xPos, yPos))
	{
		return 0;
	}
	return 1;
}

LRESULT ModelWindow::doMButtonUp(WPARAM keyFlags, int xPos, int yPos)
{
	if (inputHandler->mouseUp(keyFlags, LDInputHandler::MBMiddle, xPos, yPos))
	{
		return 0;
	}
	return 1;
}

LRESULT ModelWindow::doCaptureChanged(HWND /*hNewWnd*/)
{
	if (!releasingMouse && inputHandler->mouseCaptureChanged())
	{
		return 0;
	}
	return 1;
}

LRESULT ModelWindow::doMouseMove(WPARAM keyFlags, int xPos, int yPos)
{
	if (inputHandler->mouseMove(convertKeyModifiers(keyFlags), xPos, yPos))
	{
		return 0;
	}
	return 1;
}

void ModelWindow::mouseWheel(short keyFlags, short zDelta)
{
	inputHandler->mouseWheel(convertKeyModifiers(keyFlags),
		(TCFloat)zDelta);
}

LRESULT ModelWindow::doEraseBackground(RECT* /*updateRect*/)
{
	created = TRUE;
	return 1;
}

LRESULT ModelWindow::doSize(WPARAM sizeType, int newWidth, int newHeight)
{
	RECT rect;
	POINT point;
	LRESULT retValue;

	GetWindowRect(hWindow, &rect);
	point.x = rect.left;
	point.y = rect.top;
	ScreenToClient(hParentWindow, &point);
	if (modelViewer && !hPBufferGLRC)
	{
		modelViewer->setWidth(newWidth);
		modelViewer->setHeight(newHeight);
	}
	if (hPBufferGLRC)
	{
		wglMakeCurrent(hdc, hglrc);
	}
	retValue = CUIOGLWindow::doSize(sizeType, newWidth, newHeight);
	if (hPBufferGLRC)
	{
		makeCurrent();
	}
	return retValue;
}

void ModelWindow::sizeView(void)
{
}

void ModelWindow::perspectiveView(void)
{
	modelViewer->perspectiveView();
}

void ModelWindow::forceRedraw(int count)
{
	if (hWindow)
	{
		InvalidateRect(hWindow, NULL, FALSE);
	}
	redrawCount += count;
}

void ModelWindow::setZoomSpeed(TCFloat value)
{
	modelViewer->setZoomSpeed(value);
}

TCFloat ModelWindow::getZoomSpeed(void)
{
	return modelViewer->getZoomSpeed();
}

void ModelWindow::resetView(LDVAngle viewAngle)
{
	modelViewer->resetView(viewAngle);
	modelViewer->setXRotate(0.0f);
	modelViewer->setYRotate(0.0f);
	stopAnimation();
	fps = 0.0f;
	forceRedraw();
}

void ModelWindow::saveDefaultView(void)
{
	prefs->saveDefaultView();
}

void ModelWindow::resetDefaultView(void)
{
	prefs->resetDefaultView();
}

void ModelWindow::reload(void)
{
	clearErrors();
	modelViewer->setNeedsReload();
	forceRedraw();
}

void ModelWindow::recompile(void)
{
	modelViewer->recompile();
}

void ModelWindow::uncompile(void)
{
	if (modelViewer)
	{
		modelViewer->uncompile();
	}
}

BOOL ModelWindow::doErrorOK(void)
{
	doDialogClose(hErrorWindow);
	return TRUE;
}

void ModelWindow::doProgressCancel(void)
{
	cancelLoad = true;
}

void ModelWindow::getTreeViewLine(HWND hTreeView, HTREEITEM hItem,
								  TCStringArray *lines)
{
	TVITEM item;
	char buf1[1024];
	char buf2[1024] = "";
	int depth = 0;
	HTREEITEM hParentItem = hItem;

	while ((hParentItem = TreeView_GetParent(hTreeView, hParentItem)) != NULL)
	{
		depth++;
	}
	item.mask = TVIF_TEXT;
	item.hItem = hItem;
	item.pszText = buf1;
	item.cchTextMax = 1024 - depth;
	if (TreeView_GetItem(hTreeView, &item))
	{
		int i;

		for (i = 0; i < depth; i++)
		{
			strcat(buf2, "\t");
		}
		strcat(buf2, buf1);
		lines->addString(buf2);
	}
}

BOOL ModelWindow::doErrorTreeCopy(void)
{
	HTREEITEM hItem = TreeView_GetSelection(hErrorTree);

	if (hItem)
	{
		HTREEITEM hParentItem = TreeView_GetParent(hErrorTree, hItem);
		TCStringArray* textLines = new TCStringArray;
		int i;
		int count;

		while (hParentItem)
		{
			hItem = hParentItem;
			hParentItem = TreeView_GetParent(hErrorTree, hItem);
		}
		getTreeViewLine(hErrorTree, hItem, textLines);
		hItem = TreeView_GetChild(hErrorTree, hItem);
		while (hItem)
		{
			getTreeViewLine(hErrorTree, hItem, textLines);
			hItem = TreeView_GetNextSibling(hErrorTree, hItem);
		}
		count = textLines->getCount();
		if (count)
		{
			size_t len = 1;
			char *buf;

			for (i = 0; i < count; i++)
			{
				len += strlen((*textLines)[i]) + 2;
			}
			buf = new char[len];
			buf[0] = 0;
			for (i = 0; i < count; i++)
			{
				strcat(buf, (*textLines)[i]);
				strcat(buf, "\r\n");
			}
			if (copyToClipboard(buf))
			{
				delete buf;
				SetWindowLongPtr(hErrorWindow, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
			delete buf;
		}
	}
	return FALSE;
}

BOOL ModelWindow::doErrorTreeKeyDown(LPNMTVKEYDOWN notification)
{
//	debugPrintf("ModelWindow::doErrorTreeKeyDown: 0x%08X, 0x%04X, 0x%08X\n",
//		notification->hdr.code, notification->wVKey, notification->flags);
	if (notification->wVKey == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		return doErrorTreeCopy();
	}
	return FALSE;
}

BOOL ModelWindow::doErrorWindowNotify(LPNMHDR notification)
{
//	debugPrintf("ModelWindow::doErrorWindowNotify: %d\n", notification->code);
	switch (notification->code)
	{
	case LVN_ITEMCHANGED:
		LPNMLISTVIEW info = (LPNMLISTVIEW)notification;
		if (info->uNewState != info->uOldState)
		{
			BOOL value = ListView_GetCheckState(hErrorList, info->iItem);
			ErrorInfo *errorInfo = (*errorInfos)[info->iItem];
			char *errorKey = getErrorKey(errorInfo->getType());

			// The new state/old state check lies when you click on an item, and
			// not the item's check box.
			if (TCUserDefaults::longForKey(errorKey, 0, false) != value)
			{
				TCUserDefaults::setLongForKey(value, errorKey, false);
				if (!skipErrorUpdates)
				{
					clearErrorTree();
					populateErrorTree();
				}
			}
		}
		break;
	}
	return FALSE;
}

BOOL ModelWindow::doErrorTreeNotify(LPNMHDR notification)
{
//	debugPrintf("ModelWindow::doErrorTreeNotify: %d\n", notification->code);
	if (notification->code == NM_DBLCLK)
	{
		HTREEITEM hSelectedItem = TreeView_GetSelection(hErrorTree);

//		debugPrintf("double click!\n");
		if (hSelectedItem)
		{
			TVITEM selectedItem;
			char buf[1024];

			selectedItem.mask = TVIF_TEXT;
			selectedItem.hItem = hSelectedItem;
			selectedItem.pszText = buf;
			selectedItem.cchTextMax = 1024;
			if (TreeView_GetItem(hErrorTree, &selectedItem))
			{
				if (stringHasPrefix(buf,
					TCLocalStrings::get("ErrorTreeFilePrefix")))
				{
					char *editor = TCUserDefaults::stringForKey(EDITOR_KEY,
						"notepad.exe", false);

					ShellExecute(hWindow, NULL, editor, buf + 6, ".",
						SW_SHOWNORMAL);
					delete editor;
				}
			}
		}
		else
		{
//			debugPrintf("No selection.\n");
		}
	}
	else if (notification->code == TVN_KEYDOWN)
	{
		return doErrorTreeKeyDown((LPNMTVKEYDOWN)notification);
	}
	return FALSE;
}

BOOL ModelWindow::doDialogNotify(HWND hDlg, int controlId, LPNMHDR notification)
{
//	debugPrintf("ModelWindow::doDialogNotify: 0x%04X, 0x%04X, 0x%04x\n", hDlg,
//		controlId, notification->code);
	if (hDlg == hErrorWindow)
	{
		if (controlId == IDC_ERROR_TREE)
		{
			return doErrorTreeNotify(notification);
		}
		else
		{
			return doErrorWindowNotify(notification);
		}
	}
	else if (hDlg == hSaveDialog)
	{
		return doSaveNotify(controlId, (LPOFNOTIFY)notification);
	}
	return FALSE;
}

BOOL ModelWindow::doErrorSize(WPARAM sizeType, int newWidth, int newHeight)
{
	//RECT sizeRect;

	SendMessage(hErrorStatusWindow, WM_SIZE, sizeType,
		MAKELPARAM(newWidth, newHeight));
	//GetClientRect(hErrorWindow, &sizeRect);
	errorWindowResizer->resize(newWidth, newHeight);
	return FALSE;
}

BOOL ModelWindow::doSaveSize(WPARAM sizeType, int newWidth, int newHeight)
{
	if (saveWindowResizer)
	{
		if (sizeType != SW_MINIMIZE)
		{
			saveWindowResizer->resize(newWidth, newHeight);
			positionSaveAddOn();
		}
	}
	return FALSE;
}

void ModelWindow::positionSaveOptionsButton(void)
{
	RECT optionsButtonRect;
	RECT typeComboRect;
	HWND hParent = GetParent(hSaveDialog);
	HWND hTypeCombo = GetDlgItem(hParent, cmb1);
	int typeComboWidth;
	int typeComboHeight;
	int optionsButtonWidth;
	int optionsButtonHeight;

	// Move the Options button so that it is to the right of the file type
	// combo box.
	GetWindowRect(hTypeCombo, &typeComboRect);
	screenToClient(hParent, &typeComboRect);
	GetWindowRect(hSaveOptionsButton, &optionsButtonRect);
	optionsButtonWidth = optionsButtonRect.right - optionsButtonRect.left;
	optionsButtonHeight = optionsButtonRect.bottom - optionsButtonRect.top;
	typeComboWidth = typeComboRect.right - typeComboRect.left;
	typeComboHeight = typeComboRect.bottom - typeComboRect.top;
	clientToScreen(hParent, &typeComboRect);
	screenToClient(hSaveDialog, &typeComboRect);

	MoveWindow(hSaveOptionsButton, typeComboRect.right + 6,
		typeComboRect.top + typeComboHeight / 2 - optionsButtonHeight / 2,
		optionsButtonWidth, optionsButtonHeight, TRUE);
}

void ModelWindow::positionSaveAddOn(void)
{
	HWND hParent = GetParent(hSaveDialog);
	RECT parentClientRect;
	RECT addOnRect;
	int windowWidth;
	int windowHeight;

	GetClientRect(hParent, &parentClientRect);
	GetWindowRect(hSaveDialog, &addOnRect);
	screenToClient(hParent, &addOnRect);
	windowWidth = parentClientRect.right - parentClientRect.left;
	windowHeight = parentClientRect.bottom - parentClientRect.top;
	if (addOnRect.left != 0 || addOnRect.top != 0 ||
		addOnRect.right != windowWidth || addOnRect.bottom != windowHeight)
	{
		// Only move it if it has actually changed position.  That's because
		// this function is called from doSaveSize, and we don't want to go
		// into an infinite loop.
		MoveWindow(hSaveDialog, 0, 0, windowWidth, windowHeight, TRUE);
	}
	positionSaveOptionsButton();
}

BOOL ModelWindow::doSaveInitDone(OFNOTIFY * /*ofNotify*/)
{
	RECT optionsButtonRect;
	RECT typeComboRect;
	RECT miscRect;
	RECT parentRect;
	RECT cancelRect;
	RECT windowRect;
	HWND hParent = GetParent(hSaveDialog);
	HWND hMiscBox = GetDlgItem(hSaveDialog, IDC_MISC_BOX);
	HWND hCancelButton = GetDlgItem(hParent, IDCANCEL);
	HWND hTypeCombo = GetDlgItem(hParent, cmb1);
	int typeComboWidth;
	int typeComboHeight;
	int optionsButtonWidth;

	// Shrink the type combo to make space for the options button.
	GetWindowRect(hTypeCombo, &typeComboRect);
	GetWindowRect(hSaveOptionsButton, &optionsButtonRect);
	screenToClient(hParent, &typeComboRect);
	optionsButtonWidth = optionsButtonRect.right - optionsButtonRect.left;
	typeComboWidth = typeComboRect.right - typeComboRect.left
		- optionsButtonWidth - 6;
	typeComboHeight = typeComboRect.bottom - typeComboRect.top;
	MoveWindow(hTypeCombo, typeComboRect.left, typeComboRect.top,
		typeComboWidth, typeComboHeight, TRUE);

	// Resize the add-on window so that the right margin between the misc box
	// and the add-on window is the same as the right margin between the save
	// dialog's cancel button and the save dialog.  That way, the resizer will
	// consider that to be the "proper" right margin, and when we resize the
	// add-on to fill the available space, everything will size properly.
	GetClientRect(hParent, &parentRect);
	GetWindowRect(hCancelButton, &cancelRect);
	screenToClient(hParent, &cancelRect);
	GetWindowRect(hMiscBox, &miscRect);
	screenToClient(hSaveDialog, &miscRect);
	GetClientRect(hParent, &windowRect);
	windowRect.right = miscRect.right + parentRect.right - cancelRect.right;
	MoveWindow(hSaveDialog, 0, 0, windowRect.right, windowRect.bottom, TRUE);

	saveWindowResizer = new CUIWindowResizer;
	saveWindowResizer->setHWindow(hSaveDialog);

	if (curSaveOp == LDPreferences::SOSnapshot)
	{
		saveWindowResizer->addSubWindow(IDC_SAVE_SERIES_BOX,
			CUISizeHorizontal | CUIFloatRight | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_SAVE_SERIES,
			CUIFloatRight | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_SAVE_DIGITS_LABEL,
			CUIFloatRight | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_SAVE_DIGITS,
			CUIFloatRight | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_SAVE_DIGITS_SPIN,
			CUIFloatRight | CUIFloatTop);

		saveWindowResizer->addSubWindow(IDC_SAVE_ACTUAL_SIZE_BOX,
			CUISizeHorizontal | CUIFloatRight | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_SAVE_ACTUAL_SIZE,
			CUIFloatRight | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_SAVE_WIDTH_LABEL,
			CUIFloatRight | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_SAVE_WIDTH,
			CUIFloatRight | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_SAVE_HEIGHT_LABEL,
			CUIFloatRight | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_SAVE_HEIGHT,
			CUIFloatRight | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_SAVE_ZOOMTOFIT,
			CUIFloatRight | CUIFloatTop);

		saveWindowResizer->addSubWindow(IDC_ALL_STEPS_BOX,
			CUIFloatLeft | CUISizeHorizontal | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_STEP_SUFFIX_LABEL,
			CUIFloatLeft | CUIFloatTop | CUIFloatRight);
		saveWindowResizer->addSubWindow(IDC_STEP_SUFFIX,
			CUIFloatLeft | CUIFloatTop | CUIFloatRight);
		saveWindowResizer->addSubWindow(IDC_SAME_SCALE,
			CUIFloatLeft | CUIFloatTop | CUIFloatRight);

		saveWindowResizer->addSubWindow(IDC_MISC_BOX,
			CUIFloatLeft | CUISizeHorizontal | CUIFloatTop);
		saveWindowResizer->addSubWindow(IDC_ALL_STEPS,
			CUIFloatLeft | CUIFloatTop | CUIFloatRight);
		saveWindowResizer->addSubWindow(IDC_IGNORE_PBUFFER,
			CUIFloatLeft | CUIFloatTop | CUIFloatRight);
		saveWindowResizer->addSubWindow(IDC_TRANSPARENT_BACKGROUND,
			CUIFloatLeft | CUIFloatTop | CUIFloatRight);
		saveWindowResizer->addSubWindow(IDC_AUTO_CROP,
			CUIFloatLeft | CUIFloatTop | CUIFloatRight);
	}
	positionSaveAddOn();

	return FALSE;
}

BOOL ModelWindow::doDialogSize(HWND hDlg, WPARAM sizeType, int newWidth,
							   int newHeight)
{
//	debugPrintf("ModelWindow::doDialogSize(%d, %d, %d)\n", sizeType, newWidth,
//		newHeight);
	if (hDlg == hErrorWindow)
	{
		return doErrorSize(sizeType, newWidth, newHeight);
	}
	else if (hDlg == hSaveDialog)
	{
		return doSaveSize(sizeType, newWidth, newHeight);
	}
	return FALSE;
}

BOOL ModelWindow::doDialogGetMinMaxInfo(HWND hDlg, LPMINMAXINFO minMaxInfo)
{
	if (hDlg == hErrorWindow)
	{
		calcSystemSizes();
		minMaxInfo->ptMaxSize.x = systemMaxWidth;
		minMaxInfo->ptMaxSize.y = systemMaxHeight;
		minMaxInfo->ptMinTrackSize.x = 475;
		minMaxInfo->ptMinTrackSize.y = 260;
		minMaxInfo->ptMaxTrackSize.x = systemMaxTrackWidth;
		minMaxInfo->ptMaxTrackSize.y = systemMaxTrackHeight;
		return TRUE;
	}
	return FALSE;
}

BOOL ModelWindow::doProgressClick(int controlId, HWND /*controlHWnd*/)
{
	if (controlId == IDCANCEL)
	{
		doProgressCancel();
	}
	return TRUE;
}

char* ModelWindow::getErrorKey(LDLErrorType errorType)
{
	static char key[128];

	sprintf(key, "%s/LDLError%02d", SHOW_ERRORS_KEY, errorType);
	return key;
}

BOOL ModelWindow::doErrorClick(int controlId, HWND /*controlHWnd*/)
{
	switch (controlId)
	{
	case IDCANCEL:
		return doErrorOK();
		break;
	case IDC_COPY_ERROR:
		if (!doErrorTreeCopy())
		{
			MessageBeep(MB_OK);
		}
		break;
	case IDC_SHOW_WARNINGS:
		int value;

		value = SendDlgItemMessage(hErrorWindow, controlId, BM_GETCHECK,
			0, 0);
		TCUserDefaults::setLongForKey(value, SHOW_WARNINGS_KEY, false);
		clearErrorTree();
		populateErrorTree();
		break;
	case IDC_ERROR_SHOW_ALL:
		return setAllErrorsSelected(true);
		break;
	case IDC_ERROR_SHOW_NONE:
		return setAllErrorsSelected(false);
		break;
	}
	return TRUE;
}

BOOL ModelWindow::setAllErrorsSelected(bool selected)
{
	int i;
	int count = errorInfos->getCount();
	BOOL state = selected ? TRUE : FALSE;

	skipErrorUpdates = true;
	for (i = 0; i < count; i++)
	{
		ListView_SetCheckState(hErrorList, i, state);
	}
	skipErrorUpdates = false;
	clearErrorTree();
	populateErrorTree();
	return TRUE;
}

BOOL ModelWindow::doPageSetupClick(int controlId, HWND /*controlHWnd*/)
{
	switch (controlId)
	{
	case IDC_PRINT_BACKGROUND:
		printBackground = SendDlgItemMessage(hPageSetupDialog,
			IDC_PRINT_BACKGROUND, BM_GETCHECK, 0, 0) ? true : false;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void ModelWindow::applyPrefs(void)
{
	bool antialiasChanged = TCUserDefaults::longForKey(FSAA_MODE_KEY) !=
		currentAntialiasType;

	loadSettings();
	((LDViewWindow*)parentWindow)->applyPrefs();
	if (LDVExtensionsSetup::haveMultisampleExtension() && antialiasChanged)
	{
		setTimer(FSAA_UPDATE_TIMER, 0);
	}
	forceRedraw();
}

LRESULT ModelWindow::doCommand(int /*itemId*/, int notifyCode, HWND controlHWnd)
{
	if (controlHWnd == hPrefsWindow && controlHWnd != NULL)
	{
		switch (notifyCode)
		{
		case CUI_OK:
			stopAnimation();
			prefs->closePropertySheet();
			hPrefsWindow = NULL;
			applyPrefs();
			break;
		case CUI_CANCEL:
			stopAnimation();
			prefs->closePropertySheet();
			hPrefsWindow = NULL;
			break;
		case CUI_APPLY:
			applyPrefs();
			break;
		}
	}
	return 1;
}

BOOL ModelWindow::doDialogHelp(HWND hDlg, LPHELPINFO helpInfo)
{
	BOOL retValue = FALSE;
	DWORD dialogId = 0;

	if (hDlg == hSaveDialog)
	{
		dialogId = IDD_SAVE_OPTIONS;
	}
	if (dialogId)
	{
		char* helpPath = LDViewPreferences::getLDViewPath(
			TCLocalStrings::get("LDView.hlp"));
		DWORD helpId;

		helpId = 0x80000000 | (dialogId << 16) | (DWORD)helpInfo->iCtrlId;
		WinHelp((HWND)helpInfo->hItemHandle, helpPath, HELP_CONTEXTPOPUP,
			helpId);
		retValue = TRUE;
		delete helpPath;
	}
	return retValue;
}

BOOL ModelWindow::doDialogCommand(HWND hDlg, int controlId, int notifyCode,
								  HWND controlHWnd)
{
//	debugPrintf("ModelWindow::doDialogCommand(0x%08X, 0x%04X, 0x%04X, 0x%08X)\n",
//		hDlg, controlId, notifyCode, controlHWnd);
	if (hDlg == hSaveDialog)
	{
		return doSaveCommand(controlId, notifyCode, controlHWnd);
	}
	else if (hDlg == hPrintDialog)
	{
		return doPrintCommand(controlId, notifyCode, controlHWnd);
	}
	if (notifyCode == BN_CLICKED)
	{
		if (hDlg == hProgressWindow)
		{
			return doProgressClick(controlId, controlHWnd);
		}
		else if (hDlg == hErrorWindow)
		{
			return doErrorClick(controlId, controlHWnd);
		}
		else if (hDlg == hPageSetupDialog)
		{
			return doPageSetupClick(controlId, controlHWnd);
		}
	}
	return FALSE;
}

BOOL ModelWindow::showPreferences(void)
{
	prefs->setHDlgParent(GetParent(hWindow));
	hPrefsWindow = (HWND)prefs->show();
	return TRUE;
}

void ModelWindow::showErrors(void)
{
	showErrorsIfNeeded(FALSE);
}

void ModelWindow::hideProgress(void)
{
	if (loading)
	{
		SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
		SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)"");
		((LDViewWindow *)parentWindow)->redrawStatusBar();
		EnumThreadWindows(GetWindowThreadProcessId(hParentWindow, NULL),
			enableNonModalWindow, (LPARAM)hParentWindow);
		((LDViewWindow*)parentWindow)->setLoading(false);
//		doDialogClose(hProgressWindow);
		loading = false;
		((LDViewWindow*)parentWindow)->forceShowStatusBar(false);
	}
}

HTREEITEM ModelWindow::addErrorLine(HTREEITEM parent, char* line,
									LDLError* error, int imageIndex)
{
	TVINSERTSTRUCT insertStruct;
	TVITEMEX item;

	stripCRLF(line);
	memset(&item, 0, sizeof(item));
	item.mask = TVIF_TEXT | TVIF_PARAM;
	item.pszText = line;
	item.lParam = (LPARAM)error;
	insertStruct.hParent = parent;
	insertStruct.hInsertAfter = TVI_LAST;
	if (error->getLevel() != LDLAWarning && parent == NULL)
	{
		item.mask |= TVIF_STATE;
		item.stateMask = TVIS_BOLD;
		item.state = TVIS_BOLD;
	}
	if (imageIndex >= 0)
	{
        item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
        item.iImage = imageIndex; 
        item.iSelectedImage = imageIndex; 
	}
	insertStruct.itemex = item;
	return TreeView_InsertItem(hErrorTree, &insertStruct);
}

bool ModelWindow::addError(LDLError* error)
{
	char *buf;
	const char* string;
	HTREEITEM parent;

	if (!showsError(error))
	{
		return false;
	}
	string = error->getMessage();
	if (!string)
	{
		string = "";
	}
	buf = copyString(string);
	parent = addErrorLine(NULL, buf, error,
		errorImageIndices[error->getType()]);
	delete buf;

	if (parent)
	{
		TCStringArray *extraInfo;

   		string = error->getFilename();
		buf = new char[strlen(string) + 512];
		if (string)
		{
			sprintf(buf, "%s%s", TCLocalStrings::get("ErrorTreeFilePrefix"),
				string);
		}
		else
		{
			sprintf(buf, TCLocalStrings::get("ErrorTreeUnknownFile"));
		}
		addErrorLine(parent, buf, error);
		delete buf;
		string = error->getFileLine();
		if (string)
		{
			int lineNumber = error->getLineNumber();

			buf = new char[strlen(string) + 512];
			if (lineNumber > 0)
			{
				sprintf(buf, TCLocalStrings::get("ErrorTreeLine#"), lineNumber);
			}
			else
			{
				sprintf(buf, TCLocalStrings::get("ErrorTreeUnknownLine#"));
			}
			addErrorLine(parent, buf, error);
			sprintf(buf, TCLocalStrings::get("ErrorTreeLine"), string);
		}
		else
		{
			buf = new char[512];
			sprintf(buf, TCLocalStrings::get("ErrorTreeUnknownLine"));
		}
		addErrorLine(parent, buf, error);
		delete buf;
		if ((extraInfo = error->getExtraInfo()) != NULL)
		{
			int i;
			int count = extraInfo->getCount();

			for (i = 0; i < count; i++)
			{
				addErrorLine(parent, extraInfo->stringAtIndex(i), error);
			}
		}
	}
	return true;
}

bool ModelWindow::showsError(LDLError *error)
{
	LDLErrorType errorType = error->getType();

	if (error->getLevel() == LDLAWarning)
	{
		if (TCUserDefaults::longForKey(SHOW_WARNINGS_KEY, 0, false))
		{
			return TCUserDefaults::longForKey(getErrorKey(errorType), 0, false)
				!= 0;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return TCUserDefaults::longForKey(getErrorKey(errorType), 1, false) !=
			0;
	}
}

BOOL ModelWindow::showsErrorType(LDLErrorType errorType)
{
	return TCUserDefaults::longForKey(getErrorKey(errorType), 1, false);
}

void ModelWindow::populateErrorInfos(void)
{
	if (!errorInfos)
	{
		int i;

		errorInfos = new ErrorInfoArray;
		for (i = LDLEFirstError; i <= LDLELastError; i++)
		{
			ErrorInfo *errorInfo = new ErrorInfo;
			LDLErrorType type = (LDLErrorType)i;

			errorInfo->setType(type);
			errorInfo->setTypeName(LDLError::getTypeName(type));
			errorInfos->addObject(errorInfo);
			errorInfo->release();
		}
	}
}

void ModelWindow::setupErrorWindow(void)
{
	HIMAGELIST himl;  // handle to image list
	HBITMAP hbmp;     // handle to bitmap
	HBITMAP hMask;
	long showWarnings = TCUserDefaults::longForKey(SHOW_WARNINGS_KEY, 0, false);

	populateErrorInfos();
	populateErrorList();
	memset(errorImageIndices, 0, sizeof(errorImageIndices));
	if (errorWindowResizer)
	{
		errorWindowResizer->release();
	}
	errorWindowResizer = new CUIWindowResizer;
	errorWindowResizer->setHWindow(hErrorWindow);
	errorWindowResizer->addSubWindow(IDC_ERROR_TREE,
		CUISizeHorizontal | CUISizeVertical);
	errorWindowResizer->addSubWindow(IDC_COPY_ERROR,
		CUIFloatLeft | CUIFloatTop);
	errorWindowResizer->addSubWindow(IDC_ERROR_SHOW_ALL,
		CUIFloatLeft | CUIFloatTop);
	errorWindowResizer->addSubWindow(IDC_ERROR_SHOW_NONE,
		CUIFloatLeft | CUIFloatTop);
	errorWindowResizer->addSubWindow(IDC_SHOW_WARNINGS, CUIFloatTop);
	errorWindowResizer->addSubWindow(IDC_SHOW_ERRORS,
		CUIFloatLeft | CUISizeVertical);
	errorWindowResizer->addSubWindow(IDC_ERROR_LIST,
		CUIFloatLeft | CUISizeVertical);

	SendDlgItemMessage(hErrorWindow, IDC_SHOW_WARNINGS, BM_SETCHECK,
		showWarnings, 0);
	// Create the image list.
	if ((himl = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 8, 0)) == NULL)
		return;

	// Add the bitmaps.
	hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_INFO));
	hMask = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_INFO_MASK));
	ImageList_Add(himl, hbmp, hMask);
	DeleteObject(hbmp);
	DeleteObject(hMask);

	hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_PARSE));
	hMask = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_PARSE_MASK));
	errorImageIndices[LDLEParse] = ImageList_Add(himl, hbmp, hMask);
	errorImageIndices[LDLEGeneral] = errorImageIndices[LDLEParse];
	errorImageIndices[LDLEBFCError] = errorImageIndices[LDLEParse];
	errorImageIndices[LDLEMPDError] = errorImageIndices[LDLEParse];
	errorImageIndices[LDLEMetaCommand] = errorImageIndices[LDLEParse];
	DeleteObject(hbmp);
	DeleteObject(hMask);

	hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FNF));
	hMask = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FNF_MASK));
	errorImageIndices[LDLEFileNotFound] = ImageList_Add(himl, hbmp, hMask);
	DeleteObject(hbmp);
	DeleteObject(hMask);

	hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_MATRIX));
	hMask = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_MATRIX_MASK));
	errorImageIndices[LDLEMatrix] = ImageList_Add(himl, hbmp, hMask);
	DeleteObject(hbmp);
	DeleteObject(hMask);

	hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DETERMINANT));
	hMask = LoadBitmap(hInstance,
		MAKEINTRESOURCE(IDB_DETERMINANT_MASK));
	errorImageIndices[LDLEPartDeterminant] = ImageList_Add(himl, hbmp, hMask);
	DeleteObject(hbmp);
	DeleteObject(hMask);

	hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_NON_FLAT_QUAD));
	hMask = LoadBitmap(hInstance,
		MAKEINTRESOURCE(IDB_NON_FLAT_QUAD_MASK));
	errorImageIndices[LDLENonFlatQuad] = ImageList_Add(himl, hbmp, hMask);
	DeleteObject(hbmp);
	DeleteObject(hMask);

	hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_CONCAVE_QUAD));
	hMask = LoadBitmap(hInstance,
		MAKEINTRESOURCE(IDB_CONCAVE_QUAD_MASK));
	errorImageIndices[LDLEConcaveQuad] = ImageList_Add(himl, hbmp, hMask);
	DeleteObject(hbmp);
	DeleteObject(hMask);

	hbmp = LoadBitmap(hInstance,
		MAKEINTRESOURCE(IDB_MATCHING_POINTS));
	hMask = LoadBitmap(hInstance,
		MAKEINTRESOURCE(IDB_MATCHING_POINTS_MASK));
	errorImageIndices[LDLEMatchingPoints] = ImageList_Add(himl, hbmp, hMask);
	DeleteObject(hbmp);
	DeleteObject(hMask);

	hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_COLINEAR));
	hMask = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_COLINEAR_MASK));
	errorImageIndices[LDLEColinear] = ImageList_Add(himl, hbmp, hMask);
	DeleteObject(hbmp);
	DeleteObject(hMask);

	hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_VERTEX_ORDER));
	hMask = LoadBitmap(hInstance,
		MAKEINTRESOURCE(IDB_VERTEX_ORDER_MASK));
	errorImageIndices[LDLEVertexOrder] = ImageList_Add(himl, hbmp, hMask);
	DeleteObject(hbmp);
	DeleteObject(hMask);

	hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_ERROR_LOOP));
	hMask = LoadBitmap(hInstance,
		MAKEINTRESOURCE(IDB_ERROR_LOOP_MASK));
	errorImageIndices[LDLEModelLoop] = ImageList_Add(himl, hbmp, hMask);
	DeleteObject(hbmp);
	DeleteObject(hMask);

	// Associate the image list with the tree view control.
	TreeView_SetImageList(hErrorTree, himl, TVSIL_NORMAL);
//	TreeView_SetItemHeight(hErrorTree, 18);
}

void ModelWindow::setupProgress(void)
{
	cancelLoad = false;
}

void ModelWindow::registerErrorWindowClass(void)
{
	WNDCLASSEX windowClass;
	char prefsClassName[1024];

	if (!hProgressWindow)
	{
		createProgress();
	}
	GetClassName(hProgressWindow, prefsClassName, 1024);
	windowClass.cbSize = sizeof(windowClass);
	GetClassInfoEx(getLanguageModule(), prefsClassName, &windowClass);
	windowClass.hIcon = LoadIcon(getLanguageModule(),
		MAKEINTRESOURCE(IDI_APP_ICON));
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = "LDViewErrorWindow";
	RegisterClassEx(&windowClass);
}

void ModelWindow::initCommonControls(DWORD mask)
{
	static DWORD initializedMask = 0;

	if ((initializedMask & mask) != mask)
	{
		INITCOMMONCONTROLSEX initCtrls;

		initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
		initCtrls.dwICC = mask;
		InitCommonControlsEx(&initCtrls);
		initializedMask |= mask;
	}
}

void ModelWindow::createErrorWindow(void)
{
	if (!hErrorWindow)
	{
		HWND hActiveWindow = GetActiveWindow();
		int parts[] = {-1};

		initCommonControls(ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES);
		registerErrorWindowClass();
		hErrorWindow = createDialog(IDD_ERRORS, FALSE);
		hErrorStatusWindow = CreateStatusWindow(
			WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, "", hErrorWindow,
			2000);
		SendMessage(hErrorStatusWindow, SB_SETPARTS, 1, (LPARAM)parts);
		originalErrorDlgProc = (WNDPROC)GetWindowLongPtr(hErrorWindow,
			DWLP_DLGPROC);
		SetWindowLongPtr(hErrorWindow, GWLP_USERDATA, (LONG_PTR)this);
		SetWindowLongPtr(hErrorWindow, DWLP_DLGPROC, (LONG_PTR)staticErrorDlgProc);
		hErrorTree = GetDlgItem(hErrorWindow, IDC_ERROR_TREE);
		hErrorList = GetDlgItem(hErrorWindow, IDC_ERROR_LIST);
//		hErrorOk = GetDlgItem(hErrorWindow, IDOK);
		setupErrorWindow();
		SetActiveWindow(hActiveWindow);
	}
}

void ModelWindow::createProgress(void)
{
	initCommonControls(ICC_PROGRESS_CLASS | ICC_UPDOWN_CLASS);
	hProgressWindow = createDialog(IDD_LOAD_PROGRESS);
	hProgressMessage = GetDlgItem(hProgressWindow, IDC_LOAD_PROGRESS_MSG);
	hProgressCancelButton = GetDlgItem(hProgressWindow, IDCANCEL);
	hProgress = GetDlgItem(hProgressWindow, IDC_PROGRESS);
}

void ModelWindow::clearErrorTree(void)
{
//	RECT rect;
//	POINT topLeft;

	SetWindowRedraw(hErrorTree, FALSE);
	if (hErrorWindow)
	{
		HTREEITEM hItem;
		
		// Delete all is really, REALLY, slow for trees with lots of data.  I
		// think it starts at the beginning, and keeps copying the items back
		// each time the first is deleted.  Oh, and the "Visible" in
		// "LastVisible" doesn't mean it's visible on-screen, just that its
		// parent is expanded.
		while ((hItem = TreeView_GetLastVisible(hErrorTree)) != NULL)
		{
			TreeView_DeleteItem(hErrorTree, hItem);
		}
		TreeView_DeleteItem(hErrorTree, TVI_ROOT);
	}
	SetWindowRedraw(hErrorTree, TRUE);
	RedrawWindow(hErrorTree, NULL, NULL,
		RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
/*
	GetWindowRect(hErrorTree, &rect);
	topLeft.x = rect.left;
	topLeft.y = rect.top;
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
	ScreenToClient(GetParent(hErrorTree), &topLeft);
//	MoveWindow(hErrorTree, topLeft.x, topLeft.y, width - 1, height, TRUE);
	MoveWindow(hErrorTree, topLeft.x, topLeft.y, width, height, TRUE);
*/
	errorTreePopulated = false;
}

void ModelWindow::clearErrors(void)
{
	userLoad = true;
	errors->removeAll();
/*
	while (errors->getCount())
	{
		errors->removeObjectAtIndex(0);
	}
*/
	clearErrorTree();
}

void ModelWindow::populateErrorList(void)
{
	int i;
	int count = errorInfos->getCount();
	LVCOLUMN column;
	DWORD exStyle = ListView_GetExtendedListViewStyle(hErrorList);

	skipErrorUpdates = true;
	exStyle |= LVS_EX_CHECKBOXES;
	ListView_SetExtendedListViewStyle(hErrorList, exStyle);
	memset(&column, 0, sizeof(LVCOLUMN));
	column.mask = LVCF_FMT | LVCF_WIDTH;
	column.fmt = LVCFMT_LEFT;
	column.cx = 300;
	ListView_InsertColumn(hErrorList, 0, &column);
	for (i = 0; i < count; i++)
	{
		ErrorInfo *errorInfo = (*errorInfos)[i];
		LVITEM item;
		int state;

		memset(&item, 0, sizeof(item));
		item.mask = TVIF_TEXT | TVIF_PARAM;
		item.pszText = errorInfo->getTypeName();
		item.lParam = errorInfo->getType();
		item.iItem = i;
		item.iSubItem = 0;
		state = TCUserDefaults::longForKey(getErrorKey(errorInfo->getType()),
			1, false);
		ListView_InsertItem(hErrorList, &item);
		ListView_SetCheckState(hErrorList, i, state);
	}
	skipErrorUpdates = false;
	ListView_SetColumnWidth(hErrorList, 0, LVSCW_AUTOSIZE);
}

int ModelWindow::populateErrorTree(void)
{
	char buf[128] = "";
//	RECT rect;
//	POINT topLeft;
//	int width;
//	int height;

	SetWindowRedraw(hErrorTree, FALSE);
	if (!windowShown)
	{
		return 0;
	}
	if (hErrorWindow && !errorTreePopulated)
	{
		errorCount = 0;
		warningCount = 0;
		for (int i = 0, count = errors->getCount(); i < count; i++)
		{
			LDLError *error = (*errors)[i];

			if (addError(error))
			{
				if (error->getLevel() == LDLAWarning)
				{
					warningCount++;
				}
				else
				{
					errorCount++;
				}
			}
		}
		errorTreePopulated = true;
	}
	SetWindowRedraw(hErrorTree, TRUE);
	RedrawWindow(hErrorTree, NULL, NULL,
		RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
/*
	GetWindowRect(hErrorTree, &rect);
	topLeft.x = rect.left;
	topLeft.y = rect.top;
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
	ScreenToClient(GetParent(hErrorTree), &topLeft);
	if (errorCount > 0 || warningCount > 0)
	{
		MoveWindow(hErrorTree, topLeft.x, topLeft.y, width - 1, height, FALSE);
	}
	MoveWindow(hErrorTree, topLeft.x, topLeft.y, width, height, TRUE);
*/
//	RedrawWindow(hErrorTree, NULL, NULL, RDW_INVALIDATE | RDW_ERASE |
//		RDW_FRAME | RDW_ALLCHILDREN);
	if (errorCount > 0)
	{
		if (errorCount == 1)
		{
			sprintf(buf, TCLocalStrings::get("ErrorTreeOneError"));
		}
		else
		{
			sprintf(buf, TCLocalStrings::get("ErrorTreeNErrors"), errorCount);
		}
		if (warningCount > 0)
		{
			strcat(buf, ", ");
		}
	}
	if (warningCount > 0)
	{
		if (warningCount == 1)
		{
			strcat(buf, TCLocalStrings::get("ErrorTreeOneWarning"));
		}
		else
		{
			sprintf(buf + strlen(buf),
				TCLocalStrings::get("ErrorTreeNWarnings"), warningCount);
		}
	}
	SendMessage(hErrorStatusWindow, SB_SETTEXT, 0, (LPARAM)buf);
	return errorCount;
}

void ModelWindow::showErrorsIfNeeded(BOOL onlyIfNeeded)
{
	if (windowShown && !((LDViewWindow*)parentWindow)->getFullScreen())
	{
		if (!hErrorWindow)
		{
			createErrorWindow();
		}
		if (hErrorWindow)
		{
			wchar_t text[1024];

			populateErrorTree();
			GetWindowTextW(hErrorWindow, text, 1024);
			if (!onlyIfNeeded || (errorCount &&
				TCUserDefaults::longForKey(SHOW_ERRORS_KEY, 1, false)))
			{
				ShowWindow(hErrorWindow, SW_SHOWNORMAL);
			}
		}
	}
}

void ModelWindow::stopAnimation(void)
{
	if (modelViewer)
	{
		modelViewer->setRotationSpeed(0.0f);
		modelViewer->setZoomSpeed(0.0f);
	}
}

int ModelWindow::loadModel(void)
{
	char* filename = modelViewer->getFilename();
#ifdef _DEBUG
	_CrtMemState ms1, ms2, ms3;
#endif // _DEBUG

	loadCanceled = false;
	clearErrors();
	stopPolling();
	makeCurrent();
	if (strlen(filename) < 900)
	{
#ifdef TC_NO_UNICODE
		char title[1024];

		sprintf(title, "LDView: %s", filename);
		SetWindowText(parentWindow->getHWindow(), title);
#else // TC_NO_UNICODE
		LPWSTR wFilename;
		LPWSTR title;
		int bufferSize = MultiByteToWideChar(GetACP(), 0, filename, -1, NULL,
			0);
		int titleSize = bufferSize + 32;

		wFilename = new WCHAR[bufferSize];
		MultiByteToWideChar(GetACP(), 0, filename, -1, wFilename, bufferSize);
		title = new WCHAR[titleSize];
		sucprintf(title, titleSize, _UC("LDView: %s"), wFilename);
		parentWindow->setTitle(title);
		delete wFilename;
		delete title;
#endif // TC_NO_UNICODE
	}
	else
	{
		parentWindow->setTitle(_UC("LDView"));
	}
	_CrtMemCheckpoint(&ms1);
	if (modelViewer->loadModel())
	{
		_CrtMemCheckpoint(&ms2);
		_CrtMemDifference(&ms3, &ms1, &ms2);
		_CrtMemDumpStatistics(&ms3);
		forceRedraw();
		stopAnimation();
		if (pollSetting)
		{
			if (getFileTime(&lastWriteTime))
			{
				startPolling();
			}
		}
		return 1;
	}
	else
	{
		modelViewer->setFilename(NULL);
		return 0;
	}
}

void ModelWindow::closeWindow(void)
{
	destroyWindow();
	CUIOGLWindow::closeWindow();
}

LRESULT ModelWindow::doShowWindow(BOOL showFlag, LPARAM status)
{
	windowShown = true;
	return CUIOGLWindow::doShowWindow(showFlag, status);
}

BOOL ModelWindow::initWindow(void)
{
	LDVExtensionsSetup::setup(hInstance);
	if (((LDViewWindow*)parentWindow)->getFullScreen() ||
		((LDViewWindow*)parentWindow)->getScreenSaver() ||
		((LDViewWindow*)parentWindow)->getHParentWindow())
	{
		exWindowStyle &= ~WS_EX_CLIENTEDGE;
	}
	else
	{
		exWindowStyle |= WS_EX_CLIENTEDGE;
	}
	windowStyle |= WS_CHILD;
	cancelLoad = false;
	if (CUIOGLWindow::initWindow())
	{
		TREGLExtensions::setup();
		setupMultisample();
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void ModelWindow::setupMultisample(void)
{
	if (LDVExtensionsSetup::haveMultisampleExtension())
	{
		if (currentAntialiasType)
		{
			if (TREGLExtensions::haveNvMultisampleFilterHintExtension())
			{
				if (prefs->getUseNvMultisampleFilter())
				{
					glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
				}
				else
				{
					glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST);
				}
			}
			glEnable(GL_MULTISAMPLE_ARB);
		}
		else
		{
			if (TREGLExtensions::haveNvMultisampleFilterHintExtension())
			{
				glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST);
			}
			glDisable(GL_MULTISAMPLE_ARB);
		}
	}
}

LRESULT CALLBACK ModelWindow::staticErrorDlgProc(HWND hDlg, UINT message,
												 WPARAM wParam, LPARAM lParam)
{
	ModelWindow* modelWindow = (ModelWindow*)GetWindowLongPtr(hDlg,
		GWLP_USERDATA);

	if (modelWindow)
	{
		return modelWindow->errorDlgProc(hDlg, message, wParam, lParam);
	}
	else
	{
		return 0;
	}
}

LRESULT ModelWindow::errorDlgProc(HWND hDlg, UINT message, WPARAM wParam,
								  LPARAM lParam)
{
//	debugPrintf("ModelWindow::errorDlgProc: 0x%04x\n", message);
	return CallWindowProc(originalErrorDlgProc, hDlg, message, wParam, lParam);
}

LRESULT ModelWindow::windowProc(HWND hWnd, UINT message, WPARAM wParam,
							  LPARAM lParam)
{
#ifndef _NO_BOOST
	if (remoteListener && remoteMessageID != 0 && message == remoteMessageID)
	{
		processRemoteMessage((char *)lParam);
	}
#endif // !_NO_BOOST
//	debugPrintf("ModelWindow::windowProc: 0x%04x\n", message);
	return CUIOGLWindow::windowProc(hWnd, message, wParam, lParam);
}

void ModelWindow::processModalMessage(MSG msg)
{
	if (msg.message == WM_KEYDOWN)
	{
		msg.hwnd = hParentWindow;
	}
	CUIWindow::processModalMessage(msg);
}

int ModelWindow::progressCallback(
	CUCSTR message,
	float progress,
	bool fromImage,
	bool showErrors /*= false*/)
{
	DWORD thisProgressUpdate = GetTickCount();

	if (!windowShown)
	{
		return 1;
	}
	if (ucstrcmp(message, _UC("LoadingPNG")) == 0 ||
		ucstrcmp(message, _UC("LoadingPNGRow")) == 0)
	{
		// The only time we get this message is when loading built-in PNG files,
		// like toolbar icons and stud logo; don't show it in the progress.
		return 1;
	}
	if (progress == 2.0 && (!userLoad || !fromImage))
	{
		// done
		hideProgress();
		// If the window is closed during load, we will no longer be initialized.
		if (showErrors && initialized && !cancelLoad && userLoad)
		{
			showErrorsIfNeeded();
		}
		if (!fromImage)
		{
			userLoad = false;
		}
		makeCurrent();
		return 1;
	}
	if (!loading)
	{
		//clearErrors();
		loading = true;
		setupProgress();
		((LDViewWindow*)parentWindow)->forceShowStatusBar(true);
		if (!flushModal(hParentWindow, false))
		{
			PostQuitMessage(0);
			cancelLoad = true;
		}
		if (cancelLoad)
		{
			loadCanceled = true;
		}
		((LDViewWindow*)parentWindow)->setLoading(true);
	}
	if (message && message[0])
	{
		setStatusText(hStatusBar, 1, message);
	}
	if (progress >= 0.0f)
	{
		int oldProgress;
		int newProgress = (int)(progress * 100);

		oldProgress = SendMessage(hProgressBar, PBM_GETPOS, 0, 0);
		if (oldProgress != newProgress)
		{
			SendMessage(hProgressBar, PBM_SETPOS, newProgress, 0);
		}
	}
	if (thisProgressUpdate < lastProgressUpdate || thisProgressUpdate >
		lastProgressUpdate + 100 || progress == 1.0f)
	{
		if (!flushModal(hParentWindow, false))
		{
			PostQuitMessage(0);
			cancelLoad = true;
		}
		if (cancelLoad)
		{
			loadCanceled = true;
		}
		lastProgressUpdate = thisProgressUpdate;
	}
	if (cancelLoad)
	{
		hideProgress();
		makeCurrent();
		return 0;
	}
	else
	{
		makeCurrent();
		return 1;
	}
}

//bool ModelWindow::staticImageProgressCallback(CUCSTR message, float progress,
//											  void* userData)
//{
//	return ((ModelWindow*)userData)->progressCallback(message, progress) ? true
//		: false;
//}

int ModelWindow::errorCallback(LDLError* error)
{
	if (windowShown)
	{
		errors->addObject(error);
	}
	return 1;
}

LRESULT ModelWindow::doCreate(HWND hWnd, LPCREATESTRUCT lpcs)
{
	if (CUIOGLWindow::doCreate(hWnd, lpcs) == -1)
	{
		return -1;
	}
	else
	{
		DragAcceptFiles(hWnd, TRUE);
		return 0;
	}
}

LRESULT ModelWindow::doDropFiles(HDROP hDrop)
{
	char buf[1024];
	if (DragQueryFile(hDrop, 0, buf, 1024) > 0)
	{
		DragFinish(hDrop);
		((LDViewWindow*)parentWindow)->openModel(buf);
		return 0;
	}
	return 1;
}

void ModelWindow::checkForPart(void)
{
	bool isPart = false;
	char buf[MAX_PATH];
	char *filePart;

	if (GetFullPathName(modelViewer->getFilename(), sizeof(buf), buf,
		&filePart))
	{
		char partsDir[1024];

		*filePart = 0;
		stripTrailingPathSeparators(buf);
		strcpy(partsDir, LDLModel::lDrawDir());
		strcat(partsDir, "\\PARTS");
		convertStringToUpper(partsDir);
		convertStringToUpper(buf);
		replaceStringCharacter(buf, '/', '\\');
		replaceStringCharacter(partsDir, '/', '\\');
		if (strcmp(buf, partsDir) == 0)
		{
			isPart = true;
		}
		else
		{
			char shortPath[1024];

			if (GetShortPathName(partsDir, shortPath, 1024))
			{
				if (strcmp(buf, shortPath) == 0)
				{
					isPart = true;
				}
			}
		}
	}
	modelViewer->setFileIsPart(isPart);
}

bool ModelWindow::chDirFromFilename(const char* filename, char* outFilename)
{
	char buf[MAX_PATH];
	char* fileSpot;
	DWORD result = GetFullPathName(filename, MAX_PATH, buf, &fileSpot);

	if (result <= MAX_PATH && result > 0)
	{
//		if (strlen(fileSpot) < strlen(filename))
		{
			strcpy(outFilename, buf);
		}
		*fileSpot = 0;
		if (SetCurrentDirectory(buf))
		{
			return true;
		}
	}
	return false;
}

void ModelWindow::printSystemError(void)
{
#ifdef _DEBUG
	DWORD error = GetLastError();
	char* buf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf,
		0, NULL);
	_CrtDbgReport(_CRT_WARN, NULL, 0, NULL, "%s\n", buf);
	LocalFree(buf);
#endif
}

void ModelWindow::setupMaterial(void)
{
	// Don't call super.
}

void ModelWindow::setupLighting(void)
{
//	LDrawModelViewer::setPolygonOffsetFunc((GLPolygonOffsetFunc)glPolygonOffset);
	modelViewer->setup();
}

#ifndef TC_NO_UNICODE
void ModelWindow::setStatusText(HWND hStatus, int part, const char *text)
{
	((LDViewWindow*)parentWindow)->setStatusText(hStatus, part, text);
}
#endif // TC_NO_UNICODE

void ModelWindow::setStatusText(HWND hStatus, int part, CUCSTR text)
{
	((LDViewWindow*)parentWindow)->setStatusText(hStatus, part, text);
}

void ModelWindow::drawFPS(void)
{
	if (prefs->getShowsFPS())
	{
		if (((LDViewWindow*)parentWindow)->getFullScreen() || !hStatusBar)
		{
			if (currentAntialiasType)
			{
				glDisable(GL_MULTISAMPLE_ARB);
			}
			modelViewer->drawFPS(fps);
			if (currentAntialiasType)
			{
				glEnable(GL_MULTISAMPLE_ARB);
			}
		}
		else if (hStatusBar)
		{
			UCCHAR fpsString[1024] = _UC("");

			if (modelViewer->getMainTREModel())
			{
				if (fps > 0.0f)
				{
					sucprintf(fpsString, COUNT_OF(fpsString),
						TCLocalStrings::get(_UC("FPSFormat")), fps);
				}
				else
				{
					ucstrcpy(fpsString,
						TCLocalStrings::get(_UC("FPSSpinPrompt")));
				}
			}
			setStatusText(hStatusBar, 1, fpsString);
		}
	}
	else if (hStatusBar)
	{
		setStatusText(hStatusBar, 1, "");
	}
}

void ModelWindow::doPostPaint(void)
{
	checkFileForUpdates();
	if (hProgressBar)
	{
		if (!RedrawWindow(hProgressBar, NULL, NULL, RDW_ERASE | RDW_INVALIDATE
			| RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN))
		{
			debugPrintf("RedrawWindow failed!\n");
		}
	}
}

void ModelWindow::updateFPS(void)
{
	DWORD thisFrameTime;

	thisFrameTime = timeGetTime();
	numFramesSinceReference++;
	if (firstFPSPass)
	{
		fps = 0.0f;
		referenceFrameTime = thisFrameTime;
		numFramesSinceReference = 0;
		firstFPSPass = false;
	}
	else if (thisFrameTime - referenceFrameTime >= 250)
	{
		if (thisFrameTime > referenceFrameTime)
		{
			fps = 1000.0f / (TCFloat)(thisFrameTime - referenceFrameTime) *
				numFramesSinceReference;
		}
		else
		{
			fps = 0.0f;
		}
		referenceFrameTime = thisFrameTime;
		numFramesSinceReference = 0;
		firstFPSPass = false;
	}
	if (!frontBufferFPS())
	{
		drawFPS();
	}
}

bool ModelWindow::frontBufferFPS(void)
{
	if (prefs->getUseNvMultisampleFilter())
	{
		static bool checkedSwapInterval = false;
		static int swapInterval = 0;

		if (!checkedSwapInterval)
		{
			int (*wglGetSwapIntervalEXT)(void) =
				(int(*)(void))wglGetProcAddress("wglGetSwapIntervalEXT");

			checkedSwapInterval = true;
			if (wglGetSwapIntervalEXT)
			{
				swapInterval = wglGetSwapIntervalEXT();
			}
			else
			{
				swapInterval = 0;
			}
		}

		if (swapInterval > 0)
		{
			// We only want to draw the FPS text to the front buffer if vsync
			// is turned on.
			return true;
		}
	}
	return false;
}

void ModelWindow::swapBuffers(void)
{
	if (!SwapBuffers(hdc))
	{
		DWORD error = GetLastError();
		char buf[1024];

		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, 0, buf,
			1024, NULL);
		debugPrintf("swapBuffers error: %s\n", buf);
	}
	if (frontBufferFPS())
	{
		glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST);
		glDisable(GL_MULTISAMPLE_ARB);
		glDrawBuffer(GL_FRONT);
		drawFPS();
		glDrawBuffer(GL_BACK);
		glFlush();
		glEnable(GL_MULTISAMPLE_ARB);
		glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
	}
}

void ModelWindow::doPaint(void)
{
	bool needsPostRedraw = (modelViewer->getNeedsReload() ||
		modelViewer->getNeedsRecompile()) && modelViewer->getMainTREModel();

//	debugPrintf("ModelWindow::doPaint\n");
	if (offscreenActive || loading)
	{
		static bool looping = false;
		if (!looping)
		{
			RECT rect;
			POINT points[2];

			GetClientRect(hWindow, &rect);
			points[0].x = rect.left;
			points[0].y = rect.top;
			points[1].x = rect.right;
			points[1].y = rect.bottom;
			MapWindowPoints(hWindow, hParentWindow, points, 2);
			rect.left = points[0].x;
			rect.top = points[0].y;
			rect.right = points[1].x;
			rect.bottom = points[1].y;
			looping = true;
//			RedrawWindow(hWindow, NULL, NULL, RDW_ERASE/* | RDW_ERASENOW*/);
			RedrawWindow(hParentWindow, &rect, NULL, RDW_ERASE | RDW_INVALIDATE
				| RDW_ERASENOW | RDW_UPDATENOW);
			ValidateRect(hWindow, NULL);
			looping = false;
		}
		else
		{
			ValidateRect(hWindow, NULL);
		}
		return;
	}
	if (!initializedGL)
	{
		forceRedraw();
		return;
	}
	if (redrawCount > 0)
	{
		//debugPrintf("Multi-forced redraw.\n");
		forceRedraw();
		redrawCount--;
		return;
	}
	makeCurrent();
	//if (needsRecompile)
	//{
	//	needsRecompile = false;
	//	recompile();
	//}
	if (loading)
	{
		modelViewer->clear();
		swapBuffers();
		return;
	}
	forceRedraw();
	redrawRequested = false;
	modelViewer->update();

	//updateSpinRate();
	//if ((fEq(rotationSpeed, 0.0f) && fEq(modelViewer->getZoomSpeed(), 0.0f)
	//	&& fEq(modelViewer->getCameraXRotate(), 0.0f) &&
	//	fEq(modelViewer->getCameraYRotate(), 0.0f) &&
	//	fEq(modelViewer->getCameraZRotate(), 0.0f) &&
	//	fEq(modelViewer->getCameraMotion().length(), 0.0f))
	//	|| modelViewer->getPaused())
	if (!redrawRequested)
	{
		if (redrawCount == 0)
		{
			ValidateRect(hWindow, NULL);
			firstFPSPass = true;
		}
	}
	updateFPS();
	if (redrawCount > 0)
	{
		redrawCount--;
	}
	swapBuffers();
	if (needsPostRedraw)
	{
		forceRedraw();
	}
	((LDViewWindow *)parentWindow)->showStatusLatLon();
}

LRESULT ModelWindow::doNCDestroy(void)
{
	if (prefs && !applyingPrefs)
	{
		prefs->closePropertySheet(true);
		hPrefsWindow = NULL;
	}
	if (hProgressWindow)
	{
		DestroyWindow(hProgressWindow);
		hProgressWindow = NULL;
	}
	if (hErrorWindow)
	{
		DestroyWindow(hErrorWindow);
		hErrorWindow = NULL;
	}
	return CUIOGLWindow::doNCDestroy();
}

LRESULT ModelWindow::doDestroy(void)
{
	cancelLoad = true;
	openGlWillEnd();
//	RevokeDragDrop(hWindow);
	return CUIOGLWindow::doDestroy();
}

void ModelWindow::zoom(TCFloat amount)
{
	if (modelViewer)
	{
		modelViewer->zoom(amount);
		forceRedraw();
	}
}

void ModelWindow::setClipZoom(bool value)
{
	if (modelViewer)
	{
		modelViewer->setClipZoom(value);
	}
}

bool ModelWindow::getClipZoom(void)
{
	if (modelViewer)
	{
		return modelViewer->getClipZoom();
	}
	else
	{
		return false;
	}
}

void ModelWindow::startPolling(void)
{
	if (pollSetting && !pollTimerRunning && !loading)
	{
		setTimer(POLL_TIMER, POLL_INTERVAL);
		pollTimerRunning = true;
	}
}

BOOL ModelWindow::stopPolling(void)
{
	if (loading)
	{
		return TRUE;
	}
	else
	{
		pollTimerRunning = false;
		return killTimer(POLL_TIMER);
	}
}

void ModelWindow::setPollSetting(int value)
{
	if (value != pollSetting)
	{
		pollSetting = value;
		if (pollSetting)
		{
			startPolling();
		}
		else
		{
			stopPolling();
		}
	}
}

//bool ModelWindow::writeImage(char *filename, int width, int height,
//							 BYTE *buffer, char *formatName, bool saveAlpha)
//{
//	TCImage *image = new TCImage;
//	bool retValue;
//	const char *version = ((LDViewWindow *)parentWindow)->getProductVersion();
//	char comment[1024];
//
//	if (saveAlpha)
//	{
//		image->setDataFormat(TCRgba8);
//	}
//	image->setSize(width, height);
//	image->setLineAlignment(4);
//	image->setImageData(buffer);
//	image->setFormatName(formatName);
//	image->setFlipped(true);
//	if (strcasecmp(formatName, "PNG") == 0)
//	{
//		strcpy(comment, "Software:!:!:LDView");
//	}
//	else
//	{
//		strcpy(comment, "Created by LDView");
//	}
//	if (version)
//	{
//		strcat(comment, " ");
//		strcat(comment, version);
//	}
//	image->setComment(comment);
//	if (TCUserDefaults::longForKey(AUTO_CROP_KEY, 0, false))
//	{
//		image->autoCrop((BYTE)modelViewer->getBackgroundR(),
//			(BYTE)modelViewer->getBackgroundG(),
//			(BYTE)modelViewer->getBackgroundB());
//	}
//	retValue = image->saveFile(filename, staticImageProgressCallback, this);
//	image->release();
//	return retValue;
//}

//bool ModelWindow::writeBmp(char *filename, int width, int height, BYTE *buffer)
//{
//	return writeImage(filename, width, height, buffer, "BMP");
//}
//
//bool ModelWindow::writePng(char *filename, int width, int height, BYTE *buffer,
//						   bool saveAlpha)
//{
//	return writeImage(filename, width, height, buffer, "PNG", saveAlpha);
//}

bool ModelWindow::setupBitmapRender(int imageWidth, int imageHeight)
{
	hBitmapRenderDC = CreateCompatibleDC(NULL);
	BYTE *bmBuffer;

	if (!hBitmapRenderDC)
	{
		return false;
	}
	hRenderBitmap = createDIBSection(hBitmapRenderDC, imageWidth, imageHeight,
		96, 96, &bmBuffer);
	if (hRenderBitmap)
	{
		PIXELFORMATDESCRIPTOR pfd;
		int pfIndex;

		memset(&pfd, 0, sizeof(pfd));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cDepthBits = 32;
		pfd.cStencilBits = 2;
		pfd.cAlphaBits = 8;
		SelectObject(hBitmapRenderDC, hRenderBitmap);
		pfIndex = ChoosePixelFormat(hBitmapRenderDC, &pfd);
		if (pfIndex)
		{
			if (SetPixelFormat(hBitmapRenderDC, pfIndex, &pfd))
			{
				hBitmapRenderGLRC = wglCreateContext(hBitmapRenderDC);
				if (hBitmapRenderGLRC)
				{
					openGlWillEnd();
					TREGLExtensions::disableAll(true);
					hCurrentDC = hBitmapRenderDC;
					hCurrentGLRC = hBitmapRenderGLRC;
					wglMakeCurrent(hBitmapRenderDC, hBitmapRenderGLRC);
					setupMaterial();
					setupLighting();
					glDepthFunc(GL_LEQUAL);
					glEnable(GL_DEPTH_TEST);
					glDrawBuffer(GL_FRONT);
					glReadBuffer(GL_FRONT);
					modelViewer->setWidth(imageWidth);
					modelViewer->setHeight(imageHeight);
					modelViewer->setup();
					modelViewer->pause();
					modelViewer->setNeedsRecompile();
					return true;
				}
			}
		}
		DeleteObject(hRenderBitmap);
	}
	DeleteDC(hBitmapRenderDC);
	hBitmapRenderDC = NULL;
	return false;
}

bool ModelWindow::setupOffscreen(
	int imageWidth,
	int imageHeight,
	bool antialias)
{
	if (snapshotTaker != NULL && snapshotTaker->getUseFBO())
	{
		if (saveImageType == PNG_IMAGE_TYPE_INDEX &&
			TCUserDefaults::boolForKey(HDR_SNAPSHOTS_KEY))
		{
			snapshotTaker->set16BPC(true);
		}
		return true;
	}
	else if (setupPBuffer(imageWidth, imageHeight, antialias))
	{
		return true;
	}
	else
	{
		return setupBitmapRender(imageWidth, imageHeight);
	}
}

bool ModelWindow::setupPBuffer(int imageWidth, int imageHeight,
							   bool antialias)
{
	if (LDVExtensionsSetup::havePixelBufferExtension())
	{
		GLint intValues[] = {
			WGL_DRAW_TO_PBUFFER_ARB, GL_TRUE,
			WGL_RED_BITS_ARB, 8,
			WGL_GREEN_BITS_ARB, 8,
			WGL_BLUE_BITS_ARB, 8,
			WGL_ALPHA_BITS_ARB, 8,
			WGL_STENCIL_BITS_ARB, 2,
			0, 0,
			0, 0,
			0, 0
		};
		int index;

		if (antialias)
		{
			int offset = sizeof(intValues) / sizeof(GLint) - 6;

			intValues[offset++] = WGL_SAMPLES_EXT;
			intValues[offset++] = prefs->getFSAAFactor();
			intValues[offset++] = WGL_SAMPLE_BUFFERS_EXT;
			intValues[offset++] = GL_TRUE;
		}
		index = LDVExtensionsSetup::choosePixelFormat(hdc, intValues);
		if (index >= 0)
		{
			using namespace TREGLExtensionsNS;

			if (wglCreatePbufferARB && wglGetPbufferDCARB &&
				wglGetPixelFormatAttribivARB)
			{
				int pfSizeAttribs[3] = {
					WGL_MAX_PBUFFER_WIDTH_ARB,
					WGL_MAX_PBUFFER_HEIGHT_ARB,
					WGL_MAX_PBUFFER_PIXELS_ARB
				};
				int attribValues[3];

				wglGetPixelFormatAttribivARB(hdc, index, 0, 3, pfSizeAttribs,
					attribValues);
				// This shouldn't be necessary, but ATI returns a PBuffer even
				// if we ask for one that is too big, so we can't rely on their
				// failure to trigger failure.  The one it returns CLAIMS to be
				// the size we asked for; it just doesn't work right.
				if (attribValues[0] >= imageWidth &&
					attribValues[1] >= imageHeight &&
					attribValues[2] >= imageWidth * imageHeight)
				{
					// Given the above check, the following shouldn't really
					// matter, but I'll leave it in anyway.
					GLint intValues[] = { 
						WGL_PBUFFER_LARGEST_ARB, 0,
						0, 0
					};
					hPBuffer = wglCreatePbufferARB(hdc, index, imageWidth,
						imageHeight, intValues);

					if (hPBuffer)
					{
						hPBufferDC = wglGetPbufferDCARB(hPBuffer);
						if (hPBufferDC)
						{
							hPBufferGLRC = wglCreateContext(hPBufferDC);

							if (hPBufferGLRC)
							{
								wglShareLists(hglrc, hPBufferGLRC);
								hCurrentDC = hPBufferDC;
								hCurrentGLRC = hPBufferGLRC;
								makeCurrent();
								if (antialias)
								{
									setupMultisample();
								}
								setupMaterial();
								setupLighting();
								glDepthFunc(GL_LEQUAL);
								glEnable(GL_DEPTH_TEST);
								glDrawBuffer(GL_FRONT);
								glReadBuffer(GL_FRONT);
								modelViewer->setWidth(imageWidth);
								modelViewer->setHeight(imageHeight);
								modelViewer->setup();
								modelViewer->pause();
								if (modelViewer->getNeedsReload())
								{
									return true;
								}
								else
								{
									// No need to recompile as before, because
									// we're sharing display lists.
									return true;
								}
							}
						}
					}
				}
			}
		}
		cleanupOffscreen();
		if (antialias)
		{
			return setupPBuffer(imageWidth, imageHeight, false);
		}
	}
	return false;
}

void ModelWindow::renderOffscreenImage(void)
{
	makeCurrent();
	modelViewer->update();
	//if (canSaveAlpha())
	//{
	//	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT |
	//		GL_VIEWPORT_BIT);
	//	CUIOGLWindow::orthoView();
	//	glColor4ub(0, 0, 0, 255);
	//	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
	//	glEnable(GL_DEPTH_TEST);
	//	glDisable(GL_LIGHTING);
	//	glDisable(GL_POLYGON_OFFSET_FILL);
	//	glDepthFunc(GL_GREATER);
	//	glDepthRange(0.0f, 1.0f);
	//	glBegin(GL_QUADS);
	//		treGlVertex3f(0.0f, 0.0f, -1.0f);
	//		treGlVertex3f((TCFloat)width, 0.0f, -1.0f);
	//		treGlVertex3f((TCFloat)width, (TCFloat)height, -1.0f);
	//		treGlVertex3f(0.0f, (TCFloat)height, -1.0f);
	//	glEnd();
	//	glPopAttrib();
	//}
}

void ModelWindow::cleanupRenderSettings(void)
{
	if (!savingFromCommandLine)
	{
		// If we're saving from the command line, there's no need to
		// put things back for regular rendering (particularly
		// recompiling the model, which takes quite a bit of extra
		// time.
		makeCurrent();
		modelViewer->setWidth(width);
		modelViewer->setHeight(height);
		//modelViewer->recompile();
		modelViewer->unpause();
		modelViewer->setup();
	}
}

void ModelWindow::cleanupOffscreen(void)
{
	hCurrentDC = hdc;
	hCurrentGLRC = hglrc;
	if (hPBuffer)
	{
		cleanupPBuffer();
	}
	else if (hBitmapRenderGLRC)
	{
		cleanupBitmapRender();
	}
	cleanupRenderSettings();
}

void ModelWindow::cleanupBitmapRender(void)
{
	if (hBitmapRenderGLRC)
	{
		openGlWillEnd();
		wglDeleteContext(hBitmapRenderGLRC);
		hBitmapRenderGLRC = NULL;
	}
	if (hRenderBitmap)
	{
		DeleteObject(hRenderBitmap);
		hRenderBitmap = NULL;
	}
	if (hBitmapRenderDC)
	{
		DeleteDC(hBitmapRenderDC);
		hBitmapRenderDC = NULL;
	}
	TREGLExtensions::disableAll(false);
	modelViewer->setNeedsRecompile();
}

void ModelWindow::cleanupPBuffer(void)
{
	if (hPBuffer)
	{
		using namespace TREGLExtensionsNS;

		if (hPBufferDC)
		{
			if (hPBufferGLRC)
			{
				wglDeleteContext(hPBufferGLRC);
				hPBufferGLRC = NULL;
			}
			wglReleasePbufferDCARB(hPBuffer, hPBufferDC);
			hPBufferDC = NULL;
		}
		wglDestroyPbufferARB(hPBuffer);
		hPBuffer = NULL;
	}
}

//bool ModelWindow::canSaveAlpha(void)
//{
//	if (saveAlpha && saveImageType == PNG_IMAGE_TYPE_INDEX)
//	{
//		int alphaBits;
//
//		glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
//		return alphaBits > 0;
//	}
//	return false;
//}

void ModelWindow::setupSnapshotBackBuffer(int imageWidth, int imageHeight,
										  RECT &rect)
{
	makeCurrent();
	modelViewer->setSlowClear(true);
	GetWindowRect(hParentWindow, &rect);
	MoveWindow(hParentWindow, 0, 0, rect.right - rect.left,
		rect.bottom - rect.top, TRUE);
	modelViewer->setWidth(imageWidth);
	modelViewer->setHeight(imageHeight);
	modelViewer->setup();
	glReadBuffer(GL_BACK);
}

void ModelWindow::cleanupSnapshotBackBuffer(RECT &rect)
{
	MoveWindow(hParentWindow, rect.left, rect.top, rect.right - rect.left,
		rect.bottom - rect.top, TRUE);
	RedrawWindow(hParentWindow, NULL, NULL,
		RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
	modelViewer->setWidth(width);
	modelViewer->setHeight(height);
	modelViewer->setup();
}

LDSnapshotTaker::ImageType ModelWindow::getSaveImageType(void)
{
	switch (saveImageType)
	{
	case PNG_IMAGE_TYPE_INDEX:
		return LDSnapshotTaker::ITPng;
	case BMP_IMAGE_TYPE_INDEX:
		return LDSnapshotTaker::ITBmp;
	case JPG_IMAGE_TYPE_INDEX:
		return LDSnapshotTaker::ITJpg;
	case SVG_IMAGE_TYPE_INDEX:
		return LDSnapshotTaker::ITSvg;
	case EPS_IMAGE_TYPE_INDEX:
		return LDSnapshotTaker::ITEps;
	case PDF_IMAGE_TYPE_INDEX:
		return LDSnapshotTaker::ITPdf;
	default:
		return LDSnapshotTaker::ITPng;
	}
}

void ModelWindow::grabSetup(
	int &imageWidth,
	int &imageHeight,
	RECT &origRect,
	bool &origSlowClear)
{
	currentAntialiasType = TCUserDefaults::longForKey(FSAA_MODE_KEY);
	int newWidth = 1600;
	int newHeight = 1200;
	int numXTiles, numYTiles;

	memset(&origRect, 0, sizeof(origRect));
	origSlowClear = modelViewer->getSlowClear();
	offscreenActive = true;
	snapshotTaker->calcTiling(imageWidth, imageHeight, newWidth, newHeight,
		numXTiles, numYTiles);
	if (!setupOffscreen(newWidth, newHeight, currentAntialiasType > 0))
	{
		newWidth = width;		// width is OpenGL window width
		newHeight = height;		// height is OpenGL window height
		snapshotTaker->calcTiling(imageWidth, imageHeight, newWidth, newHeight,
			numXTiles, numYTiles);
		setupSnapshotBackBuffer(newWidth, newHeight, origRect);
	}
}

void ModelWindow::grabCleanup(RECT origRect, bool origSlowClear)
{
	if (snapshotTaker->getUseFBO())
	{
		snapshotTaker->set16BPC(false);
		cleanupRenderSettings();
	}
	else if (hPBuffer || hBitmapRenderGLRC)
	{
		cleanupOffscreen();
	}
	else
	{
		cleanupSnapshotBackBuffer(origRect);
	}
	offscreenActive = false;
	modelViewer->setSlowClear(origSlowClear);
}

bool ModelWindow::saveImage(
	char *filename,
	int imageWidth,
	int imageHeight,
	bool zoomToFit)
{
	RECT origRect;
	bool origSlowClear;
	bool retValue = false;

	if (!snapshotTaker)
	{
		snapshotTaker =  new LDSnapshotTaker(modelViewer);
		// Only try FBO if the user hasn't unchecked the "Use Pixel Buffer"
		// check box.  Note that snapshotTaker will also check that the
		// FBO extension is available before using it.
		if (LDVExtensionsSetup::havePixelBufferExtension())
		{
			snapshotTaker->setUseFBO(true);
		}
	}
	snapshotTaker->setImageType(getSaveImageType());
	snapshotTaker->setTrySaveAlpha(saveAlpha);
	snapshotTaker->setAutoCrop(autoCrop);
	snapshotTaker->setProductVersion(
		((LDViewWindow *)parentWindow)->getProductVersion());
	grabSetup(imageWidth, imageHeight, origRect, origSlowClear);
	retValue = snapshotTaker->saveImage(filename, imageWidth, imageHeight,
		zoomToFit);
	grabCleanup(origRect, origSlowClear);
	return retValue;
}

BOOL ModelWindow::doDialogInit(HWND hDlg, HWND /*hFocusWindow*/,
							   LPARAM /*lParam*/)
{
	if (hDlg == hPrintDialog)
	{
		setupPrintExtras();
	}
	else if (hDlg == hSaveDialog)
	{
		setupSaveExtras();
	}
	else if (hDlg == hPageSetupDialog)
	{
		setupPageSetupExtras();
	}
	return TRUE;
}

UINT_PTR CALLBACK ModelWindow::staticPrintHook(
	HWND hDlg,
	UINT uiMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	ModelWindow* modelWindow = (ModelWindow*)GetWindowLongPtr(hDlg,
		GWLP_USERDATA);

	if (uiMsg == WM_INITDIALOG)
	{
		modelWindow = (ModelWindow*)((PRINTDLG*)lParam)->lCustData;
		if (modelWindow)
		{
			modelWindow->hPrintDialog = hDlg;
			SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)modelWindow);
		}
	}
	if (modelWindow)
	{
		return modelWindow->dialogProc(hDlg, uiMsg, wParam, lParam);
	}
	else
	{
		return 0;
	}
}

bool ModelWindow::selectPrinter(PRINTDLG &pd)
{
	HGLOBAL hDevMode = GlobalAlloc(GHND, sizeof(DEVMODE));
	DEVMODE *devMode = (DEVMODE *)GlobalLock(hDevMode);
	bool retValue;

	devMode->dmFields = DM_ORIENTATION | DM_PAPERSIZE;
	devMode->dmOrientation = (short)printOrientation;
	devMode->dmPaperSize = (short)printPaperSize;
	GlobalUnlock(hDevMode);
	// Initialize the PRINTDLG members. 
 	pd.lStructSize = sizeof(PRINTDLG);
	pd.hDevMode = hDevMode;
	pd.hDevNames = NULL;
	pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION;// |
//		PD_ENABLEPRINTHOOK | PD_ENABLEPRINTTEMPLATE;
	pd.hwndOwner = NULL;//parentWindow->getHWindow();
	pd.hDC = NULL;
	pd.nFromPage = 1;
	pd.nToPage = 1;
	pd.nMinPage = 0;
	pd.nMaxPage = 0;
	pd.nCopies = 1;
	pd.hInstance = getLanguageModule();
	pd.lCustData = (long)this;
	pd.lpfnPrintHook = staticPrintHook;
	pd.lpfnSetupHook = NULL;
	pd.lpPrintTemplateName = MAKEINTRESOURCE(PRINTDLGORD);
	pd.lpSetupTemplateName = NULL;
	pd.hPrintTemplate = NULL;
	pd.hSetupTemplate = NULL;
 
	// Display the PRINT dialog box. 
 
	if (PrintDlg(&pd))
	{
		hPrintDialog = NULL;
		retValue = true;
	}
	else
	{
//		DWORD error = CommDlgExtendedError();
		hPrintDialog = NULL;
		retValue = false;
	}
	GlobalFree(hDevMode);
	return retValue;
}

void ModelWindow::swap(int &left, int &right)
{
	int temp = left;

	left = right;
	right = temp;
}

void ModelWindow::calcTiling(int desiredWidth, int desiredHeight,
							 int &bitmapWidth, int &bitmapHeight,
							 int &numXTiles, int &numYTiles)
{
	if (desiredWidth > bitmapWidth)
	{
		numXTiles = (desiredWidth + bitmapWidth - 1) / bitmapWidth;
	}
	else
	{
		numXTiles = 1;
	}
	bitmapWidth = desiredWidth / numXTiles;
	if (desiredHeight > bitmapHeight)
	{
		numYTiles = (desiredHeight + bitmapHeight - 1) / bitmapHeight;
	}
	else
	{
		numYTiles = 1;
	}
	bitmapHeight = desiredHeight / numYTiles;
}

void ModelWindow::calcTiling(HDC hPrinterDC, int &bitmapWidth,
							 int &bitmapHeight, int &numXTiles, int &numYTiles)
{
	int hRes;
	int vRes;
	int printXOffset;
	int printYOffset;
	int printWidth;
	int printHeight;
	int printRMarginPixels;
	int printBMarginPixels;
	int maxBitmapWidth = bitmapWidth;
	int maxBitmapHeight = bitmapHeight;

	printHDPI = GetDeviceCaps(hPrinterDC, LOGPIXELSX);
	printVDPI = GetDeviceCaps(hPrinterDC, LOGPIXELSY);
	printXOffset = GetDeviceCaps(hPrinterDC, PHYSICALOFFSETX);
	printYOffset = GetDeviceCaps(hPrinterDC, PHYSICALOFFSETY);
	printWidth = GetDeviceCaps(hPrinterDC, PHYSICALWIDTH);
	printHeight = GetDeviceCaps(hPrinterDC, PHYSICALHEIGHT);
	hRes = GetDeviceCaps(hPrinterDC, HORZRES);
	vRes = GetDeviceCaps(hPrinterDC, VERTRES);
	printLMarginPixels = (int)(printLeftMargin * printHDPI) - printXOffset;
	printRMarginPixels = (int)(printRightMargin * printHDPI) + hRes -
		printWidth + printXOffset;
	printTMarginPixels = (int)(printTopMargin * printVDPI) - printYOffset;
	printBMarginPixels = (int)(printBottomMargin * printVDPI) + vRes -
		printHeight + printYOffset;
	bitmapWidth = hRes - printLMarginPixels - printRMarginPixels;
	if (bitmapWidth > maxBitmapWidth)
	{
		numXTiles = (bitmapWidth + maxBitmapWidth - 1) / maxBitmapWidth;
	}
	else
	{
		numXTiles = 1;
	}
	bitmapWidth = bitmapWidth / numXTiles;
	bitmapHeight = vRes - printTMarginPixels - printBMarginPixels;
	if (bitmapHeight > maxBitmapHeight)
	{
		numYTiles = (bitmapHeight + maxBitmapHeight - 1) / maxBitmapHeight;
	}
	else
	{
		numYTiles = 1;
	}
	bitmapHeight = bitmapHeight / numYTiles;
}

bool ModelWindow::printPage(const PRINTDLG &pd)
{
	bool retValue = false;
	HDC hBitmapDC = CreateCompatibleDC(pd.hDC);

	if (hBitmapDC)
	{
		int bitmapWidth = 1600;
		int bitmapHeight = 1200;
		int renderWidth;
		int renderHeight;
		int numXTiles;
		int numYTiles;
		bool oldSlowClear = modelViewer->getSlowClear();
		int oldR = modelViewer->getBackgroundR();
		int oldG = modelViewer->getBackgroundG();
		int oldB = modelViewer->getBackgroundB();
		int oldA = modelViewer->getBackgroundA();
		LDVStereoMode oldStereoMode = modelViewer->getStereoMode();
		BYTE *bmBuffer = NULL;
		HBITMAP hBitmap = NULL;
		bool canceled = false;
		bool landscape = printOrientation == DMORIENT_LANDSCAPE;

		calcTiling(pd.hDC, bitmapWidth, bitmapHeight, numXTiles, numYTiles);
		if (landscape)
		{
			renderWidth = bitmapHeight;
			renderHeight = bitmapWidth;
			swap(numXTiles, numYTiles);
		}
		else
		{
			renderWidth = bitmapWidth;
			renderHeight = bitmapHeight;
		}
		offscreenActive = true;
		if (printHDPI != printVDPI)
		{
			modelViewer->setPixelAspectRatio((float)printHDPI / printVDPI);
		}
		if (printBackground)
		{
			modelViewer->setBackgroundRGBA(oldR, oldG, oldB, oldA);
		}
		else
		{
			modelViewer->setBackgroundRGBA(255, 255, 255, 255);
		}
		modelViewer->setStereoMode(LDVStereoNone);
		if (!setupOffscreen(renderWidth, renderHeight))
		{
			if (!modelViewer->getCompiled())
			{
				canceled = true;
			}
			else
			{
				if (landscape)
				{
					bitmapWidth = height;
					bitmapHeight = width;
					swap(numXTiles, numYTiles);
				}
				else
				{
					bitmapWidth = width;
					bitmapHeight = height;
				}
				calcTiling(pd.hDC, bitmapWidth, bitmapHeight, numXTiles,
					numYTiles);
				if (landscape)
				{
					renderWidth = bitmapHeight;
					renderHeight = bitmapWidth;
				}
				else
				{
					renderWidth = bitmapWidth;
					renderHeight = bitmapHeight;
				}
				modelViewer->setWidth(renderWidth);
				modelViewer->setHeight(renderHeight);
				makeCurrent();
				glReadBuffer(GL_BACK);
				modelViewer->setSlowClear(true);
			}
		}
		if (!canceled)
		{
			hBitmap = createDIBSection(hBitmapDC, bitmapWidth, bitmapHeight,
				printHDPI, printVDPI, &bmBuffer);
		}
		if (hBitmap)
		{
			int xTile, yTile;
			int renderLineSize = roundUp(renderWidth * 3, 4);
			BYTE *buffer = new BYTE[renderLineSize * renderHeight];
			TCFloat32 oldHighlightLineWidth =
				modelViewer->getHighlightLineWidth();
			TCFloat32 oldWireframeLineWidth =
				modelViewer->getWireframeLineWidth();

			modelViewer->setNumXTiles(numXTiles);
			modelViewer->setNumYTiles(numYTiles);
			if (landscape)
			{
				swap(numXTiles, numYTiles);
			}
			modelViewer->setHighlightLineWidth(printHDPI / 100 *
				oldHighlightLineWidth);
			modelViewer->setWireframeLineWidth(printHDPI / 100 *
				oldWireframeLineWidth);
			for (yTile = 0; yTile < numYTiles && !canceled; yTile++)
			{
				if (landscape)
				{
					modelViewer->setXTile(yTile);
				}
				else
				{
					modelViewer->setYTile(yTile);
				}
				progressCallback(TCLocalStrings::get(_UC("PrintingModel")),
					0.0f, false);
				for (xTile = 0; xTile < numXTiles && !canceled; xTile++)
				{
					int x, y;

					if (progressCallback((CUCSTR)NULL, (float)(yTile *
						numXTiles + xTile) / (numYTiles * numXTiles), false))
					{
						if (landscape)
						{
							modelViewer->setYTile(numXTiles - xTile - 1);
						}
						else
						{
							modelViewer->setXTile(xTile);
						}
						renderOffscreenImage();
						glReadPixels(0, 0, renderWidth, renderHeight, GL_RGB,
							GL_UNSIGNED_BYTE, buffer);
						if (landscape)
						{
							int bitmapLineSize = roundUp(bitmapWidth * 3, 4);

							for (y = 0; y < renderHeight; y++)
							{
								int offset = y * renderLineSize;
								for (x = 0; x < renderWidth; x++)
								{
									int renderSpot = offset + x * 3;
									int bitmapSpot = (renderWidth - x - 1)
										* bitmapLineSize + y * 3;

									bmBuffer[bitmapSpot] =
										buffer[renderSpot + 2];
									bmBuffer[bitmapSpot + 1] =
										buffer[renderSpot + 1];
									bmBuffer[bitmapSpot + 2] =
										buffer[renderSpot];
								}
							}
						}
						else
						{
							for (y = 0; y < bitmapHeight; y++)
							{
								int offset = y * renderLineSize;

								for (x = 0; x < bitmapWidth; x++)
								{
									int spot = offset + x * 3;

									bmBuffer[spot] = buffer[spot + 2];
									bmBuffer[spot + 1] = buffer[spot + 1];
									bmBuffer[spot + 2] = buffer[spot];
								}
							}
						}
						SelectObject(hBitmapDC, hBitmap);
						if (BitBlt(pd.hDC,
							printLMarginPixels + xTile * bitmapWidth,
							printTMarginPixels + yTile * bitmapHeight,
							bitmapWidth, bitmapHeight, hBitmapDC, 0, 0, 
							SRCCOPY))
						{
							retValue = true;
						}
					}
					else
					{
						canceled = true;
						retValue = false;
					}
				}
			}
			progressCallback((CUCSTR)NULL, 1.0f, false);
			DeleteObject(hBitmap);
			modelViewer->setHighlightLineWidth(oldHighlightLineWidth);
			modelViewer->setWireframeLineWidth(oldWireframeLineWidth);
			modelViewer->setXTile(0);
			modelViewer->setYTile(0);
			modelViewer->setNumXTiles(1);
			modelViewer->setNumYTiles(1);
			delete buffer;
			progressCallback((CUCSTR)NULL, 2.0f, false);
		}
		if (hPBuffer)
		{
			cleanupOffscreen();
		}
		else
		{
			modelViewer->setWidth(width);
			modelViewer->setHeight(height);
			modelViewer->setSlowClear(oldSlowClear);
		}
		modelViewer->setBackgroundRGBA(oldR, oldG, oldB, oldA);
		modelViewer->setStereoMode(oldStereoMode);
		modelViewer->setPixelAspectRatio(1.0f);
		DeleteDC(hBitmapDC);
	}
	offscreenActive = false;
	return retValue;
}

bool ModelWindow::parseDevMode(HGLOBAL hDevMode)
{
	DEVMODE *devMode = (DEVMODE *)GlobalLock(hDevMode);
	bool retValue = true;

	if (devMode)
	{
		if (devMode->dmOrientation != printOrientation)
		{
			printOrientation = devMode->dmOrientation;
			TCUserDefaults::setLongForKey(printOrientation, ORIENTATION_KEY, false);
		}
		if (devMode->dmPaperSize != printPaperSize)
		{
			if (devMode->dmPaperSize < DMPAPER_USER)
			{
				printPaperSize = devMode->dmPaperSize;
				TCUserDefaults::setLongForKey(printPaperSize, PAPER_SIZE_KEY,
					false);
			}
			else
			{
				retValue = false;
			}
		}
		GlobalUnlock(hDevMode);
	}
	return retValue;
}

UINT_PTR CALLBACK ModelWindow::staticPageSetupHook(
	HWND hDlg,
	UINT uiMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	ModelWindow* modelWindow = (ModelWindow*)GetWindowLongPtr(hDlg,
		GWLP_USERDATA);

	if (uiMsg == WM_INITDIALOG)
	{
		modelWindow = (ModelWindow*)((PAGESETUPDLG*)lParam)->lCustData;
		if (modelWindow)
		{
			modelWindow->hPageSetupDialog = hDlg;
			SetWindowLong(hDlg, GWLP_USERDATA, (LONG_PTR)modelWindow);
		}
	}
	if (modelWindow)
	{
		return modelWindow->dialogProc(hDlg, uiMsg, wParam, lParam);
	}
	else
	{
		return 0;
	}
}

bool ModelWindow::pageSetup(void)
{
	PAGESETUPDLG psd;
	HGLOBAL hDevMode = GlobalAlloc(GHND, sizeof(DEVMODE));
	DEVMODE *devMode = (DEVMODE *)GlobalLock(hDevMode);
	bool retValue;

	devMode->dmFields = DM_ORIENTATION | DM_PAPERSIZE;
	devMode->dmOrientation = (short)printOrientation;
	devMode->dmPaperSize = (short)printPaperSize;
	GlobalUnlock(hDevMode);
	psd.lStructSize = sizeof(PAGESETUPDLG);
	psd.hwndOwner = GetParent(hWindow);
	psd.hDevMode = hDevMode;
	psd.hDevNames = NULL;
	psd.Flags = PSD_DISABLEPRINTER | PSD_INTHOUSANDTHSOFINCHES | PSD_MARGINS |
		PSD_ENABLEPAGESETUPTEMPLATE | PSD_ENABLEPAGESETUPHOOK;
	psd.rtMargin.left = (long)(printLeftMargin * 1000);
	psd.rtMargin.right = (long)(printRightMargin * 1000);
	psd.rtMargin.top = (long)(printTopMargin * 1000);
	psd.rtMargin.bottom = (long)(printBottomMargin * 1000);
	psd.lCustData = (long)this;
	psd.hInstance = getLanguageModule();
	psd.lpfnPageSetupHook = staticPageSetupHook;
	psd.lpPageSetupTemplateName = MAKEINTRESOURCE(PAGESETUPDLGORD);

	stopAnimation();
	if (PageSetupDlg(&psd))
	{
		printLeftMargin = psd.rtMargin.left / 1000.0f;
		printRightMargin = psd.rtMargin.right / 1000.0f;
		printTopMargin = psd.rtMargin.top / 1000.0f;
		printBottomMargin = psd.rtMargin.bottom / 1000.0f;
		if (!parseDevMode(psd.hDevMode))
		{
			MessageBox(hParentWindow,
				TCLocalStrings::get("PrintCustomPaperError"),
				TCLocalStrings::get("PrintPaperSize"),
				MB_OK | MB_ICONEXCLAMATION);
		}
		TCUserDefaults::setLongForKey(psd.rtMargin.left, LEFT_MARGIN_KEY,
			false);
		TCUserDefaults::setLongForKey(psd.rtMargin.right, RIGHT_MARGIN_KEY,
			false);
		TCUserDefaults::setLongForKey(psd.rtMargin.top, TOP_MARGIN_KEY, false);
		TCUserDefaults::setLongForKey(psd.rtMargin.bottom, BOTTOM_MARGIN_KEY,
			false);
		TCUserDefaults::setLongForKey(printBackground ? 1 : 0,
			PRINT_BACKGROUND_KEY, false);
		retValue = true;
	}
	else
	{
		retValue = false;
	}
	GlobalFree(hDevMode);
	return retValue;
}

bool ModelWindow::print(void)
{
	PRINTDLG pd;
	bool retValue = false;

	stopAnimation();
	memset(&pd, 0, sizeof(pd));
	pd.lStructSize = sizeof(pd);
	if (selectPrinter(pd))
	{
		DOCINFO di;
		int printJobId;
		char *docName = ucstringtombs(parentWindow->getWindowTitle());

		parseDevMode(pd.hDevMode);
		TCUserDefaults::setLongForKey(usePrinterDPI ? 1 : 0,
			USE_PRINTER_DPI_KEY);
		TCUserDefaults::setLongForKey(printDPI, PRINT_DPI_KEY);
		TCUserDefaults::setLongForKey(printBackground ? 1 : 0,
			PRINT_BACKGROUND_KEY, false);
			
		di.cbSize = sizeof(DOCINFO);
		di.lpszDocName = docName;
		di.lpszOutput = NULL;
		di.lpszDatatype = NULL;
		di.fwType = 0;
		printJobId = StartDoc(pd.hDC, &di);
		if (printJobId > 0 && StartPage(pd.hDC) > 0)
		{
			if (printPage(pd))
			{
				retValue = true;
				EndPage(pd.hDC);
				EndDoc(pd.hDC);
			}
			else
			{
				EndPage(pd.hDC);
				AbortDoc(pd.hDC);
			}
		}
		delete docName;
	}
	return retValue;
}

void ModelWindow::disableSaveSize(void)
{
	enableSaveSize(FALSE);
}

void ModelWindow::enableSaveSize(BOOL enable /*= TRUE*/)
{
	char buf[128];

	EnableWindow(hSaveWidthLabel, enable);
	EnableWindow(hSaveWidth, enable);
	EnableWindow(hSaveHeightLabel, enable);
	EnableWindow(hSaveHeight, enable);
	EnableWindow(hSaveZoomToFitButton, enable);
	if (enable)
	{
		sprintf(buf, "%d", saveWidth);
		SendDlgItemMessage(hSaveDialog, IDC_SAVE_WIDTH, WM_SETTEXT, 0, (LPARAM)buf);
		SendDlgItemMessage(hSaveDialog, IDC_SAVE_WIDTH, EM_LIMITTEXT, 4, 0);
		sprintf(buf, "%d", saveHeight);
		SendDlgItemMessage(hSaveDialog, IDC_SAVE_HEIGHT, WM_SETTEXT, 0,
			(LPARAM)buf);
		SendDlgItemMessage(hSaveDialog, IDC_SAVE_HEIGHT, EM_LIMITTEXT, 4, 0);
		SendDlgItemMessage(hSaveDialog, IDC_SAVE_ZOOMTOFIT, BM_SETCHECK,
			saveZoomToFit ? 1 : 0, 0);
	}
	else
	{
		SendDlgItemMessage(hSaveDialog, IDC_SAVE_WIDTH, WM_SETTEXT, 0,
			(LPARAM)"");
		SendDlgItemMessage(hSaveDialog, IDC_SAVE_HEIGHT, WM_SETTEXT, 0,
			(LPARAM)"");
		SendDlgItemMessage(hSaveDialog, IDC_SAVE_ZOOMTOFIT, BM_SETCHECK, 0, 0);
	}
}

void ModelWindow::disableSaveSeries(void)
{
	enableSaveSeries(FALSE);
}

void ModelWindow::enableSaveSeries(BOOL enable /*= TRUE*/)
{
	EnableWindow(hSaveDigitsLabel, enable);
	EnableWindow(hSaveDigitsField, enable);
	EnableWindow(hSaveDigitsSpin, enable);
	if (enable)
	{
		SendDlgItemMessage(hSaveDialog, IDC_SAVE_DIGITS_SPIN, UDM_SETPOS, 0,
			MAKELONG(saveDigits, 0));
	}
	else
	{
		SendDlgItemMessage(hSaveDialog, IDC_SAVE_DIGITS, WM_SETTEXT, 0,
			(LPARAM)"");
	}
}

void ModelWindow::disableSaveAllSteps(void)
{
	enableSaveAllSteps(FALSE);
}

void ModelWindow::enableSaveAllSteps(BOOL enable /*= TRUE*/)
{
	BOOL sameScaleEnable = enable && !saveActualSize && saveZoomToFit;

	EnableWindow(hSaveStepSuffixLabel, enable);
	EnableWindow(hSaveStepSuffixField, enable);
	EnableWindow(hSaveStepsSameScaleButton, sameScaleEnable);
	if (enable)
	{
		sendDlgItemMessageUC(hSaveDialog, IDC_STEP_SUFFIX, WM_SETTEXT, 0,
			(LPARAM)saveStepSuffix);
	}
	else
	{
		SendDlgItemMessage(hSaveDialog, IDC_STEP_SUFFIX, WM_SETTEXT, 0,
			(LPARAM)"");
	}
	if (sameScaleEnable)
	{
		SendDlgItemMessage(hSaveDialog, IDC_SAME_SCALE, BM_SETCHECK,
			saveStepsSameScale ? 1 : 0, 0);
	}
	else
	{
		SendDlgItemMessage(hSaveDialog, IDC_SAME_SCALE, BM_SETCHECK, 0, 0);
	}
}

void ModelWindow::updatePrintDPIField(void)
{
	if (usePrinterDPI)
	{
		EnableWindow(hPrintDPI, FALSE);
		SendDlgItemMessage(hPrintDialog, IDC_PRINT_DPI, WM_SETTEXT, 0,
			(LPARAM)"");
	}
	else
	{
		char buf[128];

		sprintf(buf, "%d", printDPI);
		EnableWindow(hPrintDPI, TRUE);
		SendDlgItemMessage(hPrintDialog, IDC_PRINT_DPI, WM_SETTEXT, 0,
			(LPARAM)buf);
	}
}

void ModelWindow::setupPrintExtras(void)
{
	HWND hIconWindow = GetDlgItem(hPrintDialog, 1086);
	RECT iconRect;
	POINT iconPoint;

	hPrintDPI = GetDlgItem(hPrintDialog, IDC_PRINT_DPI);
	GetWindowRect(hIconWindow, &iconRect);
	iconPoint.x = iconRect.left;
	iconPoint.y = iconRect.top;
	ScreenToClient(hPrintDialog, &iconPoint);
	MoveWindow(hIconWindow, iconPoint.x, iconPoint.y, 114, 36, TRUE);
	SendDlgItemMessage(hPrintDialog, IDC_PRINT_PRINTER_DPI, BM_SETCHECK,
		usePrinterDPI ? 1 : 0, 0);
	SendDlgItemMessage(hPrintDialog, IDC_PRINT_SPECIFY_DPI, BM_SETCHECK,
		usePrinterDPI ? 0 : 1, 0);
	SendDlgItemMessage(hPrintDialog, IDC_PRINT_BACKGROUND, BM_SETCHECK,
		printBackground ? 1 : 0, 0);
	SendDlgItemMessage(hPrintDialog, IDC_PRINT_DPI, EM_LIMITTEXT, 4, 0);
	updatePrintDPIField();
}

void ModelWindow::setupPageSetupExtras(void)
{
	SendDlgItemMessage(hPageSetupDialog, IDC_PRINT_BACKGROUND, BM_SETCHECK,
		printBackground ? 1 : 0, 0);
}

void ModelWindow::updateSaveSizeEnabled(void)
{
	if (saveActualSize)
	{
		disableSaveSize();
	}
	else
	{
		enableSaveSize();
	}
}

void ModelWindow::updateSaveSeriesEnabled(void)
{
	if (saveSeries)
	{
		enableSaveSeries();
	}
	else
	{
		disableSaveSeries();
	}
}

void ModelWindow::updateSaveAllStepsEnabled(void)
{
	if (saveAllSteps)
	{
		enableSaveAllSteps();
	}
	else
	{
		disableSaveAllSteps();
	}
}

void ModelWindow::saveTypeUpdated(void)
{
	switch (curSaveOp)
	{
	case LDPreferences::SOExport:
		saveExportTypeUpdated();
		break;
	case LDPreferences::SOSnapshot:
	default:
		saveImageTypeUpdated();
		break;
	}
}

void ModelWindow::saveImageTypeUpdated(void)
{
	BOOL enable = FALSE;
	WPARAM transBg = 0;

	EnableWindow(hSaveOptionsButton, saveImageType == JPG_IMAGE_TYPE_INDEX);
	if (saveImageType == PNG_IMAGE_TYPE_INDEX)
	{
		enable = TRUE;
		transBg = saveAlpha ? 1 : 0;
	}
	SendMessage(hSaveTransBgButton, BM_SETCHECK, transBg, 0);
	EnableWindow(hSaveTransBgButton, enable);
}

void ModelWindow::saveExportTypeUpdated(void)
{
	BOOL enable = FALSE;
	const LDExporter *exporter;

	exporter = modelViewer->getExporter(
		(LDrawModelViewer::ExportType)saveExportType);
	if (exporter != NULL && exporter->getSettings().size() > 0)
	{
		enable = TRUE;
	}
	EnableWindow(hSaveOptionsButton, enable);
}

void ModelWindow::setupSaveExtras(void)
{
	hSaveOptionsButton = GetDlgItem(hSaveDialog, IDC_SAVE_OPTIONS);
	hSaveTransBgButton = GetDlgItem(hSaveDialog, IDC_TRANSPARENT_BACKGROUND);
	hSaveWidthLabel = GetDlgItem(hSaveDialog, IDC_SAVE_WIDTH_LABEL);
	hSaveWidth = GetDlgItem(hSaveDialog, IDC_SAVE_WIDTH);
	hSaveHeightLabel = GetDlgItem(hSaveDialog, IDC_SAVE_HEIGHT_LABEL);
	hSaveHeight = GetDlgItem(hSaveDialog, IDC_SAVE_HEIGHT);
	hSaveZoomToFitButton = GetDlgItem(hSaveDialog, IDC_SAVE_ZOOMTOFIT);
	hSaveDigitsLabel = GetDlgItem(hSaveDialog, IDC_SAVE_DIGITS_LABEL);
	hSaveDigitsField = GetDlgItem(hSaveDialog, IDC_SAVE_DIGITS);
	hSaveDigitsSpin = GetDlgItem(hSaveDialog, IDC_SAVE_DIGITS_SPIN);
	hSaveStepSuffixLabel = GetDlgItem(hSaveDialog, IDC_STEP_SUFFIX_LABEL);
	hSaveStepSuffixField = GetDlgItem(hSaveDialog, IDC_STEP_SUFFIX);
	hSaveStepsSameScaleButton = GetDlgItem(hSaveDialog, IDC_SAME_SCALE);
	SendDlgItemMessage(hSaveDialog, IDC_SAVE_ACTUAL_SIZE, BM_SETCHECK,
		saveActualSize ? 0 : 1, 0);
	SendDlgItemMessage(hSaveDialog, IDC_SAVE_SERIES, BM_SETCHECK,
		saveSeries ? 1 : 0, 0);
	SendDlgItemMessage(hSaveDialog, IDC_SAVE_DIGITS_SPIN, UDM_SETRANGE32, 1, 5);
	SendDlgItemMessage(hSaveDialog, IDC_SAVE_DIGITS_SPIN, UDM_SETPOS, 0,
		MAKELONG(saveDigits, 0));
	SendDlgItemMessage(hSaveDialog, IDC_IGNORE_PBUFFER, BM_SETCHECK,
		ignorePBuffer ? 1 : 0, 0);
	SendDlgItemMessage(hSaveDialog, IDC_AUTO_CROP, BM_SETCHECK,
		autoCrop ? 1 : 0, 0);
	SendDlgItemMessage(hSaveDialog, IDC_ALL_STEPS, BM_SETCHECK,
		saveAllSteps ? 1 : 0, 0);
	updateSaveSizeEnabled();
	updateSaveSeriesEnabled();
	updateSaveAllStepsEnabled();
	saveTypeUpdated();
}

void ModelWindow::updateSaveWidth(void)
{
	if (!saveActualSize)
	{
		char buf[128];
		int temp;

		SendDlgItemMessage(hSaveDialog, IDC_SAVE_WIDTH, WM_GETTEXT, 128,
			(LPARAM)buf);
		if (sscanf(buf, "%d", &temp) == 1 && temp > 0 &&
			temp <= MAX_SNAPSHOT_WIDTH)
		{
			saveWidth = temp;
		}
		else
		{
			sprintf(buf, "%d", saveWidth);
			SendDlgItemMessage(hSaveDialog, IDC_SAVE_WIDTH, WM_SETTEXT, 0,
				(LPARAM)buf);
			MessageBeep(MB_OK);
		}
	}
}

void ModelWindow::updateSaveHeight(void)
{
	if (!saveActualSize)
	{
		char buf[128];
		int temp;

		SendDlgItemMessage(hSaveDialog, IDC_SAVE_HEIGHT, WM_GETTEXT, 128,
			(LPARAM)buf);
		if (sscanf(buf, "%d", &temp) == 1 && temp > 0 &&
			temp <= MAX_SNAPSHOT_HEIGHT)
		{
			saveHeight = temp;
		}
		else
		{
			sprintf(buf, "%d", saveHeight);
			SendDlgItemMessage(hSaveDialog, IDC_SAVE_HEIGHT, WM_SETTEXT, 0,
				(LPARAM)buf);
			MessageBeep(MB_OK);
		}
	}
}

void ModelWindow::updateStepSuffix(void)
{
	if (saveAllSteps)
	{
		std::string newSuffix;

		CUIDialog::windowGetText(hSaveDialog, IDC_STEP_SUFFIX, newSuffix);
		delete saveStepSuffix;
		saveStepSuffix = mbstoucstring(newSuffix.c_str(),
			(int)newSuffix.size());
	}
	updateSaveFilename();
	//std::string filename;
	//HWND hDlg = GetParent(hSaveDialog);

	//CUIDialog::windowGetText(hDlg, edt1, filename);
	//if (filename.size() > 0 && saveAllSteps)
	//{
	//	char *oldSuffix = ucstringtombs(saveStepSuffix);
	//	std::string newFilename;
	//	std::string newSuffix;

	//	newFilename = LDSnapshotTaker::removeStepSuffix(filename, oldSuffix);
	//	delete oldSuffix;
	//	CUIDialog::windowGetText(hSaveDialog, IDC_STEP_SUFFIX, newSuffix);
	//	newFilename = LDSnapshotTaker::addStepSuffix(newFilename, newSuffix, 1,
	//		modelViewer->getNumSteps());
	//	SendMessage(hDlg, CDM_SETCONTROLTEXT, edt1,
	//		(LPARAM)newFilename.c_str());
	//	delete saveStepSuffix;
	//	saveStepSuffix = mbstoucstring(newSuffix.c_str());
	//}
}

void ModelWindow::updateSaveFilename(void)
{
	char buf[1024];

	if (calcSaveFilename(buf, 1024))
	{
		SendMessage(GetParent(hSaveDialog), CDM_SETCONTROLTEXT, edt1,
			(LPARAM)buf);
	}
}

void ModelWindow::updateSaveDigits(void)
{
	char buf[128];
	int temp;

	SendDlgItemMessage(hSaveDialog, IDC_SAVE_DIGITS, WM_GETTEXT, sizeof(buf),
		(LPARAM)buf);
	if (sscanf(buf, "%d", &temp) == 1 && temp > 0 && temp <= 6)
	{
		saveDigits = temp;
	}
	updateSaveFilename();
}

void ModelWindow::doSaveOptionsClick(void)
{
	switch (curSaveOp)
	{
	case LDPreferences::SOExport:
		{
			LDExporter *exporter = modelViewer->getExporter();

			if (exporter != NULL)
			{
				ExportOptionsDialog *dialog = new ExportOptionsDialog(
					getLanguageModule(), exporter);

				if (dialog->doModal(hSaveDialog) != IDOK)
				{
					// Force a new exporter to be created in order to forget
					// about any settings changes that might have happened as
					// a result of a reset.
					modelViewer->getExporter((LDrawModelViewer::ExportType)0,
						true);
				}
				dialog->release();
			}
		}
		break;
	case LDPreferences::SOSnapshot:
	default:
		{
			JpegOptionsDialog *dialog = new JpegOptionsDialog(
				getLanguageModule(), hSaveDialog);

			dialog->doModal(hSaveDialog);
			dialog->release();
		}
		break;
	}
}

BOOL ModelWindow::doSaveClick(int controlId, HWND /*hControlWnd*/)
{
	switch (controlId)
	{
	case IDC_SAVE_OPTIONS:
		doSaveOptionsClick();
		break;
	case IDC_SAVE_ACTUAL_SIZE:
		saveActualSize = SendDlgItemMessage(hSaveDialog, controlId, BM_GETCHECK,
			0, 0) ? false : true;
		updateSaveSizeEnabled();
		updateSaveAllStepsEnabled();
		break;
	case IDC_SAVE_SERIES:
		saveSeries = SendDlgItemMessage(hSaveDialog, controlId, BM_GETCHECK, 0,
			0) ? true : false;
		updateSaveSeriesEnabled();
		break;
	case IDC_ALL_STEPS:
		saveAllSteps = SendDlgItemMessage(hSaveDialog, controlId, BM_GETCHECK,
			0, 0) ? true : false;
		updateSaveAllStepsEnabled();
		break;
	case IDC_IGNORE_PBUFFER:
		ignorePBuffer = SendDlgItemMessage(hSaveDialog, controlId, BM_GETCHECK,
			0, 0) ? true : false;
		break;
	case IDC_TRANSPARENT_BACKGROUND:
		saveAlpha = SendDlgItemMessage(hSaveDialog, controlId, BM_GETCHECK, 0,
			0) ? true : false;
		break;
	case IDC_AUTO_CROP:
		autoCrop = SendDlgItemMessage(hSaveDialog, controlId, BM_GETCHECK, 0,
			0) ? true : false;
		break;
	case IDC_SAME_SCALE:
		saveStepsSameScale = SendDlgItemMessage(hSaveDialog, controlId,
			BM_GETCHECK, 0, 0) ? true : false;
		break;
	case IDC_SAVE_ZOOMTOFIT:
		saveZoomToFit = SendDlgItemMessage(hSaveDialog, controlId, BM_GETCHECK,
			0, 0) ? true : false;
		updateSaveAllStepsEnabled();
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL ModelWindow::doSaveKillFocus(int controlId, HWND /*hControlWnd*/)
{
	switch (controlId)
	{
	case IDC_SAVE_WIDTH:
		updateSaveWidth();
		break;
	case IDC_SAVE_HEIGHT:
		updateSaveHeight();
		break;
	case IDC_SAVE_DIGITS:
		updateSaveDigits();
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL ModelWindow::doSaveNotify(int /*controlId*/, LPOFNOTIFY notification)
{
	switch(notification->hdr.code)
	{
	case CDN_TYPECHANGE:
		{
			char buf[1024];

			saveType = notification->lpOFN->nFilterIndex;
			switch (curSaveOp)
			{
			case LDPreferences::SOExport:
				saveExportType = saveType;
				break;
			case LDPreferences::SOSnapshot:
			default:
				saveImageType = saveType;
				break;
			}
			if (calcSaveFilename(buf, 1024))
			{
				SendMessage(GetParent(hSaveDialog), CDM_SETCONTROLTEXT, edt1,
					(LPARAM)buf);
			}
			saveTypeUpdated();
		}
		break;
	case CDN_INITDONE:
		return doSaveInitDone(notification);
		break;
	default:
//		debugPrintf("%d\n", notification->hdr.code);
		break;
	}
	return FALSE;
}

BOOL ModelWindow::doSaveCommand(int controlId, int notifyCode, HWND hControlWnd)
{
//	debugPrintf("ModelWindow::doSaveCommand: 0x%04X, 0x%04X, 0x%04x\n",
//		controlId, notifyCode, hControlWnd);
	switch (notifyCode)
	{
	case BN_CLICKED:
		return doSaveClick(controlId, hControlWnd);
		break;
	case EN_KILLFOCUS:
		return doSaveKillFocus(controlId, hControlWnd);
		break;
	case EN_CHANGE:
		switch (controlId)
		{
		case IDC_SAVE_DIGITS:
			updateSaveDigits();
			break;
		case IDC_STEP_SUFFIX:
			updateStepSuffix();
			break;
		}
		break;
	}
	return FALSE;
}

BOOL ModelWindow::doPrintClick(int controlId, HWND /*hControlWnd*/)
{
	switch (controlId)
	{
	case IDC_PRINT_PRINTER_DPI:
		usePrinterDPI = true;
		updatePrintDPIField();
		break;
	case IDC_PRINT_SPECIFY_DPI:
		usePrinterDPI = false;
		updatePrintDPIField();
		break;
	case IDC_PRINT_BACKGROUND:
		printBackground = SendDlgItemMessage(hPrintDialog, IDC_PRINT_BACKGROUND,
			BM_GETCHECK, 0, 0) ? true : false;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void ModelWindow::updatePrintDPI(void)
{
	char buf[1024];
	int temp;

	SendDlgItemMessage(hPrintDialog, IDC_PRINT_DPI, WM_GETTEXT, 128,
		(LPARAM)buf);
	if (sscanf(buf, "%d", &temp) == 1 && temp > 0 && temp <= 2400)
	{
		printDPI = temp;
	}
	else
	{
		sprintf(buf, "%d", printDPI);
		SendDlgItemMessage(hPrintDialog, IDC_PRINT_DPI, WM_SETTEXT, 0,
			(LPARAM)buf);
		MessageBeep(MB_OK);
	}
}

BOOL ModelWindow::doPrintKillFocus(int controlId, HWND /*hControlWnd*/)
{
	switch (controlId)
	{
	case IDC_PRINT_DPI:
		updatePrintDPI();
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
BOOL ModelWindow::doPrintCommand(int controlId, int notifyCode,
								 HWND hControlWnd)
{
	switch (notifyCode)
	{
	case BN_CLICKED:
		return doPrintClick(controlId, hControlWnd);
		break;
	case EN_KILLFOCUS:
		return doPrintKillFocus(controlId, hControlWnd);
		break;
	}
	return FALSE;
}

UINT_PTR CALLBACK ModelWindow::staticSaveHook(
	HWND hDlg,
	UINT uiMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	ModelWindow* modelWindow = (ModelWindow*)GetWindowLong(hDlg, GWLP_USERDATA);

	if (uiMsg == WM_INITDIALOG)
	{
		modelWindow = (ModelWindow*)((OPENFILENAME*)lParam)->lCustData;
		if (modelWindow)
		{
			modelWindow->hSaveDialog = hDlg;
			SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)modelWindow);
		}
	}
	if (modelWindow)
	{
		return modelWindow->dialogProc(hDlg, uiMsg, wParam, lParam);
	}
	else
	{
		return 0;
	}
}

const char *ModelWindow::saveExtension(int type /*= -1*/) const
{
	if (type == -1)
	{
		type = saveImageType;
	}
	switch (type)
	{
	case BMP_IMAGE_TYPE_INDEX:
		return "bmp";
	case JPG_IMAGE_TYPE_INDEX:
		return "jpg";
	case SVG_IMAGE_TYPE_INDEX:
		return "svg";
	case EPS_IMAGE_TYPE_INDEX:
		return "eps";
	case PDF_IMAGE_TYPE_INDEX:
		return "pdf";
	default:
		return "png";
	}
}

bool ModelWindow::calcSaveFilename(
	char* saveFilename,
	int /*len*/)
{
	char* filename = filenameFromPath(modelViewer->getFilename());

	if (filename != NULL)
	{
		bool found = false;
		std::string baseFilename = filename;
		size_t dotSpot;
		LDLModel *mpdChild = modelViewer->getMpdChild();

		delete filename;
		dotSpot = baseFilename.rfind('.');
		if (dotSpot < baseFilename.size())
		{
			baseFilename.erase(dotSpot);
		}
		if (mpdChild != NULL && mpdChild->getName() != NULL &&
			modelViewer->getMpdChildIndex() > 0)
		{
			std::string mpdName = mpdChild->getName();
			size_t dotSpot = mpdName.rfind('.');

			if (dotSpot < mpdName.size())
			{
				std::string mpdExt = mpdName.substr(dotSpot);

				convertStringToLower(&mpdExt[0]);
				if (mpdExt == ".dat" || mpdExt == ".ldr" || mpdExt == ".mpd")
				{
					mpdName = mpdName.substr(0, dotSpot);
				}
			}
			baseFilename += '-';
			baseFilename += mpdName;
		}
		if (curSaveOp == LDPreferences::SOExport)
		{
			LDExporter *exporter = modelViewer->getExporter(
				(LDrawModelViewer::ExportType)saveExportType);
			std::string extension;

			if (exporter)
			{
				extension = exporter->getExtension();
			}
			else
			{
				extension = "pov";
			}
			sprintf(saveFilename, "%s.%s", baseFilename.c_str(),
				extension.c_str());
			return true;
		}
		else
		{
			const char *extension = saveExtension();
			char format[32] = "%s.%s";
			int max;
			int i;

			if (saveSeries)
			{
				sprintf(format, "%%s%%0%dd.%%s", saveDigits);
				max = (int)(pow(10.0, saveDigits) + 0.1);
			}
			else
			{
				max = 2;
			}
			for (i = 1; i < max && !found; i++)
			{
				if (saveSeries)
				{
					sprintf(saveFilename, format, baseFilename.c_str(), i,
						extension);
				}
				else
				{
					sprintf(saveFilename, format, baseFilename.c_str(), extension);
				}
				if (saveAllSteps)
				{
					char *suffix = ucstringtombs(saveStepSuffix);
					std::string temp = LDSnapshotTaker::addStepSuffix(saveFilename,
						suffix, 1, modelViewer->getNumSteps());
					delete suffix;
					strcpy(saveFilename, temp.c_str());
				}
				if (!LDrawModelViewer::fileExists(saveFilename))
				{
					found = true;
				}
			}
			return true;
		}
	}
	else
	{
		return false;
	}
}

// Note: static method
std::string ModelWindow::defaultDir(
	LDPreferences::DefaultDirMode mode,
	const char *lastPath,
	const char *defaultPath)
{
	switch (mode)
	{
	case LDPreferences::DDMLastDir:
		{
			return lastPath;
		}
	case LDPreferences::DDMSpecificDir:
		return defaultPath;
	}
	return ".";
}

std::string ModelWindow::getSaveDir(LDPreferences::SaveOp saveOp)
{
	LDPreferences *ldPrefs = prefs->getLDPrefs();

	if (ldPrefs != NULL)
	{
		return ldPrefs->getDefaultSaveDir(saveOp, modelViewer->getFilename());
	}
	return ".";
}

std::string ModelWindow::getSaveDir(void)
{
	return getSaveDir(curSaveOp);
}

void ModelWindow::fillSnapshotFileTypes(char *fileTypes)
{
	memset(fileTypes, 0, 2);
	addFileType(fileTypes, ls("PngFileType"), "*.png");
	addFileType(fileTypes, ls("BmpFileType"), "*.bmp");
	addFileType(fileTypes, ls("JpgFileType"), "*.jpg");
	if (TCUserDefaults::boolForKey(GL2PS_ALLOWED_KEY, false, false))
	{
		addFileType(fileTypes, ls("SvgFileType"), "*.svg");
		addFileType(fileTypes, ls("EpsFileType"), "*.eps");
		addFileType(fileTypes, ls("PdfFileType"), "*.pdf");
	}
}

void ModelWindow::fillExportFileTypes(char *fileTypes)
{
	memset(fileTypes, 0, 2 * sizeof(fileTypes[0]));
	for (int i = LDrawModelViewer::ETFirst; i <= LDrawModelViewer::ETLast; i++)
	{
		const LDExporter *exporter = modelViewer->getExporter(
			(LDrawModelViewer::ExportType)i);

		if (exporter != NULL)
		{
			ucstring fileType = exporter->getTypeDescription();
			char *aFileType = ucstringtombs(fileType.c_str(),
				(int)fileType.size());
			std::string extension = "*.";

			extension += exporter->getExtension();
			addFileType(fileTypes, aFileType, extension.c_str());
			delete aFileType;
		}
	}
}

bool ModelWindow::getSaveFilename(
	char* saveFilename,
	int len)
{
	OPENFILENAME openStruct;
	char fileTypes[1024];
	std::string initialDir = getSaveDir();
	int maxImageType = 3;
	char defaultExt[32];

	stopAnimation();
	memset(&openStruct, 0, sizeof(OPENFILENAME));
	if (!calcSaveFilename(saveFilename, len))
	{
		saveFilename[0] = 0;
	}
	openStruct.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |
		OFN_ENABLESIZING;
	switch (curSaveOp)
	{
	case LDPreferences::SOExport:
		{
			const LDExporter *exporter = modelViewer->getExporter();
			std::string extension = exporter->getExtension();

			fillExportFileTypes(fileTypes);
			modelViewer->setExportType(
				(LDrawModelViewer::ExportType)saveExportType);
			saveType = saveExportType;
			openStruct.lpstrTitle = ls("ExportModel");
			openStruct.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
			openStruct.hInstance = getLanguageModule();
			openStruct.lpTemplateName = MAKEINTRESOURCE(IDD_EXPORT_SAVE_OPTIONS);
			openStruct.lpfnHook = staticSaveHook;
			strcpy(defaultExt, extension.c_str());
		}
		break;
	case LDPreferences::SOSnapshot:
	default:
		fillSnapshotFileTypes(fileTypes);
		if (saveImageType < 1 || saveImageType > maxImageType)
		{
			saveImageType = 1;
		}
		saveType = saveImageType;
		openStruct.lpstrTitle = ls("SaveSnapshot");
		openStruct.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
		openStruct.hInstance = getLanguageModule();
		openStruct.lpTemplateName = MAKEINTRESOURCE(IDD_SAVE_OPTIONS);
		openStruct.lpfnHook = staticSaveHook;
		strcpy(defaultExt, saveExtension());
		break;
	}
	openStruct.lStructSize = getOpenFilenameSize(false);
	openStruct.hwndOwner = hWindow;
	openStruct.lpstrFilter = fileTypes;
	openStruct.nFilterIndex = saveType;
	openStruct.lpstrFile = saveFilename;
	openStruct.lpstrDefExt = defaultExt;
	openStruct.nMaxFile = len;
	openStruct.lpstrInitialDir = initialDir.c_str();
	openStruct.lCustData = (long)this;
	if (GetSaveFileName(&openStruct))
	{
		int index = (int)openStruct.nFilterIndex;
		char *dir = directoryFromPath(saveFilename);
		LDPreferences *ldPrefs = prefs->getLDPrefs();

		if (ldPrefs != NULL)
		{
			ldPrefs->setLastSaveDir(curSaveOp, dir, true);
		}
		delete dir;
		switch (curSaveOp)
		{
		case LDPreferences::SOExport:
			TCUserDefaults::setLongForKey(saveExportType,
				SAVE_EXPORT_TYPE_KEY, false);
			break;
		case LDPreferences::SOSnapshot:
		default:
			if (index > 0 && index <= maxImageType)
			{
				TCUserDefaults::setLongForKey(saveImageType,
					SAVE_IMAGE_TYPE_KEY, false);
			}
			break;
		}
		TCObject::release(saveWindowResizer);
		saveWindowResizer = NULL;
		TCUserDefaults::setBoolForKey(saveAlpha, SAVE_ALPHA_KEY, false);
		TCUserDefaults::setBoolForKey(autoCrop, AUTO_CROP_KEY, false);
		TCUserDefaults::setBoolForKey(saveAllSteps, SAVE_STEPS_KEY, false);
		if (saveAllSteps)
		{
			TCUserDefaults::setStringForKey(saveStepSuffix,
				SAVE_STEPS_SUFFIX_KEY, false);
			TCUserDefaults::setBoolForKey(saveStepsSameScale,
				SAVE_STEPS_SAME_SCALE_KEY, false);
		}
		return true;
	}
	else
	{
		loadSaveSettings();
	}
	TCObject::release(saveWindowResizer);
	saveWindowResizer = NULL;
	return false;
}

bool ModelWindow::shouldOverwriteFile(char* filename)
{
	char buf[256];

	sprintf(buf, TCLocalStrings::get("OverwritePrompt"),
		filename);
	if (MessageBox(hWindow, buf, "LDView", MB_YESNO | MB_ICONQUESTION) ==
		IDYES)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ModelWindow::exportModel(void)
{
	char filename[1024];

	curSaveOp = LDPreferences::SOExport;
	if (getSaveFilename(filename, COUNT_OF(filename)))
	{
		LDViewWindow *ldviewWindow = ((LDViewWindow *)parentWindow);
		std::string copyright = ldviewWindow->getLegalCopyright();
		unsigned char copyrightSym = 169;
		size_t index = copyright.find((char)copyrightSym);

		if (index < copyright.size())
		{
			copyright = copyright.substr(0, index) + "(C)" +
				copyright.substr(index + 1);
		}
		modelViewer->setExportType(
			(LDrawModelViewer::ExportType)saveExportType);
		setWaitCursor();
		modelViewer->exportCurModel(filename,
			ldviewWindow->getProductVersion());
		setArrowCursor();
	}
}

bool ModelWindow::saveSnapshot(void)
{
	char saveFilename[1024] = "";
	bool retValue = saveSnapshot(saveFilename);

	forceRedraw();
	return retValue;
}

bool ModelWindow::saveSnapshot(char *saveFilename, bool fromCommandLine,
							   bool notReallyCommandLine)
{
	bool externalFilename = saveFilename[0] != 0;

	curSaveOp = LDPreferences::SOSnapshot;
	savingFromCommandLine = fromCommandLine && !notReallyCommandLine;
	if (saveFilename[0])
	{
		char *snapshotSuffix = TCUserDefaults::stringForKey(SNAPSHOT_SUFFIX_KEY,
			NULL, false);

		if (!snapshotSuffix)
		{
			char *suffixSpot = strrchr(saveFilename, '.');

			if (suffixSpot)
			{
				snapshotSuffix = copyString(suffixSpot);
			}
			else
			{
				snapshotSuffix = copyString("");
			}
		}
		if (stringHasCaseInsensitiveSuffix(snapshotSuffix, ".png"))
		{
			saveImageType = PNG_IMAGE_TYPE_INDEX;
		}
		else if (stringHasCaseInsensitiveSuffix(snapshotSuffix, ".bmp"))
		{
			saveImageType = BMP_IMAGE_TYPE_INDEX;
		}
		else if (stringHasCaseInsensitiveSuffix(snapshotSuffix, ".jpg"))
		{
			saveImageType = JPG_IMAGE_TYPE_INDEX;
		}
		else
		{
			delete snapshotSuffix;
			if (fromCommandLine)
			{
				consolePrintf(TCLocalStrings::get(
					_UC("ConsoleSnapshotFailed")));
			}
			return false;
		}
		delete snapshotSuffix;
	}
	if (saveFilename[0] || getSaveFilename(saveFilename, 1024))
	{
		if (!fromCommandLine)
		{
			TCUserDefaults::setLongForKey(ignorePBuffer ? 1 : 0,
				IGNORE_PBUFFER_KEY, false);
			if (saveSeries)
			{
				TCUserDefaults::setLongForKey(1, SAVE_SERIES_KEY, false);
				TCUserDefaults::setLongForKey(saveDigits, SAVE_DIGITS_KEY,
					false);
			}
			else
			{
				TCUserDefaults::setLongForKey(0, SAVE_SERIES_KEY, false);
			}
		}
		if (saveActualSize)
		{
			if (!fromCommandLine)
			{
				TCUserDefaults::setLongForKey(1, SAVE_ACTUAL_SIZE_KEY, false);
			}
			return saveImage(saveFilename, width, height, externalFilename &&
				saveZoomToFit);
		}
		else
		{
			if (!fromCommandLine)
			{
				TCUserDefaults::setLongForKey(0, SAVE_ACTUAL_SIZE_KEY, false);
				TCUserDefaults::setLongForKey(saveWidth, SAVE_WIDTH_KEY, false);
				TCUserDefaults::setLongForKey(saveHeight, SAVE_HEIGHT_KEY,
					false);
				TCUserDefaults::setLongForKey(saveZoomToFit,
					SAVE_ZOOM_TO_FIT_KEY, false);
			}
			return saveImage(saveFilename, saveWidth, saveHeight,
				saveZoomToFit);
		}
	}
	else
	{
		return false;
	}
}

BOOL ModelWindow::setupPFD(void)
{
	currentAntialiasType = TCUserDefaults::longForKey(FSAA_MODE_KEY);

	if (currentAntialiasType && LDVExtensionsSetup::haveMultisampleExtension())
	{
		GLint intValues[] = {
			WGL_SAMPLE_BUFFERS_EXT, GL_TRUE,
			WGL_SAMPLES_EXT, 1,
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_ALPHA_BITS_ARB, 4,
			WGL_STENCIL_BITS_ARB, 2,
			0, 0
		};
		int numIntValues = sizeof(intValues) / sizeof(intValues[0]);

		if (currentAntialiasType)
		{
			intValues[3] = prefs->getFSAAFactor();
		}
		else
		{
			// Remove the first four items in the array.
			memmove(intValues, intValues + 4, sizeof(intValues) -
				sizeof(intValues[0]) * 4);
			numIntValues -= 4;
		}
		pfIndex = LDVExtensionsSetup::choosePixelFormat(hdc, intValues);
		if (pfIndex < 0)
		{
			// Try with dest alpha but no stencil by clearing the last two used
			// elements in the array (there are two terminating zeros after
			// them).
			intValues[numIntValues - 4] = 0;
			intValues[numIntValues - 3] = 0;
			pfIndex = LDVExtensionsSetup::choosePixelFormat(hdc, intValues);
		}
		if (pfIndex < 0)
		{
			// Try with stencil but no dest alpha by changing the last two used
			// elements in the array back to their original values that specify
			// the stencil request.
			intValues[numIntValues - 6] = WGL_STENCIL_BITS_ARB;
			intValues[numIntValues - 5] = 1;
			pfIndex = LDVExtensionsSetup::choosePixelFormat(hdc, intValues);
		}
		if (pfIndex < 0)
		{
			// Try with no stencil OR dest alpha by clearing two more elements
			// at the end of the array.
			intValues[numIntValues - 6] = 0;
			intValues[numIntValues - 5] = 0;
			pfIndex = LDVExtensionsSetup::choosePixelFormat(hdc, intValues);
		}
		if (pfIndex >= 0)
		{
			if (setPixelFormat(pfIndex))
			{
				return TRUE;
			}
		}
	}
	currentAntialiasType = 0;
	return CUIOGLWindow::setupPFD();
}

void ModelWindow::setKeepRightSideUp(bool value, bool saveSetting /*= true*/)
{
	if (modelViewer != NULL)
	{
		modelViewer->setKeepRightSideUp(value);
	}
	if (saveSetting)
	{
		TCUserDefaults::setBoolForKey(value, KEEP_RIGHT_SIDE_UP_KEY, false);
	}
}

void ModelWindow::setViewMode(
	LDInputHandler::ViewMode mode,
	bool examineLatLong,
	bool saveSetting)
{
	inputHandler->setViewMode(mode);
	viewMode = mode;
	if (saveSetting)
	{
		TCUserDefaults::setLongForKey(viewMode, VIEW_MODE_KEY, false);
	}
	if (modelViewer)
	{
		if (viewMode == LDInputHandler::VMExamine)
		{
			LDrawModelViewer::ExamineMode examineMode;

			modelViewer->setConstrainZoom(true);
			if (examineLatLong)
			{
				examineMode = LDrawModelViewer::EMLatLong;
			}
			else
			{
				examineMode = LDrawModelViewer::EMFree;
			}
			if (saveSetting)
			{
				TCUserDefaults::setLongForKey(examineMode, EXAMINE_MODE_KEY,
					false);
			}
			modelViewer->setExamineMode(examineMode);
		}
		else
		{
			modelViewer->setConstrainZoom(false);
		}
	}
}

LRESULT ModelWindow::processKeyDown(int keyCode, LPARAM /*keyData*/)
{
	if (keyCode == VK_ESCAPE)
	{
		cancelLoad = true;
	}
	if (inputHandler->keyDown(getCurrentKeyModifiers(),
		convertKeyCode(keyCode)))
	{
		return 0;
	}
	return 1;
	//if (modelViewer && viewMode == LDVViewFlythrough)
	//{
	//	TCVector cameraMotion = modelViewer->getCameraMotion();
	//	TCFloat motionAmount = 1.0f;
	//	TCFloat rotationAmount = 0.01f;
	//	TCFloat strafeAmount = 1.0f;
	//	int i;

	//	if (modelViewer)
	//	{
	//		TCFloat fov = modelViewer->getFov();

	//		motionAmount = modelViewer->getDefaultDistance() / 400.0f;
	//		rotationAmount *= (TCFloat)tan(deg2rad(fov));
	//		strafeAmount *= (TCFloat)sqrt(fov / 45.0f);
	//	}
	//	if (GetKeyState(VK_SHIFT) & 0x8000)
	//	{
	//		motionAmount *= 2.0f;
	//		strafeAmount *= 2.0f;
	//	}
	//	switch (keyCode)
	//	{
	//	case 'W':
	//		cameraMotion[2] = -motionAmount;
	//		break;
	//	case 'S':
	//		cameraMotion[2] = motionAmount;
	//		break;
	//	case 'A':
	//		cameraMotion[0] = -strafeAmount;
	//		break;
	//	case 'D':
	//		cameraMotion[0] = strafeAmount;
	//		break;
	//	case 'R':
	//		cameraMotion[1] = strafeAmount;
	//		break;
	//	case 'F':
	//		cameraMotion[1] = -strafeAmount;
	//		break;
	//	case 'E':
	//		modelViewer->setCameraZRotate(0.01f);
	//		break;
	//	case 'Q':
	//		modelViewer->setCameraZRotate(-0.01f);
	//		break;
	//	case VK_UP:
	//		modelViewer->setCameraYRotate(rotationAmount);
	//		break;
	//	case VK_DOWN:
	//		modelViewer->setCameraYRotate(-rotationAmount);
	//		break;
	//	case VK_LEFT:
	//		modelViewer->setCameraXRotate(-rotationAmount);
	//		break;
	//	case VK_RIGHT:
	//		modelViewer->setCameraXRotate(rotationAmount);
	//		break;
	//	case VK_SHIFT:
	//		for (i = 0; i < 3; i++)
	//		{
	//			if (cameraMotion[i] > 0.0f)
	//			{
	//				cameraMotion[i] = 2.0;
	//			}
	//			else if (cameraMotion[i] < 0.0f)
	//			{
	//				cameraMotion[i] = -2.0;
	//			}
	//		}
	//		break;
	//	default:
	//		return 1;
	//		break;
	//	}
	//	modelViewer->setCameraMotion(cameraMotion);
	//	forceRedraw(2);
	//	return 0;
	//}
	//return 1;
}

LRESULT ModelWindow::processKeyUp(int keyCode, LPARAM /*keyData*/)
{
	if (inputHandler->keyUp(getCurrentKeyModifiers(),
		convertKeyCode(keyCode)))
	{
		return 0;
	}
	return 1;
	//if (modelViewer && viewMode == LDVViewFlythrough)
	//{
	//	TCVector cameraMotion = modelViewer->getCameraMotion();
	//	int i;

	//	switch (keyCode)
	//	{
	//	case 'W':
	//		cameraMotion[2] = 0.0f;
	//		break;
	//	case 'S':
	//		cameraMotion[2] = 0.0f;
	//		break;
	//	case 'A':
	//		cameraMotion[0] = 0.0f;
	//		break;
	//	case 'D':
	//		cameraMotion[0] = 0.0f;
	//		break;
	//	case 'R':
	//		cameraMotion[1] = 0.0f;
	//		break;
	//	case 'F':
	//		cameraMotion[1] = 0.0f;
	//		break;
	//	case 'E':
	//		modelViewer->setCameraZRotate(0.0f);
	//		break;
	//	case 'Q':
	//		modelViewer->setCameraZRotate(0.0f);
	//		break;
	//	case VK_UP:
	//		modelViewer->setCameraYRotate(0.0f);
	//		break;
	//	case VK_DOWN:
	//		modelViewer->setCameraYRotate(0.0f);
	//		break;
	//	case VK_LEFT:
	//		modelViewer->setCameraXRotate(0.0f);
	//		break;
	//	case VK_RIGHT:
	//		modelViewer->setCameraXRotate(0.0f);
	//		break;
	//	case VK_SHIFT:
	//		for (i = 0; i < 3; i++)
	//		{
	//			if (cameraMotion[i] > 0.0f)
	//			{
	//				cameraMotion[i] = 1.0;
	//			}
	//			else if (cameraMotion[i] < 0.0f)
	//			{
	//				cameraMotion[i] = -1.0;
	//			}
	//		}
	//		break;
	//	default:
	//		return 1;
	//		break;
	//	}
	//	modelViewer->setCameraMotion(cameraMotion);
	//	forceRedraw(2);
	//	return 0;
	//}
	//return 1;
}

LRESULT ModelWindow::doMove(int newX, int newY)
{
	return CUIOGLWindow::doMove(newX, newY);
}

void ModelWindow::makeCurrent(void)
{
	if (initializedGL)
	{
		if (hCurrentDC && hCurrentGLRC)
		{
			wglMakeCurrent(hCurrentDC, hCurrentGLRC);
		}
		else
		{
			CUIOGLWindow::makeCurrent();
		}
	}
}

bool ModelWindow::performHotKey(int hotKeyIndex)
{
	if (prefs)
	{
		return prefs->performHotKey(hotKeyIndex);
	}
	else
	{
		return false;
	}
}

void ModelWindow::initFail(char * /*reason*/)
{
	MessageBox(hWindow, TCLocalStrings::get("OpenGlInitFailed"),
		TCLocalStrings::get("FatalError"), MB_OK | MB_ICONERROR);
	PostQuitMessage(-1);
}

void ModelWindow::drawLight(GLenum /*light*/, TCFloat /*x*/, TCFloat /*y*/,
							TCFloat /*z*/)
{
	// Don't call super.
}

void ModelWindow::drawLights(void)
{
	// Don't call super.
}

void ModelWindow::orthoView(void)
{
	// Don't call super.
}

void ModelWindow::setupLight(GLenum /*light*/)
{
	// Don't call super.
}

void ModelWindow::setSaveZoomToFit(bool value, bool commit /*= false*/)
{
	saveZoomToFit = value;
	if (commit)
	{
		TCUserDefaults::setLongForKey(value, SAVE_ZOOM_TO_FIT_KEY, false);
	}
}

void ModelWindow::boundingBoxToggled(void)
{
	((LDViewWindow*)parentWindow)->boundingBoxToggled();
}
