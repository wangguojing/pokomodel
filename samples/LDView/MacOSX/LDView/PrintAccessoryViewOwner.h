/* PrintAccessoryViewOwner */

#import <Cocoa/Cocoa.h>

@interface PrintAccessoryViewOwner : NSObject
{
    IBOutlet NSView *accessoryView;
    IBOutlet NSButton *adjustThicknessCheck;
    IBOutlet NSTextField *bottomField;
    IBOutlet NSTextField *dpiField;
    IBOutlet NSPopUpButton *dpiPopUp;
    IBOutlet NSTextField *leftField;
    IBOutlet NSButton *printBackgroundCheck;
    IBOutlet NSTextField *rightField;
    IBOutlet NSTextField *topField;
	
	NSPrintOperation *printOperation;
	float minLeft;
	float minRight;
	float minTop;
	float minBottom;
	int dpi;
	int customDpi;
}

- (id)initWithPrintOperation:(NSPrintOperation *)thePrintOperation;

- (IBAction)dpi:(id)sender;

@end
