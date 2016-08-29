#ifndef _NO_BOOST

#ifndef __LDLIBRARYUPDATER_H__
#define __LDLIBRARYUPDATER_H__

#include <TCFoundation/TCAlertSender.h>
#include <TCFoundation/TCStlIncludes.h>
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4244 4512)
#endif // WIN32
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#ifdef WIN32
#pragma warning(pop)
#endif // WIN32
//#include <TCFoundation/mystring.h>
//#include <stdio.h>

template <class Type> class TCTypedObjectArray;
class TCWebClient;
class LDLibraryUpdateInfo;
class TCStringArray;

typedef TCTypedObjectArray<TCWebClient> TCWebClientArray;
typedef TCTypedObjectArray<LDLibraryUpdateInfo> LDLibraryUpdateInfoArray;

typedef std::list<std::string> StringList;

#define LD_LIBRARY_UPDATER "LDLibraryUpdater"

class LDLibraryUpdater : public TCAlertSender
{
public:
	LDLibraryUpdater(void);
	void setLibraryUpdateKey(const char *libraryUpdateKey);
	void setLdrawDir(const char *ldrawDir);
	void checkForUpdates(void);
	void installLDraw(void);
	bool canCheckForUpdates(UCSTR &error);
	CUCSTR getError(void) { return m_error; }
protected:
	virtual ~LDLibraryUpdater(void);
	virtual void dealloc(void);
	virtual void threadRun(void);
	virtual void threadStart(void);
	virtual void threadFinish(void);
	void scanDir(const std::string &dir, StringList &dirList);
	bool findLatestOfficialRelease(const StringList &dirList, char *updateName,
		bool *aborted);
	bool findOfficialRelease(const std::string &filename, char *updateName);
	bool parseUpdateList(const char *updateList, bool *aborted);
	bool determineLastUpdate(LDLibraryUpdateInfoArray *updateArray,
		char *updateName, bool *aborted);
	int compareUpdates(LDLibraryUpdateInfoArray *updateArray, const char *left,
		const char *right);
	bool fileExists(const char *filename);
	TCStringArray *getUpdateQueue(void);
	void downloadUpdates(bool *aborted);
	void updateDlFinish(TCWebClient *webClient);
	void processUpdateQueue(void);
	void sendDlProgress(bool *aborted);
	void sendExtractProgress(bool *aborted);
	void extractUpdate(const char *filename);
	void extractUpdates(bool *aborted);
	void launchThread();
	bool caseSensitiveFileSystem(UCSTR &error);
	int getUpdateNumber(const char *updateName);

	TCWebClientArray *m_webClients;
	TCWebClientArray *m_finishedWebClients;
	boost::thread *m_thread;
	boost::mutex *m_mutex;
	boost::condition *m_threadFinish;
	char *m_libraryUpdateKey;
	char *m_ldrawDir;
	char *m_ldrawDirParent;
	TCStringArray *m_updateQueue;
	TCStringArray *m_updateUrlList;
	TCStringArray *m_downloadList;
	int m_initialQueueSize;
	UCCHAR m_error[1024];
	bool m_aborting;
	bool m_install;
/*
	class ThreadHelper
	{
	public:
		ThreadHelper(LDLibraryUpdater *libraryUpdater)
			: m_libraryUpdater(libraryUpdater)
		{
			printf("     ThreadHelper: 0x%08x.\n", this);
		}
		ThreadHelper(const ThreadHelper &other)
			: m_libraryUpdater(other.m_libraryUpdater)
		{
			debugPrintf("ThreadHelper copy: 0x%08x, 0x%08x.\n", this,
				m_libraryUpdater);
		}
		~ThreadHelper(void)
		{
			debugPrintf("    ~ThreadHelper: 0x%08x.\n", this);
		}
		void operator()()
		{
			m_libraryUpdater->threadStart();
			m_libraryUpdater->threadFinish();
		}
*/
	protected:
		LDLibraryUpdater *m_libraryUpdater;
//	friend class ThreadHelper;
};

#endif // __LDLIBRARYUPDATER_H__

#endif // !_NO_BOOST
