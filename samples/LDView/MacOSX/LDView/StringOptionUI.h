//
//  StringOptionUI.h
//  LDView
//
//  Created by Travis Cobbs on 6/16/08.
//  Copyright 2008 Travis Cobbs. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "OptionUI.h"


@interface StringOptionUI : OptionUI
{
	NSTextField *label;
	NSTextField *textField;
}

- (id)initWithOptions:(Options *)theOptions setting:(LDExporterSetting &)theSetting;
- (CGFloat)updateLayoutX:(CGFloat)x y:(CGFloat)y width:(CGFloat)width update:(bool)update optimalWidth:(CGFloat &)optimalWidth;
- (void)commit;
- (void)setEnabled:(BOOL)enabled;
- (NSRect)frame;
- (CGFloat)textFieldWidthForWidth:(CGFloat)width;
- (CGFloat)row2Height;
- (NSView *)firstKeyView;

@end
