#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdlib.h> // Needed for the abs() math function
#include "vshader_shbin.h"

// 4 coordinates (X, Y, Z, W) stops the GPU from tearing triangles apart
typedef struct { float pos[4]; float clr[4]; } vertex;

static const vertex hallway_mesh[] = {
    // Floor (Brown)
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    
    // Left Wall (Grey)
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    {{ -1.0f, 1.5f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},

    // Right Wall (Darker Grey)
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
    {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
    {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
    {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
    {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
    {{  1.0f, 1.5f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},

    // The Door (Dark Wood Color)
    {{ -0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
    {{  0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
    {{ -0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
    {{  0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
    {{  0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
    {{ -0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
};

int main() {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, 0x01001000);

    DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgram_s program;
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
    C3D_BindProgram(&program);

    int uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx");
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 4); 
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 4); 

    void* vbo_data = linearAlloc(sizeof(hallway_mesh));
    memcpy(vbo_data, hallway_mesh, sizeof(hallway_mesh));
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    // --- CAMERA POSITION VARIABLES ---
    float camX = 0.0f;
    float camZ = 1.0f; 

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // --- CIRCLE PAD LOGIC ---
        circlePosition circlePad;
        hidCircleRead(&circlePad);
        
        // The "10" is a deadzone so it doesn't drift. 
        // 1560.0f slows down the raw analog stick data so you don't fly at lightspeed!
        if (abs(circlePad.dy) > 10) camZ -= circlePad.dy / 1560.0f; 
        if (abs(circlePad.dx) > 10) camX -= circlePad.dx / 1560.0f;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x102030FF, 0); 
        C3D_FrameDrawOn(target);

        C3D_Mtx projection;
        Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        
        // --- APPLY MOVEMENT TO WORLD ---
        C3D_Mtx view;
        Mtx_Identity(&view);
        // We move the world in the reverse direction of our camera to simulate walking
        Mtx_Translate(&view, -camX, -0.8f, -camZ, true); 

        C3D_Mtx projView;
        Mtx_Multiply(&projView, &projection, &view);

        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projView);
        
        // Draw 24 vertices (4 shapes * 6 points)
        C3D_DrawArrays(GPU_TRIANGLES, 0, 24);

        C3D_FrameEnd(0);
    }

    // Cleanup
    linearFree(vbo_data);
    shaderProgramFree(&program);
    DVLB_Free(vshader_dvlb);
    C3D_Fini();
    gfxExit();
    return 0;
}
