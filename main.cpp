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

std::vector<vertex> world_vbo;

// Clean function to build boxes (walls, desks, beds)
void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b) {
    float x2 = x + w, y2 = y + h, z2 = z + d;
    vertex v[] = {
        {{x, y, z, 1}, {r,g,b,1}}, {{x2, y, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}},
        {{x2, y, z, 1}, {r,g,b,1}}, {{x2, y2, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}},
        {{x, y, z2, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        {{x2, y, z2, 1}, {r,g,b,1}}, {{x2, y2, z2, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        {{x, y, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}},
        {{x, y2, z, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}}
    };
    for(int i=0; i<18; i++) world_vbo.push_back(v[i]);
}

void buildHotelLobby() {
    // Floor & Ceiling (Lobby Colors)
    addBox(-6, 0, 5, 12, 0.01f, -15, 0.2f, 0.12f, 0.08f); // Wood/Carpet
    addBox(-6, 1.8f, 5, 12, 0.01f, -15, 0.15f, 0.15f, 0.15f); // Ceiling
    
    // Front Desk (Left side)
    addBox(-5.5f, 0, -2, 4.5f, 0.7f, -1.2f, 0.3f, 0.2f, 0.1f); 
    addBox(-5.5f, 0.7f, -2, 4.5f, 0.05f, -1.2f, 0.4f, 0.3f, 0.2f); // Counter Top
    
    // Key Rack behind desk
    addBox(-5.9f, 0.8f, -4.0f, 0.1f, 0.6f, 1.5f, 0.1f, 0.05f, 0.02f);
}

void buildRandomRoom(float zOffset) {
    addBox(-2, 0, zOffset, 4, 0.01f, -10, 0.2f, 0.1f, 0.05f); // Hall floor
    if(rand() % 2 == 0) addBox(1.3f, 0, zOffset - 5, 0.6f, 1.4f, -0.6f, 0.25f, 0.15f, 0.05f); // Cabinet
    else addBox(-1.8f, 0, zOffset - 5, 1.3f, 0.4f, -2.6f, 0.45f, 0.1f, 0.1f); // Bed
}

int main() {
    gfxInitDefault(); gfxSet3D(false); irrstInit(); srand(time(NULL));
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    // Build World: Fixed Lobby + 10 Random Rooms
    buildHotelLobby();
    for(int i=0; i<10; i++) buildRandomRoom(-10 - (i*10));

    DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgram_s program; shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]); C3D_BindProgram(&program);

    int uLoc_proj = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx");
    C3D_AttrInfo* attr = C3D_GetAttrInfo(); AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 4); AttrInfo_AddLoader(attr, 1, GPU_FLOAT, 4); 

    void* vbo_ptr = linearAlloc(world_vbo.size() * sizeof(vertex));
    memcpy(vbo_ptr, world_vbo.data(), world_vbo.size() * sizeof(vertex));
    GSPGPU_FlushDataCache(vbo_ptr, world_vbo.size() * sizeof(vertex)); 

    C3D_BufInfo* buf = C3D_GetBufInfo(); BufInfo_Init(buf);
    BufInfo_Add(buf, vbo_ptr, sizeof(vertex), 2, 0x10);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    float camX = 0, camZ = 4, camYaw = 0, camPitch = 0, doorSlide = 0;
    int frames = 0;

    while (aptMainLoop()) {
        hidScanInput(); irrstScanInput();
        if (hidKeysDown() & KEY_START) break;

        frames++;
        if (frames > 60 && doorSlide < 1.0f) doorSlide += 0.02f;

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
        Mtx_Translate(&view, -camX, -0.75f, -camZ, true);
        Mtx_Multiply(&view, &proj, &view);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_proj, &view);
        
        C3D_DrawArrays(GPU_TRIANGLES, 0, world_vbo.size());
        C3D_FrameEnd(0);
    }
    linearFree(vbo_ptr); C3D_Fini(); gfxExit();
    return 0;
}
