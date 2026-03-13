#include <3ds.h>
#include <citro3d.h>
#include <string.h>

int main() {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    
    // Setup the screen
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, 0x01001000);

    // Main Loop
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break; // Exit on Start

        // FIXED LINE BELOW: Changed SYNCHRONOUS to SYNCDRAW
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW); 
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0);
        C3D_FrameDrawOn(target);

        // Frame rendering logic would go here
        
        C3D_FrameEnd(0);
    }

    C3D_Fini();
    gfxExit();
    return 0;
}
