/* LDrawModelView */

#import <Cocoa/Cocoa.h>

class LDrawModelViewer;
class TCImage;
class LDInputHandler;
class TCAlert;

typedef enum
{
	LDVViewExamine,
	LDVViewFlythrough
} LDVViewMode;

@interface LDrawModelView : NSOpenGLView
{
	LDrawModelViewer *modelViewer;

	float rotationSpeed;
	NSPoint lastMouseLocation;
	NSPoint lastFrameMouseLocation;
	float originalZoomY;
	BOOL lButtonDown;
	BOOL rButtonDown;
	unsigned int mouseDownModifierFlags;
	unsigned int rightMouseDownModifierFlags;
	//NSDate *lastMoveTime;
	LDVViewMode viewMode;
	BOOL loading;
	BOOL parsing;
	BOOL redisplayRequested;
	float fps;
	int resizeCornerTextureId;
	LDInputHandler *inputHandler;
	bool redrawRequested;
	bool fullScreen;
	bool desiredFlyThrough;
	bool threeFingerPan;
	id panIdentity;
}

- (BOOL)openModel:(NSString *)filename;
- (void)resetView:(id)sender;
- (LDrawModelViewer *)modelViewer;
- (void)reloadNeeded;
- (void)reload;
- (void)setFps:(float)value;
- (void)setViewingAngle:(int)value;
- (IBAction)zoomToFit:(id)sender;
- (IBAction)rightSideUp:(id)sender;
- (IBAction)viewMode:(id)sender;
- (void)setFlyThroughMode:(bool)flyThroughMode;
- (bool)flyThroughMode;
- (void)rotationUpdate;
- (bool)fullScreen;

- (IBAction)toggleFullScreen:(id)sender;

- (void)modelViewerAlertCallback:(TCAlert *)alert;
- (void)redrawAlertCallback:(TCAlert *)alert;
- (void)captureAlertCallback:(TCAlert *)alert;
- (void)releaseAlertCallback:(TCAlert *)alert;
//- (void)peekMouseUpAlertCallback:(TCAlert *)alert;

@end
