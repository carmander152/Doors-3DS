#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdlib.h> 
#include <math.h> 
#include "vshader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float pos[4]; float clr[4]; } vertex;

// We now have 60 vertices! (Hallway + Door + Room)
static const vertex level_mesh[] = {
    // --- THE HALLWAY ---
    // Floor
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    // Left Wall
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    // Right Wall
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
    {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 1.5f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},

    // --- THE DOOR (Vertices 18 to 23) ---
    {{ -0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{  0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{ -0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
    {{  0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{  0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{ -0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},

    // --- THE NEW ROOM ---
    // Room Floor (Green, extending from Z=-3 to Z=-8, Width X=-3 to X=3)
    {{ -3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }},
    {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }},
    
    // Room Left Wall (Blue)
    {{ -3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }}, {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }}, {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }},
    {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }}, {{ -3.0f, 1.5f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }}, {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }},

    // Room Right Wall (Blue)
    {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }}, {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }}, {{  3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }},
    {{  3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }}, {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }}, {{  3.0f, 1.5f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }},

    // Room Back Wall (Blue)
    {{ -3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }}, {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }}, {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }},
    {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }}, {{  3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }}, {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }},
    
    // Front Inner Walls (Beside the door, so you can't look out into the void)
    {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{ -3.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }},
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{ -3.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }},
    
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }},
    {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{  3.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }},
};

int main() {
    gfxInitDefault();
    gfxSet3D(false); 
    irrstInit(); 

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

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

    // Update memory allocation for the larger level_mesh!
    void* vbo_data = linearAlloc(sizeof(level_mesh));
    memcpy(vbo_data, level_mesh, sizeof(level_mesh));
    GSPGPU_FlushDataCache(vbo_data, sizeof(level_mesh)); 

    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    float camX = 0.0f;
    float camZ = 1.0f; 
    float camYaw = 0.0f; 

    // Define collision bounds
    const float playerRadius = 0.2f;

    while (aptMainLoop()) {
        hidScanInput();
        irrstScanInput(); 

        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        if (kDown & KEY_START) break;

        circlePosition cStick;
        irrstCstickRead(&cStick);
        if (abs(cStick.dx) > 10) camYaw += cStick.dx / 1560.0f * 0.05f;
        if (kHeld & KEY_DLEFT)   camYaw -= 0.05f;
        if (kHeld & KEY_DRIGHT)  camYaw += 0.05f;

        float nextX = camX;
        float nextZ = camZ;

        circlePosition circlePad;
        hidCircleRead(&circlePad);
        
        if (abs(circlePad.dy) > 10) {
            float speed = (circlePad.dy / 1560.0f) * 0.1f;
            nextZ -= cosf(camYaw) * speed;
            nextX += sinf(camYaw) * speed; 
        }
        if (abs(circlePad.dx) > 10) {
            float speed = (circlePad.dx / 1560.0f) * 0.1f;
            nextX += cosf(camYaw) * speed; 
            nextZ += sinf(camYaw) * speed;
        }

        // --- ZONE-BASED COLLISION ---
        bool doorOpen = (nextZ < -1.5f); // Door opens when you get close!

        // Zone 1: The Hallway (Z is greater than -3.0)
        if (nextZ > -3.0f + playerRadius) {
            if (nextX < -1.0f + playerRadius) nextX = -1.0f + playerRadius;
            if (nextX >  1.0f - playerRadius) nextX =  1.0f - playerRadius;
            if (nextZ >  1.5f - playerRadius) nextZ =  1.5f - playerRadius; // Start wall
            
            // If the door is closed, don't let them reach Z=-3.0
            if (!doorOpen && nextZ < -2.8f + playerRadius) {
                nextZ = -2.8f + playerRadius;
            }
        } 
        // Zone 2: The Doorway frame (Crossing between hall and room)
        else if (nextZ <= -3.0f + playerRadius && nextZ >= -3.0f - playerRadius) {
            // Force them to squeeze through the door gap (X between -1 and 1)
            if (nextX < -0.8f) nextX = -0.8f;
            if (nextX >  0.8f) nextX =  0.8f;
        }
        // Zone 3: The New Room (Z is less than -3.0)
        else {
            if (nextX < -3.0f + playerRadius) nextX = -3.0f + playerRadius;
            if (nextX >  3.0f - playerRadius) nextX =  3.0f - playerRadius;
            if (nextZ < -8.0f + playerRadius) nextZ = -8.0f + playerRadius; // Back wall
        }

        // Apply calculated movement
        camX = nextX;
        camZ = nextZ;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x68B0D8FF, 0); 
        C3D_FrameDrawOn(target);

        C3D_Mtx projection;
        Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        
        C3D_Mtx view;
        Mtx_Identity(&view);
        Mtx_RotateY(&view, -camYaw, true); 
        Mtx_Translate(&view, -camX, -0.8f, -camZ, true); 

        C3D_Mtx projView;
        Mtx_Multiply(&projView, &projection, &view);

        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projView);
        
        // --- DRAWING LOGIC ---
        // 1. Draw Hallway (Vertices 0 to 18)
        C3D_DrawArrays(GPU_TRIANGLES, 0, 18);
        
        // 2. Draw Door? (Vertices 18 to 24)
        if (!doorOpen) {
            C3D_DrawArrays(GPU_TRIANGLES, 18, 6);
        }

        // 3. Draw Room (Vertices 24 to 60)
        C3D_DrawArrays(GPU_TRIANGLES, 24, 36);

        C3D_FrameEnd(0);
    }

    linearFree(vbo_data);
    shaderProgramFree(&program);
    DVLB_Free(vshader_dvlb);
    C3D_Fini();
    irrstExit(); 
    gfxExit();
    return 0;
}
