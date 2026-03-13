#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "vshader_shbin.h"

typedef struct { float pos[3]; float clr[3]; } vertex;

static const vertex hallway_mesh[] = {
    // Floor (Brown)
    {{ -1.0f, 0.0f, -3.0f }, { 0.4f, 0.2f, 0.1f }},
    {{  1.0f, 0.0f, -3.0f }, { 0.4f, 0.2f, 0.1f }},
    {{ -1.0f, 0.0f,  0.0f }, { 0.4f, 0.2f, 0.1f }},
    {{  1.0f, 0.0f, -3.0f }, { 0.4f, 0.2f, 0.1f }},
    {{  1.0f, 0.0f,  0.0f }, { 0.4f, 0.2f, 0.1f }},
    {{ -1.0f, 0.0f,  0.0f }, { 0.4f, 0.2f, 0.1f }},
    
    // Left Wall (Grey)
    {{ -1.0f, 0.0f, -3.0f }, { 0.5f, 0.5f, 0.5f }},
    {{ -1.0f, 1.5f, -3.0f }, { 0.5f, 0.5f, 0.5f }},
    {{ -1.0f, 0.0f,  0.0f }, { 0.5f, 0.5f, 0.5f }},
    {{ -1.0f, 1.5f, -3.0f }, { 0.5f, 0.5f, 0.5f }},
    {{ -1.0f, 1.5f,  0.0f }, { 0.5f, 0.5f, 0.5f }},
    {{ -1.0f, 0.0f,  0.0f }, { 0.5f, 0.5f, 0.5f }}
};

int main() {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, 0x01001000);

    // 1. Load the Shader
    DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgram_s program;
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
    C3D_BindProgram(&program);

    // 2. Map our 3D data to the shader
    int uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx");
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // Position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 3); // Color

    void* vbo_data = linearAlloc(sizeof(hallway_mesh));
    memcpy(vbo_data, hallway_mesh, sizeof(hallway_mesh));
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

    // FIX 1: Turn off Face Culling so we don't accidentally hide the walls
    C3D_CullFace(GPU_CULL_NONE);
    // FIX 2: Enable the Depth Buffer so 3D shapes overlap correctly
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        // FIX 3: Change background to a dark spooky blue instead of pitch black
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x102030FF, 0); 
        C3D_FrameDrawOn(target);

        // --- CAMERA MATH ---
        C3D_Mtx projection;
        Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        
        // Move the camera back and slightly up so we aren't standing inside the geometry
        C3D_Mtx view;
        Mtx_Identity(&view);
        Mtx_Translate(&view, 0.0f, -0.5f, 1.5f, true); 

        // Combine the Lens (Projection) and the Position (View)
        C3D_Mtx projView;
        Mtx_Multiply(&projView, &projection, &view);

        // Send to Shader and Draw 12 Vertices (6 for floor, 6 for wall)
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projView);
        C3D_DrawArrays(GPU_TRIANGLES, 0, 12);

        C3D_FrameEnd(0);
    }

    linearFree(vbo_data);
    shaderProgramFree(&program);
    DVLB_Free(vshader_dvlb);
    C3D_Fini();
    gfxExit();
    return 0;
}
