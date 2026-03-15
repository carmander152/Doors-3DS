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

// Increased buffer size since we are drawing full 6-sided boxes now
#define MAX_VERTS 20000 

typedef struct { float pos[4]; float clr[4]; } vertex;
typedef struct { float minX, minZ, maxX, maxZ; } BBox;

std::vector<vertex> world_mesh;
std::vector<BBox> collisions;

bool hasKey = false;
int roomSequence[100]; 

// FIXED: Now draws all 6 sides (36 vertices) of the box!
void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide) {
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
        {{x2, y2, z, 1}, {r,g,b,1}}, {{x2, y2, z2, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}},
        // Top
        {{x, y2, z, 1}, {r,g,b,1}}, {{x2, y2, z, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        {{x2, y2, z, 1}, {r,g,b,1}}, {{x2, y2, z2, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        // Bottom
        {{x, y, z, 1}, {r,g,b,1}}, {{x2, y, z, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}},
        {{x2, y, z, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}}
    };
    for(int i=0; i<36; i++) world_mesh.push_back(v[i]);
    if(collide) collisions.push_back({fmin(x,x2), fmin(z,z2), fmax(x,x2), fmax(z,z2)});
}

// Builds the wall divider and the door itself
void addWallWithDoor(float z, bool isFirstDoor, bool playerHasKey) {
    // Left and Right wall frames
    addBox(-2.0f, 0.0f, z, 1.4f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true);
    addBox(0.6f, 0.0f, z, 1.4f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true);
    // Top frame (above the door)
    addBox(-0.6f, 1.2f, z, 1.2f, 0.6f, -0.2f, 0.2f, 0.15f, 0.1f, false);

    if (isFirstDoor && !playerHasKey) {
        // Solid locked door blocking the way
        addBox(-0.6f, 0.0f, z, 1.2f, 1.2f, -0.1f, 0.3f, 0.1f, 0.05f, true);
    } else {
        // An "open" door swung into the next room
        addBox(-0.6f, 0.0f, z, 0.1f, 1.2f, -1.2f, 0.3f, 0.1f, 0.05f, true);
    }
}

bool checkCollision(float x, float z) {
    float r = 0.2f; 
    for(auto& b : collisions) {
        if(x + r > b.minX && x - r < b.maxX && z + r > b.minZ && z - r < b.maxZ) return true;
    }
    return false;
}

void buildWorld(int currentChunk) {
    world_mesh.clear(); 
    collisions.clear();
    
    // The Starting Lobby
    if (currentChunk < 2) {
        addBox(-6, 0, 5, 12, 0.01f, -15, 0.22f, 0.15f, 0.1f, false); // Floor
        addBox(-6, 1.8f, 5, 12, 0.01f, -15, 0.1f, 0.1f, 0.1f, false); // Ceiling
        addBox(-6, 0, 5, 0.1f, 1.8f, -15, 0.3f, 0.3f, 0.3f, true); // Left Wall
        addBox(6, 0, 5, 0.1f, 1.8f, -15, 0.3f, 0.3f, 0.3f, true);  // Right Wall
        
        // Front Desk & Key
        addBox(-5.5f, 0, -2, 4.5f, 0.75f, -1.2f, 0.25f, 0.15f, 0.1f, true); 
        if(!hasKey) addBox(-5.8f, 1.0f, -4.0f, 0.05f, 0.15f, 0.05f, 1.0f, 0.84f, 0.0f, false);
    }

    int startRoom = currentChunk - 1;
    int endRoom = currentChunk + 2;
    if (startRoom < 0) startRoom = 0;
    if (endRoom > 99) endRoom = 99;

    for(int i = startRoom; i <= endRoom; i++) {
        float z = -10 - (i * 10);
        
        // Add the doorway! If i == 0, it's the first door and requires the key.
        addWallWithDoor(z, (i == 0), hasKey);

        // Room Shell
        addBox(-2, 0, z, 4, 0.01f, -10, 0.2f, 0.1f, 0.05f, false); // Floor
        addBox(-2, 1.8f, z, 4, 0.01f, -10, 0.15f, 0.15f, 0.15f, false); // Ceiling
        addBox(-2, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true); // Left Wall
        addBox(1.9f, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true); // Right Wall

        // Furniture
        if(roomSequence[i] == 0) {
            addBox(1.3f, 0, z-5, 0.6f, 1.4f, -0.6f, 0.3f, 0.2f, 0.1f, true); // Cabinet
        } else {
            addBox(-1.8f, 0, z-5, 1.4f, 0.4f, -2.5f, 0.4f, 0.1f, 0.1f, true); // Bed
        }
    }
}

int main() {
    gfxInitDefault(); gfxSet3D(false); irrstInit(); srand(time(NULL));
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    for(int i=0; i<100; i++) roomSequence[i] = rand() % 2;

    int currentChunk = 0;
    buildWorld(currentChunk);

    DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgram_s program; shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]); C3D_BindProgram(&program);

    int uLoc_proj = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx");
    C3D_AttrInfo* attr = C3D_GetAttrInfo(); AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 4); AttrInfo_AddLoader(attr, 1, GPU_FLOAT, 4);

    void* vbo_ptr = linearAlloc(MAX_VERTS * sizeof(vertex));
    memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex));
    GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));

    C3D_BufInfo* buf = C3D_GetBufInfo(); BufInfo_Init(buf);
    BufInfo_Add(buf, vbo_ptr, sizeof(vertex), 2, 0x10);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    float camX = 0, camZ = 4, camYaw = 0, camPitch = 0;

    while (aptMainLoop()) {
        hidScanInput(); irrstScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // Key Collection
        if(!hasKey && (kDown & KEY_A) && camX < -4.0f && camZ < -3.0f) {
            hasKey = true; 
            buildWorld(currentChunk);
            memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex));
            GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
        }

        // Room Chunking
        int newChunk = 0;
        if (camZ < -10.0f) {
            newChunk = (int)((abs(camZ) - 10.0f) / 10.0f) + 1;
        }
        
        if (newChunk != currentChunk) {
            currentChunk = newChunk;
            buildWorld(currentChunk);
            memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex));
            GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
        }

        // Camera Looking
        circlePosition cStick, cPad;
        irrstCstickRead(&cStick); hidCircleRead(&cPad);
        if (abs(cStick.dx) > 10) camYaw -= cStick.dx / 1560.0f * 0.15f;
        if (abs(cStick.dy) > 10) camPitch += cStick.dy / 1560.0f * 0.15f;
        
        // Movement (Speed increased from 0.1f to 0.2f)
        if (abs(cPad.dy) > 10 || abs(cPad.dx) > 10) {
            float s = 0.2f; 
            float sy = cPad.dy/1560.0f, sx = cPad.dx/1560.0f;
            float nextX = camX - (sinf(camYaw) * sy - cosf(camYaw) * sx) * s;
            float nextZ = camZ - (cosf(camYaw) * sy + sinf(camYaw) * sx) * s;
            if(!checkCollision(nextX, camZ)) camX = nextX;
            if(!checkCollision(camX, nextZ)) camZ = nextZ;
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
        C3D_DrawArrays(GPU_TRIANGLES, 0, world_mesh.size());
        
        C3D_FrameEnd(0);
    }
    
    linearFree(vbo_ptr); C3D_Fini(); gfxExit();
    return 0;
}
