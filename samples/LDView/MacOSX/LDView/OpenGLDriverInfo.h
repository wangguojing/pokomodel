/* OpenGLDriverInfo */

#import <Cocoa/Cocoa.h>

@interface OpenGLDriverInfo : NSObject
{
    IBOutlet NSPanel *panel;
    IBOutlet NSTextField *textField;
    IBOutlet NSTextView *textView;
}

- (id)init;
- (void)showWithInfo:(NSString *)info numExtensions:(int)numExtensions;
- (IBAction)ok:(id)sender;

@end
