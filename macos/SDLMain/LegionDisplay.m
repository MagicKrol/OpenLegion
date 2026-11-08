#import <AppKit/AppKit.h>

// Keep the framebuffer in SDL 1.2's logical pixels on Retina displays.
void LegionUseLogicalOpenGLSurface(void) {
    NSOpenGLContext *context = [NSOpenGLContext currentContext];
    NSView *view = [context view];
    if (view != nil) {
        [view setWantsBestResolutionOpenGLSurface:NO];
        [context update];
    }
}

// Keep a requested window fully inside the visible desktop, including its title bar.
void LegionFitWindowToDisplay(int *width,int *height) {
    NSScreen *screen = [NSScreen mainScreen];
    if (screen == nil || width == NULL || height == NULL || *width <= 0 || *height <= 0) return;
    NSRect available = [screen visibleFrame];
    NSRect frame = [NSWindow frameRectForContentRect:NSMakeRect(0,0,*width,*height)
                                           styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable)];
    CGFloat maxWidth = available.size.width - (frame.size.width - *width);
    CGFloat maxHeight = available.size.height - (frame.size.height - *height);
    if (maxWidth <= 0 || maxHeight <= 0) return;
    CGFloat factor = MIN(1.0, MIN(maxWidth / *width, maxHeight / *height));
    if (factor < 1.0) {
        *width = MAX(1, (int)(*width * factor));
        *height = MAX(1, (int)(*height * factor));
    }
}
