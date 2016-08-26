#import "PrintAccessoryViewOwner.h"
#import "LDViewCategories.h"

#include <LDLib/LDUserDefaultsKeys.h>
#include <TCFoundation/TCUserDefaults.h>

@implementation PrintAccessoryViewOwner

- (id)initWithPrintOperation:(NSPrintOperation *)thePrintOperation
{
	self = [super init];
	if (self != nil)
	{
		printOperation = [thePrintOperation retain];
		[NSBundle loadNibNamed:@"PrintAccessoryView.nib" owner:self];
	}
	return self;
}

- (float)pointsToInches:(float)value
{
	return value / 72.0f;
}

- (float)inchesToPoints:(float)value
{
	return (float)(int)(value * 72.0f + 0.5f);
}

- (BOOL)getMargin:(float *)pValue udKey:(const char *)udKey
{
	long value = TCUserDefaults::longForKey(udKey, -1, false);
	
	if (value != -1)
	{
		*pValue = (float)value;
		return YES;
	}
	else
	{
		return NO;
	}
}

- (void)initOptions
{
	dpi = TCUserDefaults::longForKey(PRINT_DPI_KEY, 300, false);
	customDpi = TCUserDefaults::longForKey(PRINT_CUSTOM_DPI_KEY, 200, false);
	if ([dpiPopUp indexOfItemWithTag:dpi] != -1)
	{
		[dpiPopUp selectItemWithTag:dpi];
		[dpiField setEnabled:NO];
		[dpiField setStringValue:@""];
	}
	else
	{
		[dpiPopUp selectItemWithTag:0];
		[dpiField setEnabled:YES];
		[dpiField setIntValue:customDpi];
	}
	[printBackgroundCheck setCheck:TCUserDefaults::boolForKey(PRINT_BACKGROUND_KEY, false, false)];
	[adjustThicknessCheck setCheck:TCUserDefaults::boolForKey(PRINT_ADJUST_EDGES_KEY, true, false)];
}

- (void)initMargins
{
	NSPrintInfo *printInfo = [printOperation printInfo];
	NSRect maxRect = [printInfo imageablePageBounds];
	NSSize paperSize = [printInfo paperSize];
	float value;
	
	if ([self getMargin:&value udKey:LEFT_MARGIN_KEY])
	{
		[printInfo setLeftMargin:value];
	}
	if ([self getMargin:&value udKey:RIGHT_MARGIN_KEY])
	{
		[printInfo setRightMargin:value];
	}
	if ([self getMargin:&value udKey:TOP_MARGIN_KEY])
	{
		[printInfo setTopMargin:value];
	}
	if ([self getMargin:&value udKey:BOTTOM_MARGIN_KEY])
	{
		[printInfo setBottomMargin:value];
	}
	minLeft = [self pointsToInches:maxRect.origin.x];
	[leftField setFloatValue:[self pointsToInches:[printInfo leftMargin]]];
	minRight = [self pointsToInches:paperSize.width - maxRect.size.width - maxRect.origin.x];
	[rightField setFloatValue:[self pointsToInches:[printInfo rightMargin]]];
	minTop = [self pointsToInches:paperSize.height - maxRect.size.height - maxRect.origin.y];
	[topField setFloatValue:[self pointsToInches:[printInfo topMargin]]];
	minBottom = [self pointsToInches:maxRect.origin.y];
	[bottomField setFloatValue:[self pointsToInches:[printInfo bottomMargin]]];
}

- (void)awakeFromNib
{
	[printOperation setAccessoryView:accessoryView];
	[self initOptions];
	[self initMargins];
}

- (void)dealloc
{
	[printOperation setAccessoryView:nil]; 
	[printOperation release];
	[accessoryView release];
	[super dealloc];
}

- (IBAction)dpi:(id)sender
{
	if ([sender selectedTag] == 0)
	{
		[dpiField setEnabled:YES];
		[dpiField setIntValue:customDpi];
		dpi = customDpi;
	}
	else
	{
		dpi = [sender selectedTag];
		[dpiField setEnabled:NO];
		[dpiField setStringValue:@""];
	}
	TCUserDefaults::setLongForKey(dpi, PRINT_DPI_KEY, false);
}

- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
	NSTextField *field = [aNotification object];
	NSPrintInfo *printInfo = [printOperation printInfo];
	float value = [field floatValue];
	const char *udKey = NULL;

	if (field == leftField)
	{
		if ([field floatValue] < minLeft)
		{
			[field setFloatValue:minLeft];
			value = minLeft;
		}
		[printInfo setLeftMargin:[self inchesToPoints:value]];
		udKey = LEFT_MARGIN_KEY;
	}
	else if (field == rightField)
	{
		if ([field floatValue] < minRight)
		{
			[field setFloatValue:minRight];
			value = minRight;
		}
		[printInfo setRightMargin:[self inchesToPoints:value]];
		udKey = RIGHT_MARGIN_KEY;
	}
	else if (field == topField)
	{
		if ([field floatValue] < minTop)
		{
			[field setFloatValue:minTop];
			value = minTop;
		}
		[printInfo setTopMargin:[self inchesToPoints:value]];
		udKey = TOP_MARGIN_KEY;
	}
	else if (field == bottomField)
	{
		if ([field floatValue] < minBottom)
		{
			[field setFloatValue:minBottom];
			value = minBottom;
		}
		[printInfo setBottomMargin:[self inchesToPoints:value]];
		udKey = BOTTOM_MARGIN_KEY;
	}
	else if (field == dpiField)
	{
		customDpi = [field intValue];
		if (customDpi < 72)
		{
			dpi = 72;
		}
		else if (customDpi > 1200)
		{
			dpi = 1200;
		}
		else
		{
			dpi = customDpi;
		}
		if (dpi != customDpi)
		{
			customDpi = dpi;
			[field setIntValue:customDpi];
		}
		TCUserDefaults::setLongForKey(dpi, PRINT_DPI_KEY, false);
		TCUserDefaults::setLongForKey(dpi, PRINT_CUSTOM_DPI_KEY, false);
	}
	if (udKey)
	{
		TCUserDefaults::setLongForKey((long)[self inchesToPoints:value], udKey, false);
	}
}

- (IBAction)printBackground:(id)sender
{
	TCUserDefaults::setBoolForKey([sender getCheck], PRINT_BACKGROUND_KEY, false);
}

- (IBAction)adjustEdges:(id)sender
{
	TCUserDefaults::setBoolForKey([sender getCheck], PRINT_ADJUST_EDGES_KEY, false);
}

@end
