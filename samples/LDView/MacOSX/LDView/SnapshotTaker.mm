//
//  SnapshotTaker.mm
//  LDView
//
//  Created by Travis Cobbs on 10/14/07.
//  Copyright 2008 Travis Cobbs. All rights reserved.
//

#import "SnapshotTaker.h"
#include <LDLib/LDrawModelViewer.h>
#include <TCFoundation/TCAlert.h>
#import "SnapshotAlertHandler.h"
#import "AutoDeleter.h"
#include <algorithm>

#define PB_WIDTH 1024
#define PB_HEIGHT 1024

@implementation SnapshotTaker

- (id)init
{
	return [self initWithModelViewer:nil sharedContext:nil];
}

- (BOOL)choosePixelFormat:(CGLPixelFormatObj *)pPixelFormat remote:(bool)remote
{
	int attrs[] =
	{
		kCGLPFAPBuffer,
		kCGLPFAColorSize, 16,
		kCGLPFAAlphaSize, 4,
		kCGLPFADepthSize, 16,
		kCGLPFAStencilSize, 8,
		kCGLPFAMaximumPolicy,
		kCGLPFAAccelerated,
		kCGLPFANoRecovery,
		0,	// Spot for kCGLPFARemotePBuffer if tryRemote is set
		0
	};
	GLint num;

	if (remote)
	{
		attrs[sizeof(attrs) / sizeof(attrs[0]) - 2] = kCGLPFARemotePBuffer;
	}
	if (CGLChoosePixelFormat((CGLPixelFormatAttribute *)attrs, pPixelFormat, &num) == kCGLNoError)
	{
		return YES;
	}
	return NO;
}

- (id)initWithModelViewer:(LDrawModelViewer *)theModelViewer sharedContext:(NSOpenGLContext *)theSharedContext
{
	self = [super init];
	if (self)
	{
		sharedContext = theSharedContext;
		modelViewer = theModelViewer;
		snapshotAlertHandler = new SnapshotAlertHandler(self);
		if (modelViewer)
		{
			ldSnapshotTaker = new LDSnapshotTaker(modelViewer);
		}
	}
	return self;
}

- (void)contextCreatedWithPixelFormat:(CGLPixelFormatObj)pixelFormat
{
	GLint virtualScreen;

	CGLDestroyPixelFormat(pixelFormat);
	CGLSetCurrentContext(context);
	CGLGetVirtualScreen(context, &virtualScreen);
	CGLSetPBuffer(context, pbuffer, 0, 0, virtualScreen);
}

- (void)setupContext
{
	if (CGLCreatePBuffer(PB_WIDTH, PB_HEIGHT, GL_TEXTURE_2D, GL_RGBA, 0, &pbuffer) == kCGLNoError)
	{			
		CGLPixelFormatObj pixelFormat;

		if ([self choosePixelFormat:&pixelFormat remote:sharedContext == nil])
		{
			if (CGLCreateContext(pixelFormat, (CGLContextObj)[sharedContext CGLContextObj], &context) == kCGLNoError)
			{
				[self contextCreatedWithPixelFormat:pixelFormat];
				return;
			}
			CGLDestroyPixelFormat(pixelFormat);
		}
		if (sharedContext == nil)
		{
			NSLog(@"Error creating remote OpenGL context; trying non-remote context.\n");
		}
		if ([self choosePixelFormat:&pixelFormat remote:false])
		{
			if (CGLCreateContext(pixelFormat, (CGLContextObj)[sharedContext CGLContextObj], &context) == kCGLNoError)
			{
				[self contextCreatedWithPixelFormat:pixelFormat];
				return;
			}
			CGLDestroyPixelFormat(pixelFormat);
		}
		if (sharedContext == nil)
		{
			NSLog(@"Error creating OpenGL context for snapshot.\n");
		}
		else
		{
			NSRunAlertPanel(@"Error", @"Error creating OpenGL context for snapshot.", @"OK", nil, nil);
		}
	}
}

