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

// The mesh now has 72 vertices! (We added 12 for the Roofs)
static const vertex level_mesh[] = {
    // --- 1. THE HALLWAY (0 to 17) ---
    // Floor
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    // Left Wall
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    // Right Wall
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
    {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 1.5f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},

    // --- 2. THE DOOR (18 to 23) ---
    {{ -0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{  0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{ -0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
    {{  0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{  0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{ -0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},

    // --- 3. THE ROOM (24 to 59) ---
    // Floor
    {{ -3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }},
    {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }},
    // Left Wall
    {{ -3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }}, {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }}, {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }},
    {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }}, {{ -3.0f, 1.5f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }}, {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.5f, 1.0f }},
    // Right Wall
    {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }}, {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }}, {{  3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }},
    {{  3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }}, {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }}, {{  3.0f, 1.5f, -3.0f, 1.0f }, { 0.1f, 0.2f, 0.4f, 1.0f }},
    // Back Wall
    {{ -3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }}, {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }}, {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }},
    {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }}, {{  3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }}, {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.2f, 0.6f, 1.0f }},
    // Front Inner Walls 
    {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{ -3.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }},
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{ -3.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }},
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }},
    {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{  3.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }}, {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.3f, 0.5f, 1.0f }},

    // --- 4. THE ROOFS (60 to 71) ---
    // Hallway Roof (Dark Grey)
    {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }}, {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }}, {{ -1.0f, 1.5f,  0.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }},
    {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }}, {{  1.0f, 1.5f,  0.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }}, {{ -1.0f, 1.5f,  0.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }},
    // Room Roof (Dark Blue)
    {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.1f, 0.3f, 1.0f }}, {{  3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.1f, 0.3f, 1.0f }}, {{ -3.0f, 1.5f, -3.0f, 1.0f }, { 0.1f, 0.1f, 0.3f, 1.0f }},
    {{  3.0f, 1.5f, -8.0f, 1.0f }, { 0.1f, 0.1f, 0.3f, 1.0f }}, {{  3.0f, 1.5f, -3.0f, 1.0f }, { 0.1f, 0.1f, 0.3f, 1.0f }}, {{ -3.0f, 1.5f, -3.0f, 1.0f }, { 0.1f, 0.1f, 0.3f, 1.0f }},
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
    float camYaw = 0.0f;   // Looking Left/Right
    float camPitch = 0.0f; // NEW: Looking Up/Down

    const float playerRadius = 0.2f;

    while (aptMainLoop()) {
        hidScanInput();
        irrstScanInput(); 

        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        if (kDown & KEY_START) break;

        // --- THE LOOKING CAMERA (C-STICK) ---
        circlePosition cStick;
        irrstCstickRead(&cStick);
        
        // FIX: Inverted C-Stick X so left is left and right is right!
        if (abs(cStick.dx) > 10) camYaw -= cStick.dx / 1560.0f * 0.05f;
        // NEW: Pitch Up and Down with C-Stick Y
        if (abs(cStick.dy) > 10) camPitch += cStick.dy / 1560.0f * 0.05f; 
        
        // D-Pad looking backup
        if (kHeld & KEY_DLEFT)   camYaw += 0.05f;
        if (kHeld & KEY_DRIGHT)  camYaw -= 0.05f;
        if (kHeld & KEY_DUP)     camPitch += 0.05f;
        if (kHeld & KEY_DDOWN)   camPitch -= 0.05f;

        // Stop the player from breaking their neck looking too far up or down!
        if (camPitch > 1.5f)  camPitch = 1.5f;
        if (camPitch < -1.5f) camPitch = -1.5f;

        float nextX = camX;
        float nextZ = camZ;

        // --- TRUE FPS MOVEMENT (CIRCLE PAD) ---
        circlePosition circlePad;
        hidCircleRead(&circlePad);
        
        // Moving Forward/Backward based on exactly where you are looking!
        if (abs(circlePad.dy) > 10) {
            float speed = (circlePad.dy / 1560.0f) * 0.1f;
            nextX += sinf(camYaw) * speed; 
            nextZ -= cosf(camYaw) * speed; 
        }
        // Strafing Left/Right
        if (abs(circlePad.dx) > 10) {
            float speed = (circlePad.dx / 1560.0f) * 0.1f;
            nextX += cosf(camYaw) * speed; 
            nextZ += sinf(camYaw) * speed;
        }

        // --- ZONE-BASED COLLISION ---
        bool doorOpen = (nextZ < -1.5f); 

        // Zone 1: Hallway
        if (nextZ > -3.0f + playerRadius) {
            if (nextX < -1.0f + playerRadius) nextX = -1.0f + playerRadius;
            if (nextX >  1.0f - playerRadius) nextX =  1.0f - playerRadius;
            if (nextZ >  1.5f - playerRadius) nextZ =  1.5f - playerRadius;
            if (!doorOpen && nextZ < -2.8f + playerRadius) nextZ = -2.8f + playerRadius;
        } 
        // Zone 2: Doorway Frame
        else if (nextZ <= -3.0f + playerRadius && nextZ >= -3.0f - playerRadius) {
            if (nextX < -0.8f) nextX = -0.8f;
            if (nextX >  0.8f) nextX =  0.8f;
        }
        // Zone 3: The Big Room
        else {
            if (nextX < -3.0f + playerRadius) nextX = -3.0f + playerRadius;
            if (nextX >  3.0f - playerRadius) nextX =  3.0f - playerRadius;
            if (nextZ < -8.0f + playerRadius) nextZ = -8.0f + playerRadius; 
        }

        camX = nextX;
        camZ = nextZ;

        // --- DRAW THE FRAME ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); // Back to black background!
        C3D_FrameDrawOn(target);

        C3D_Mtx projection;
        Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        
        // FIX: The Magic Matrix Order!
        C3D_Mtx view;
        Mtx_Identity(&view);
        Mtx_RotateX(&view, -camPitch, true); // 1. Pitch the world
        Mtx_RotateY(&view, -camYaw, true);   // 2. Yaw the world
        Mtx_Translate(&view, -camX, -0.8f, -camZ, true); // 3. Shift the world!

        C3D_Mtx projView;
        Mtx_Multiply(&projView, &projection, &view);

        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projView);
        
        // 1. Draw Hallway
        C3D_DrawArrays(GPU_TRIANGLES, 0, 18);
        // 2. Draw Door?
        if (!doorOpen) C3D_DrawArrays(GPU_TRIANGLES, 18, 6);
        // 3. Draw Room
        C3D_DrawArrays(GPU_TRIANGLES, 24, 36);
        // 4. Draw Roofs
        C3D_DrawArrays(GPU_TRIANGLES, 60, 12);

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
