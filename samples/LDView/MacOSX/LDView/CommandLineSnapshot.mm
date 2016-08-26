//
//  CommandLineSnapshot.mm
//  LDView
//
//  Created by Travis Cobbs on 10/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CommandLineSnapshot.h"
#import "OCUserDefaults.h"
#import "SnapshotTaker.h"
#include <TCFoundation/TCStringArray.h>
#include <TCFoundation/TCUserDefaults.h>
#include <LDLib/LDrawModelViewer.h>
#include <LDLib/LDUserDefaultsKeys.h>
#include <LDLib/LDPreferences.h>
#include <TRE/TREMainModel.h>

@implementation CommandLineSnapshot

+ (BOOL)takeSnapshot
{
	BOOL retValue = NO;
	SnapshotTaker *snapshotTaker = [[SnapshotTaker alloc] init];

	TREMainModel::loadStudTexture([[[NSBundle mainBundle] pathForResource:@"StudLogo" ofType:@"png"] cStringUsingEncoding:NSASCIIStringEncoding]);
	if ([snapshotTaker saveFile])
	{
		retValue = YES;
	}
	[snapshotTaker release];
	[[NSUserDefaults standardUserDefaults] synchronize];
	return retValue;
}

@end
