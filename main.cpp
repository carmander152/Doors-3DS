#include <3ds.h>
#include <citro3d.h>
#include <string.h>

// Simple vertex structure
typedef struct { float pos[3]; float clr[3]; } vertex;

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

        C3D_FrameBegin(C3D_FRAME_SYNCHRONOUS);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0);
        C3D_FrameDrawOn(target);

        // Draw nothing for now to ensure a successful build
        
        C3D_FrameEnd(0);
    }

    C3D_Fini();
    gfxExit();
    return 0;
}
