#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdlib.h> 
#include <math.h> 
#include <vector>
#include <time.h>
#include "vshader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float pos[4]; float clr[4]; } vertex;
typedef enum { NOT_HIDING, IN_CABINET, UNDER_BED } HideState;

std::vector<vertex> world_vbo;

void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b) {
    float x2 = x + w, y2 = y + h, z2 = z + d;
    vertex v[] = {
        // Front
        {{x, y, z, 1}, {r,g,b,1}}, {{x2, y, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}},
        {{x2, y, z, 1}, {r,g,b,1}}, {{x2, y2, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}},
        // Back
        {{x, y, z2, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        {{x2, y, z2, 1}, {r,g,b,1}}, {{x2, y2, z2, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        // Left
        {{x, y, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}},
        {{x, y2, z, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}},
        // Right
        {{x2, y, z, 1}, {r,g,b,1}}, {{x2, y2, z, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}},
        {{x2, y2, z, 1}, {r,g,b,1}}, {{x2, y2, z2, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}}
    };
    for(int i=0; i<24; i++) world_vbo.push_back(v[i]);
}

void buildSpawnLobby() {
    // Floor & Ceiling
    addBox(-5, 0, 3, 10, 0.01f, -13, 0.15f, 0.08f, 0.05f); // Dark Carpet
    addBox(-5, 1.5f, 3, 10, 0.01f, -13, 0.1f, 0.1f, 0.1f);   // Ceiling
    
    // Front Desk (Left side like the image)
    addBox(-4.5f, 0, -2, 4, 0.6f, -1.5f, 0.25f, 0.15f, 0.05f); 
    addBox(-4.5f, 0.6f, -2, 4, 0.05f, -1.5f, 0.35f, 0.25f, 0.15f); // Counter top
    
    // Key Rack behind desk
    addBox(-4.9f, 0.7f, -3.8f, 0.1f, 0.6f, 1.2f, 0.1f, 0.05f, 0.02f);
    
    // Lobby Walls
    addBox(-5, 0, 3, 0.1f, 1.5f, -13, 0.15f, 0.2f, 0.15f); // Left wall
    addBox(5, 0, 3, 0.1f, 1.5f, -13, 0.15f, 0.2f, 0.15f);  // Right wall
}

void buildRandomRoom(float startZ) {
    // Basic Hallway Connector
    addBox(-2, 0, startZ, 4, 0.01f, -10, 0.2f, 0.1f, 0.05f); 
    addBox(-2, 1.5f, startZ, 4, 0.01f, -10, 0.1f, 0.1f, 0.1f);
    
    int type = rand() % 2;
    if (type == 0) {
        // Cabinet Room
        addBox(1.2f, 0, startZ - 4, 0.6f, 1.3f, -0.6f, 0.2f, 0.1f, 0.05f);
    } else {
        // Bed Room
        addBox(-1.8f, 0, startZ - 5, 1.2f, 0.4f, -2.5f, 0.4f, 0.1f, 0.1f);
    }
}

int main() {
    gfxInitDefault(); gfxSet3D(false); irrstInit(); srand(time(NULL));
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    // GENERATE WORLD
    buildSpawnLobby();
    for(int i=0; i<10; i++) buildRandomRoom(-10 - (i*10));

    DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgram_s program; shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]); C3D_BindProgram(&program);

    int uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx");
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo(); AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 4); AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 4); 

    void* vbo_data = linearAlloc(world_vbo.size() * sizeof(vertex));
    memcpy(vbo_data, world_vbo.data(), world_vbo.size() * sizeof(vertex));
    GSPGPU_FlushDataCache(vbo_data, world_vbo.size() * sizeof(vertex)); 

    C3D_BufInfo* bufInfo = C3D_GetBufInfo(); BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    float camX = 0, camZ = 4, camYaw = 0, camPitch = 0;
    
    while (aptMainLoop()) {
        hidScanInput(); irrstScanInput();
        if (hidKeysDown() & KEY_START) break;

        circlePosition cStick, cPad;
        irrstCstickRead(&cStick); hidCircleRead(&cPad);
        
        if (abs(cStick.dx) > 10) camYaw -= cStick.dx / 1560.0f * 0.15f;
        if (abs(cStick.dy) > 10) camPitch += cStick.dy / 1560.0f * 0.15f;
        
        if (abs(cPad.dy) > 10 || abs(cPad.dx) > 10) {
            float s = 0.12f, sy = cPad.dy/1560.0f, sx = cPad.dx/1560.0f;
            camX -= (sinf(camYaw) * sy - cosf(camYaw) * sx) * s;
            camZ -= (cosf(camYaw) * sy + sinf(camYaw) * sx) * s;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); 
        C3D_FrameDrawOn(target);

        C3D_Mtx proj, view;
        Mtx_PerspTilt(&proj, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        Mtx_Identity(&view);
        Mtx_RotateX(&view, -camPitch, true); Mtx_RotateY(&view, -camYaw, true);
        Mtx_Translate(&view, -camX, -0.7f, -camZ, true);
        Mtx_Multiply(&view, &proj, &view);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &view);
        
        C3D_DrawArrays(GPU_TRIANGLES, 0, world_vbo.size());
        C3D_FrameEnd(0);
    }
    linearFree(vbo_data);
    C3D_Fini(); gfxExit();
    return 0;
}
