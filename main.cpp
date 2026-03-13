#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <vector>

// Vertex structure
typedef struct { float position[3]; float color[3]; } vertex;

// The "Hallway" vertices (Floor, Walls, Ceiling)
static const vertex vertex_data[] = {
    // Floor
    {{-1.0f, 0.0f, -10.0f}, {0.2f, 0.1f, 0.0f}}, {{ 1.0f, 0.0f, -10.0f}, {0.2f, 0.1f, 0.0f}}, {{ -1.0f, 0.0f, 0.0f}, {0.2f, 0.1f, 0.0f}},
    // Left Wall
    {{-1.0f, 0.0f, -10.0f}, {0.3f, 0.3f, 0.3f}}, {{-1.0f, 2.0f, -10.0f}, {0.3f, 0.3f, 0.3f}}, {{ -1.0f, 0.0f, 0.0f}, {0.3f, 0.3f, 0.3f}},
    // Right Wall
    {{ 1.0f, 0.0f, -10.0f}, {0.3f, 0.3f, 0.3f}}, {{ 1.0f, 2.0f, -10.0f}, {0.3f, 0.3f, 0.3f}}, {{  1.0f, 0.0f, 0.0f}, {0.3f, 0.3f, 0.3f}},
};

#define WAIT_TIMEOUT 1000000000ULL

int main() {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, GX_TRANSFER_OUT_RAW_RGB(0));

    // Initialize VBO (Vertex Buffer Object)
    void* vbo_data = linearAlloc(sizeof(vertex_data));
    memcpy(vbo_data, vertex_data, sizeof(vertex_data));
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;

        C3D_FrameBegin(C3D_FRAME_SYNCHRONOUS);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0);
        C3D_FrameDrawOn(target);

        // Here is where we draw our hallway!
        C3D_DrawArrays(GPU_TRIANGLES, 0, 9);

        C3D_FrameEnd(0);
    }

    linearFree(vbo_data);
    C3D_Fini();
    gfxExit();
    return 0;
}
