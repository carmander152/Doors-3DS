#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdlib.h> 
#include <math.h> // NEW: We need Sine and Cosine for FPS movement!
#include "vshader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float pos[4]; float clr[4]; } vertex;

static const vertex hallway_mesh[] = {
    // Floor
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    
    // Left Wall
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},

    // Right Wall
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
    {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 1.5f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},

    // The Door (Starts at vertex 18)
    {{ -0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{  0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{ -0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
    {{  0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{  0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{ -0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
};

int main() {
    gfxInitDefault();
    gfxSet3D(false); 
    
    // NEW: Initialize the New 3DS C-Stick Hardware
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

    void* vbo_data = linearAlloc(sizeof(hallway_mesh));
    memcpy(vbo_data, hallway_mesh, sizeof(hallway_mesh));
    GSPGPU_FlushDataCache(vbo_data, sizeof(hallway_mesh)); 

    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    // --- PLAYER VARIABLES ---
    float camX = 0.0f;
    float camZ = 1.0f; 
    float camYaw = 0.0f; // This is the angle of our head!

    while (aptMainLoop()) {
        hidScanInput();
        irrstScanInput(); // Scan the C-Stick

        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        if (kDown & KEY_START) break;

        // --- LOOK AROUND (C-STICK & D-PAD) ---
        circlePosition cStick;
        irrstCstickRead(&cStick);
        if (abs(cStick.dx) > 10) camYaw += cStick.dx / 1560.0f * 0.05f;
        if (kHeld & KEY_DLEFT)   camYaw -= 0.05f;
        if (kHeld & KEY_DRIGHT)  camYaw += 0.05f;

        // --- WALK AROUND (CIRCLE PAD) ---
        circlePosition circlePad;
        hidCircleRead(&circlePad);
        
        // Forward / Backward
        if (abs(circlePad.dy) > 10) {
            float speed = (circlePad.dy / 1560.0f) * 0.1f;
            camZ -= cosf(camYaw) * speed;
            camX += sinf(camYaw) * speed; 
        }
        // Strafe Left / Right (Inverted to fix the controls!)
        if (abs(circlePad.dx) > 10) {
            float speed = (circlePad.dx / 1560.0f) * 0.1f;
            camX += cosf(camYaw) * speed; 
            camZ += sinf(camYaw) * speed;
        }

        // --- COLLISION DETECTION ---
        // 1. Wall Collision (Don't let X go past -0.8 or 0.8)
        if (camX < -0.8f) camX = -0.8f;
        if (camX >  0.8f) camX =  0.8f;

        // 2. Start Wall (Don't let Z go backwards past 1.5)
        if (camZ > 1.5f) camZ = 1.5f;

        // 3. The Door Logic!
        int verticesToDraw = 24; // Default: Draw the floor, walls, AND the door
        
        if (camZ < -2.0f) {
            // We are close to the door! Change to 18 so the GPU skips the door vertices!
            verticesToDraw = 18; 
            
            // Invisible wall past the door so you don't walk into the endless void
            if (camZ < -5.0f) camZ = -5.0f; 
        } else {
            // The door is closed. Don't let us walk through it!
            if (camZ < -2.5f) camZ = -2.5f;
        }

        // --- DRAW THE FRAME ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x68B0D8FF, 0); 
        C3D_FrameDrawOn(target);

        C3D_Mtx projection;
        Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        
        C3D_Mtx view;
        Mtx_Identity(&view);
        // Turn the world in the opposite direction of our head, then move it!
        Mtx_RotateY(&view, -camYaw, true); 
        Mtx_Translate(&view, -camX, -0.8f, -camZ, true); 

        C3D_Mtx projView;
        Mtx_Multiply(&projView, &projection, &view);

        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projView);
        
        // Use our dynamic variable! If we are close, it drops from 24 to 18 to "open" the door.
        C3D_DrawArrays(GPU_TRIANGLES, 0, verticesToDraw);

        C3D_FrameEnd(0);
    }

    linearFree(vbo_data);
    shaderProgramFree(&program);
    DVLB_Free(vshader_dvlb);
    C3D_Fini();
    irrstExit(); // Turn off C-Stick hardware
    gfxExit();
    return 0;
}
