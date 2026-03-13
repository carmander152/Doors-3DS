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

typedef enum { NOT_HIDING, IN_CABINET, UNDER_BED } HideState;

static const vertex level_mesh[] = {
    // --- 1. THE HALLWAY (Index 0-17) ---
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{ -1.0f, 0.0f,  3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{  1.0f, 0.0f,  3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{ -1.0f, 0.0f,  3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
    // Hallway Walls & Roof
    {{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f,  3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f,  3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f,  3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
    {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.1f, 0.1f, 0.1f, 1.0f }}, {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.1f, 0.1f, 0.1f, 1.0f }}, {{ -1.0f, 1.5f,  3.0f, 1.0f }, { 0.1f, 0.1f, 0.1f, 1.0f }},

    // --- 2. ROOM INTERIOR (Index 30-65: Floor, Walls, Roof) ---
    {{ -3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }},
    {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{  3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }}, {{ -3.0f, 0.0f, -3.0f, 1.0f }, { 0.1f, 0.4f, 0.1f, 1.0f }},
    // Room Roof
    {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }}, {{  3.0f, 1.5f, -8.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }}, {{ -3.0f, 1.5f, -3.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }},
    // Room Back Wall
    {{ -3.0f, 0.0f, -8.0f, 1.0f }, { 0.3f, 0.3f, 0.4f, 1.0f }}, {{  3.0f, 0.0f, -8.0f, 1.0f }, { 0.3f, 0.3f, 0.4f, 1.0f }}, {{ -3.0f, 1.5f, -8.0f, 1.0f }, { 0.3f, 0.3f, 0.4f, 1.0f }},

    // --- 3. ELEVATOR (Index 66-101) ---
    {{ -1.0f, 0.0f, 3.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }}, {{  1.0f, 0.0f, 3.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }}, {{ -1.0f, 0.0f, 5.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 1.0f }},
    {{ -1.0f, 1.5f, 3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{ -1.0f, 1.5f, 5.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{ -1.0f, 0.0f, 5.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
    // Elevator Back Wall
    {{ -1.0f, 0.0f, 5.0f, 1.0f }, { 0.15f, 0.15f, 0.15f, 1.0f }}, {{  1.0f, 0.0f, 5.0f, 1.0f }, { 0.15f, 0.15f, 0.15f, 1.0f }}, {{ -1.0f, 1.5f, 5.0f, 1.0f }, { 0.15f, 0.15f, 0.15f, 1.0f }},

    // --- 4. CABINET (Index 102-137) ---
    // Body (102-125)
    {{  1.5f, 0.0f, -6.0f, 1.0f }, { 0.2f, 0.1f, 0.05f, 1.0f }}, {{  2.5f, 0.0f, -6.0f, 1.0f }, { 0.2f, 0.1f, 0.05f, 1.0f }}, {{  1.5f, 1.5f, -6.0f, 1.0f }, { 0.2f, 0.1f, 0.05f, 1.0f }},
    {{  2.5f, 0.0f, -6.0f, 1.0f }, { 0.2f, 0.1f, 0.05f, 1.0f }}, {{  2.5f, 1.5f, -6.0f, 1.0f }, { 0.2f, 0.1f, 0.05f, 1.0f }}, {{  1.5f, 1.5f, -6.0f, 1.0f }, { 0.2f, 0.1f, 0.05f, 1.0f }},
    // Doors (126-137)
    {{  1.52f, 0.0f, -5.95f, 1.0f }, { 0.3f, 0.15f, 0.1f, 1.0f }}, {{  1.98f, 0.0f, -5.95f, 1.0f }, { 0.3f, 0.15f, 0.1f, 1.0f }}, {{  1.52f, 1.5f, -5.95f, 1.0f }, { 0.3f, 0.15f, 0.1f, 1.0f }},
    {{  2.02f, 0.0f, -5.95f, 1.0f }, { 0.3f, 0.15f, 0.1f, 1.0f }}, {{  2.48f, 0.0f, -5.95f, 1.0f }, { 0.3f, 0.15f, 0.1f, 1.0f }}, {{  2.02f, 1.5f, -5.95f, 1.0f }, { 0.3f, 0.15f, 0.1f, 1.0f }},

    // --- 5. BEDS (Index 138+) ---
    {{ -2.8f, 0.0f, -7.0f, 1.0f }, { 0.4f, 0.1f, 0.1f, 1.0f }}, {{ -1.2f, 0.0f, -7.0f, 1.0f }, { 0.4f, 0.1f, 0.1f, 1.0f }}, {{ -2.8f, 0.4f, -7.0f, 1.0f }, { 0.4f, 0.1f, 0.1f, 1.0f }},
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

    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    // Starting Position: Inside Elevator
    float camX = 0.0f, camZ = 4.0f, camYaw = 0.0f, camPitch = 0.0f;
    int frameCount = 0;
    HideState hideState = NOT_HIDING;

    while (aptMainLoop()) {
        hidScanInput();
        irrstScanInput(); 
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        frameCount++;

        // Hiding Proximity
        bool nearCabinet = (camX > 1.0f && camZ < -5.0f && camZ > -7.0f);
        bool nearBed     = (camX < -1.0f && camZ < -5.0f && camZ > -8.0f);

        if (kDown & KEY_X) {
            if (hideState == NOT_HIDING) {
                if (nearCabinet) { hideState = IN_CABINET; camX = 2.0f; camZ = -6.5f; camYaw = 0.0f; }
                else if (nearBed) { hideState = UNDER_BED; camX = -2.0f; camZ = -6.5f; camYaw = 1.57f; }
            } else {
                hideState = NOT_HIDING; camZ += 1.0f;
            }
        }

        float currentHeight = -0.8f;
        if (hideState == NOT_HIDING && frameCount > 120) { // Lock until elevator opens
            circlePosition cStick;
            irrstCstickRead(&cStick);
            if (abs(cStick.dx) > 10) camYaw -= cStick.dx / 1560.0f * 0.15f;
            if (abs(cStick.dy) > 10) camPitch += cStick.dy / 1560.0f * 0.15f;

            circlePosition cPad;
            hidCircleRead(&cPad);
            if (abs(cPad.dy) > 10 || abs(cPad.dx) > 10) {
                float moveSpeed = 0.1f;
                float stickY = cPad.dy / 1560.0f; 
                float stickX = cPad.dx / 1560.0f; 
                camX -= (sinf(camYaw) * stickY - cosf(camYaw) * stickX) * moveSpeed;
                camZ -= (cosf(camYaw) * stickY + sinf(camYaw) * stickX) * moveSpeed;
            }
        } else if (hideState == IN_CABINET) currentHeight = -0.6f;
        else if (hideState == UNDER_BED)    currentHeight = -0.15f;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); 
        C3D_FrameDrawOn(target);

        C3D_Mtx projection;
        Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        C3D_Mtx view;
        Mtx_Identity(&view);
        Mtx_RotateX(&view, -camPitch, true); 
        Mtx_RotateY(&view, -camYaw, true);   
        Mtx_Translate(&view, -camX, currentHeight, -camZ, true); 

        C3D_Mtx projView;
        Mtx_Multiply(&projView, &projection, &view);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projView);
        
        C3D_DrawArrays(GPU_TRIANGLES, 0, 30);   // Hallway & Roof
        C3D_DrawArrays(GPU_TRIANGLES, 30, 36);  // Room & Roof
        C3D_DrawArrays(GPU_TRIANGLES, 66, 36);  // Elevator
        C3D_DrawArrays(GPU_TRIANGLES, 102, 24); // Cabinet Body
        C3D_DrawArrays(GPU_TRIANGLES, 126, 12); // Cabinet Doors
        C3D_DrawArrays(GPU_TRIANGLES, 138, 18); // Beds

        C3D_FrameEnd(0);
    }
    linearFree(vbo_data);
    shaderProgramFree(&program);
    C3D_Fini();
    gfxExit();
    return 0;
}
