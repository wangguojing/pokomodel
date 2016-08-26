#ifndef __LDSNAPSHOTTAKER_H__
#define __LDSNAPSHOTTAKER_H__

#include <TCFoundation/TCAlertSender.h>
#include <TCFoundation/TCStlIncludes.h>
#include <TCFoundation/TCImage.h>

class LDrawModelViewer;

class LDSnapshotTaker : public TCAlertSender
{
public:
	enum ImageType
	{
		ITFirst = 1,
		ITPng = 1,
		ITBmp = 2,
		ITJpg = 3,
		ITSvg = 4,
		ITEps = 5,
		ITPdf = 6,
		ITLast = 6
	};
	LDSnapshotTaker(void);
	LDSnapshotTaker(LDrawModelViewer *modelViewer);
	LDrawModelViewer *getModelViewer(void) { return m_modelViewer; }
	void setImageType(ImageType value) { m_imageType = value; }
	ImageType getImageType(void) const { return m_imageType; }
	void setTrySaveAlpha(bool value) { m_trySaveAlpha = value; }
	bool getTrySaveAlpha(void) const { return m_trySaveAlpha; }
	void setAutoCrop(bool value) { m_autoCrop = value; }
	bool getAutoCrop(void) const { return m_autoCrop; }
	void setUseFBO(bool value);
	bool getUseFBO(void) const { return m_useFBO; }
	void set16BPC(bool value) { m_16BPC = value; }
	bool get16BPC(void) const { return m_16BPC; }
	int getFBOSize(void) const;
	void setProductVersion(const std::string &value)
	{
		m_productVersion = value;
	}
	const std::string &getProductVersion(void) const
	{
		return m_productVersion;
	}
	void calcTiling(int desiredWidth, int desiredHeight, int &bitmapWidth,
		int &bitmapHeight, int &numXTiles, int &numYTiles);

	bool saveImage(const char *filename, int imageWidth, int imageHeight,
		bool zoomToFit);
	bool saveImage(void);
	TCByte *grabImage(int &imageWidth, int &imageHeight, bool zoomToFit,
		TCByte *buffer, bool *saveAlpha);

	static bool doCommandLine(void);
	static std::string removeStepSuffix(const std::string &filename,
		const std::string &stepSuffix);
	static std::string addStepSuffix(const std::string &filename,
		const std::string &stepSuffix, int step, int numSteps);
	static std::string extensionForType(ImageType type,
		bool includeDot = false);
	static ImageType typeForFilename(const char *filename, bool gl2psAllowed);
	static ImageType lastImageType(void);

	static const char *alertClass(void) { return "LDSnapshotTaker"; }
protected:
	virtual ~LDSnapshotTaker(void);
	virtual void dealloc(void);
	bool writeJpg(const char *filename, int width, int height, TCByte *buffer);
	bool writeBmp(const char *filename, int width, int height, TCByte *buffer);
	bool writePng(const char *filename, int width, int height, TCByte *buffer,
		bool saveAlpha);
	bool writeImage(const char *filename, int width, int height, TCByte *buffer,
		const char *formatName, bool saveAlpha);
	bool canSaveAlpha(void);
	void renderOffscreenImage(void);
	bool imageProgressCallback(CUCSTR message, float progress);
	bool shouldZoomToFit(bool zoomToFit);
	void grabSetup(void);
	bool saveStepImage(const char *filename, int imageWidth, int imageHeight,
		bool zoomToFit);
	bool saveGl2psStepImage(const char *filename, int imageWidth,
		int imageHeight, bool zoomToFit);

	static bool staticImageProgressCallback(CUCSTR message, float progress,
		void* userData);

	LDrawModelViewer *m_modelViewer;
	ImageType m_imageType;
	std::string m_productVersion;
	bool m_trySaveAlpha;
	bool m_autoCrop;
	bool m_fromCommandLine;
	bool m_commandLineSaveSteps;
	bool m_commandLineStep;
	int m_step;
	bool m_grabSetupDone;
	bool m_gl2psAllowed;
	bool m_useFBO;
	bool m_16BPC;
	std::string m_modelFilename;
	std::string m_currentImageFilename;
};

#endif // __LDSNAPSHOTTAKER_H__
