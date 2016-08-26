/* PrefSetNew */

#import <Cocoa/Cocoa.h>
#import "GenericSheet.h"

@class PrefSetsPage;

@interface PrefSetNewSheet : GenericSheet
{
    IBOutlet NSTextField *nameField;

	PrefSetsPage *parent;
}

- (id)initWithParent:(PrefSetsPage *)value;
- (NSString *)getName;

@end
