// The following is necessary to get rid of some truly screwed up warnings
#pragma GCC visibility push(default)

#import "LDrawPage.h"
#import "OCLocalStrings.h"
#import "LDViewCategories.h"

#include <TCFoundation/TCUserDefaults.h>
#include <LDLib/LDPreferences.h>
#include <LDLib/LDUserDefaultsKeys.h>

@implementation LDrawPage

- (void)dealloc
{
	[tableViewReorder release];
	[super dealloc];
}

- (void)setupExtraFolders:(const StringVector &)extraDirs
{
	[extraFolders release];
	extraFolders = [[NSMutableArray alloc] initWithCapacity:extraDirs.size()];
	for (size_t i = 0; i < extraDirs.size(); i++)
	{
		[extraFolders addObject:[NSString stringWithASCIICString:extraDirs[i].c_str()]];
	}
	[extraFoldersTableView reloadData];
}

- (void)setup
{
	[ldrawDirField setStringValue:[NSString stringWithASCIICString:ldPreferences->getLDrawDir()]];
	[self setupExtraFolders:ldPreferences->getExtraDirs()];
	[super setup];
}

+ (bool)verifyLDrawDir:(NSString *)ldrawDir
{
	NSFileManager *fileManager = [NSFileManager defaultManager];
	BOOL isDir;

	if ([ldrawDir length] == 0 && !TCUserDefaults::boolForKey(VERIFY_LDRAW_DIR_KEY, true, false))
	{
		return true;
	}
	if ([fileManager fileExistsAtPath:ldrawDir isDirectory:&isDir] && isDir)
	{
		NSString *partsDir = [ldrawDir stringByAppendingPathComponent:@"parts"];

		if ([fileManager fileExistsAtPath:partsDir isDirectory:&isDir] && isDir)
		{
			NSString *pDir = [ldrawDir stringByAppendingPathComponent:@"p"];

			if ([fileManager fileExistsAtPath:pDir isDirectory:&isDir] && isDir)
			{
				return true;
			}
		}
	}
	return false;
}

- (void)updateLDrawDir:(NSString *)ldrawDir apply:(BOOL)apply
{
	const char *value = [ldrawDir cStringUsingEncoding:NSASCIIStringEncoding];
	
	ldPreferences->setLDrawDir(value);
	if (apply)
	{
		ldPreferences->applyLDrawSettings();
		ldPreferences->commitLDrawSettings();
		[ldrawDirField setStringValue:ldrawDir];
	}
}

- (NSString *)ldrawDir
{
	return [NSString stringWithASCIICString:ldPreferences->getLDrawDir()];
}

- (void)updateLDrawDir:(NSString *)ldrawDir
{
	[self updateLDrawDir:ldrawDir apply:YES];
}

- (bool)updateLdPreferences
{
	StringVector extraDirs;

	if (![[self class] verifyLDrawDir:[ldrawDirField stringValue]])
	{
		[preferences setApplyFailed:self];
		NSRunCriticalAlertPanel([OCLocalStrings get:@"InvalidDir"], [OCLocalStrings get:@"LDrawNotInDir"], [OCLocalStrings get:@"OK"], nil, nil);
		return false;
	}
	[self updateLDrawDir:[ldrawDirField stringValue] apply:NO];
	for (int i = 0; i < [extraFolders count]; i++)
	{
		extraDirs.push_back([[extraFolders objectAtIndex:i] cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	ldPreferences->setExtraDirs(extraDirs);
	return [super updateLdPreferences];
}

- (IBAction)resetPage:(id)sender
{
	ldPreferences->loadDefaultLDrawSettings(false);
	[super resetPage:sender];
}

- (void)openPanelDidEnd:(NSOpenPanel *)openPanel returnCode:(int)returnCode contextInfo:(void  *)contextInfo
{
	if (returnCode == NSOKButton)
	{
		NSString *filename = [openPanel filename];
		
		if (filename)
		{
			if (contextInfo == ldrawDirField)
			{
				[ldrawDirField setStringValue:filename];
			}
			else
			{
				[extraFolders addObject:filename];
				[extraFoldersTableView reloadData];
			}
			[self valueChanged:self];
		}
	}
}

- (void)addExtraFolder
{
	[self browseForFolder:NULL];
}

- (void)removeExtraFolder
{
	int index = [extraFoldersTableView selectedRow];
	
	if (index >= 0)
	{
		[extraFolders removeObjectAtIndex:index];
	}
	[extraFoldersTableView reloadData];
	[self valueChanged:self];
}

- (IBAction)addRemoveExtraFolder:(id)sender
{
	switch ([addRemoveExtraFolder selectedSegment])
	{
		case 0:
			[self addExtraFolder];
			break;
		case 1:
			[self removeExtraFolder];
			break;
	}
}

- (NSMutableArray *)tableRows:(TableViewReorder *)sender
{
	return extraFolders;
}

- (void)awakeFromNib
{
	NSSegmentedCell *segmentCell = [addRemoveExtraFolder cell];

	[segmentCell setToolTip:@"Add extra search folder" forSegment:0];
	[segmentCell setToolTip:@"Remove extra search folder" forSegment:0];
	tableViewReorder = [[TableViewReorder alloc] initWithTableView:extraFoldersTableView owner:self dragType:@"LDViewExtraDirsDragType"];
	[super awakeFromNib];
}

- (IBAction)extraFolderSelected:(id)sender
{
	[addRemoveExtraFolder setEnabled:[extraFoldersTableView selectedRow] != -1 forSegment:1];
}

- (IBAction)ldrawFolderBrowse:(id)sender
{
	[self browseForFolder:ldrawDirField];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
	return [extraFolders objectAtIndex:rowIndex];
}

- (void)tableViewReorderDidOccur:(id)sender
{
	[self valueChanged:self];
}

@end
