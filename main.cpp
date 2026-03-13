#include <3ds.h>
#include <citro3d.h>
#include <string.h>

// This defines the "corners" of our hallway walls
typedef struct { float pos[3]; float clr[3]; } vertex;

static const vertex hallway_mesh[] = {
    // Floor
    {{ -1.0f, 0.0f, -3.0f }, { 0.2f, 0.1f, 0.0f }}, {{  1.0f, 0.0f, -3.0f }, { 0.2f, 0.1f, 0.0f }}, {{ -1.0f, 0.0f,  0.0f }, { 0.2f, 0.1f, 0.0f }},
    // Left Wall
    {{ -1.0f, 0.0f, -3.0f }, { 0.3f, 0.3f, 0.3f }}, {{ -1.0f, 1.5f, -3.0f }, { 0.3f, 0.3f, 0.3f }}, {{ -1.0f, 0.0f,  0.0f }, { 0.3f, 0.3f, 0.3f }},
};

int main() {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, 0x01001000);

    // Prepare the GPU to handle our hallway shapes
    void* vbo_data = linearAlloc(sizeof(hallway_mesh));
    memcpy(vbo_data, hallway_mesh, sizeof(hallway_mesh));
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000011FF, 0); // Darker blue background
        C3D_FrameDrawOn(target);

        // Tell the 3DS to draw the hallway triangles
        C3D_DrawArrays(GPU_TRIANGLES, 0, 6);

        C3D_FrameEnd(0);
    }

    linearFree(vbo_data);
    C3D_Fini();
    gfxExit();
    return 0;
}