- (void)dealloc
{
	TCObject::release(snapshotAlertHandler);
	TCObject::release(ldSnapshotTaker);
	if (context)
	{
		CGLDestroyContext(context);
	}
	if (pbuffer)
	{
		CGLDestroyPBuffer(pbuffer);
	}
	[super dealloc];
}

- (void)setImageType:(LDSnapshotTaker::ImageType)value
{
	ldSnapshotTaker->setImageType(value);
}

- (void)setTrySaveAlpha:(bool)value
{
	ldSnapshotTaker->setTrySaveAlpha(value);
}

- (void)setAutoCrop:(bool)value
{
	ldSnapshotTaker->setAutoCrop(value);
}

- (void)saveFileSetup
{
	CGLSetCurrentContext(context);
	glViewport(0, 0, PB_WIDTH, PB_HEIGHT);
	if (modelViewer)
	{
		modelViewer->perspectiveView();
	}
	glViewport(0, 0, PB_WIDTH, PB_HEIGHT);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glDrawBuffer(GL_FRONT);
	glReadBuffer(GL_FRONT);
	if (modelViewer)
	{
		if (modelViewer->getMainTREModel() == NULL && !modelViewer->getNeedsReload())
		{
			modelViewer->loadModel(true);
		}
	}
}

- (LDSnapshotTaker *)ldSnapshotTaker
{
	return ldSnapshotTaker;
}

- (void)snapshotCallback:(TCAlert *)alert;
{
	if (strcmp(alert->getMessage(), "PreSave") == 0)
	{
		if (!context)
		{
			[self setupContext];
		}
		[self saveFileSetup];
	}
}

- (bool)saveFile
{
	if (ldSnapshotTaker)
	{
		return ldSnapshotTaker->saveImage();
	}
	else
	{
		return LDSnapshotTaker::doCommandLine();
	}
}

- (NSImage *)imageWithWidth:(int)width height:(int)height zoomToFit:(bool)zoomToFit
{
	int actualWidth = width;
	int actualHeight = height;

	ldSnapshotTaker->setTrySaveAlpha(false);
	TCByte *imageData = ldSnapshotTaker->grabImage(actualWidth, actualHeight, zoomToFit, NULL, NULL);
	if (imageData)
	{
		int rowSize = TCImage::roundUp(actualWidth * 3, 4);
		TCByte *imageDataArray[5] = { imageData, NULL, NULL, NULL, NULL };
		NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:imageDataArray pixelsWide:actualWidth pixelsHigh:actualHeight bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace bytesPerRow:rowSize bitsPerPixel:24];
		NSImage *image = [[NSImage alloc] initWithSize:NSMakeSize((float)actualWidth, (float)actualHeight)];
		int y;
		TCByte *tmpRow = new TCByte[rowSize];

		// The good news is that drawing this image to the printer doesn't
		// copy the image data.  The bad news is that means that we can't free
		// the image data until we get back up to the main autorelease pool,
		// so create an AutoDeleter class that will free the memory when it
		// gets released during processing of the main autorelease pool.
		[AutoDeleter autoDeleterWithBytePointer:imageData];
		// Flip the image
		for (y = 0; y < actualHeight / 2; y++)
		{
			TCByte *botRow = &imageData[y * rowSize];
			TCByte *topRow = &imageData[(actualHeight - y - 1) * rowSize];

			memcpy(tmpRow, botRow, rowSize);
			memcpy(botRow, topRow, rowSize);
			memcpy(topRow, tmpRow, rowSize);
		}
		delete tmpRow;
		[image addRepresentation:imageRep];
		[imageRep release];
		return [image autorelease];
	}
	return nil;
}

- (bool)saveFile:(NSString *)filename width:(int)width height:(int)height zoomToFit:(bool)zoomToFit
{
	[self saveFileSetup];
	return ldSnapshotTaker->saveImage([filename cStringUsingEncoding:NSASCIIStringEncoding], width, height, zoomToFit);
}

@end
