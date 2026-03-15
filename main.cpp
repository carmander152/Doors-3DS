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

// Collision Box Structure
typedef struct { float minX, minZ, maxX, maxZ; } BBox;

std::vector<vertex> world_mesh;
std::vector<BBox> collisions;
bool hasKey = false;

// Helper to build the room geometry and collision boxes
void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide) {
    float x2 = x + w, y2 = y + h, z2 = z + d;
    vertex v[] = {
        {{x, y, z, 1}, {r,g,b,1}}, {{x2, y, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}},
        {{x2, y, z, 1}, {r,g,b,1}}, {{x2, y2, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}},
        {{x, y, z2, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        {{x2, y, z2, 1}, {r,g,b,1}}, {{x2, y2, z2, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        {{x, y, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}},
        {{x, y2, z, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}}
    };
    for(int i=0; i<18; i++) world_mesh.push_back(v[i]);
    
    // Create an invisible wall for this box if it's collidable
    if(collide) collisions.push_back({fmin(x,x2), fmin(z,z2), fmax(x,x2), fmax(z,z2)});
}

// Check if the player is trying to walk into a solid object
bool checkCollision(float x, float z) {
    float r = 0.2f; // Player radius
    for(auto& b : collisions) {
        if(x + r > b.minX && x - r < b.maxX && z + r > b.minZ && z - r < b.maxZ) return true;
    }
    return false;
}

// Procedural generation function
void buildWorld() {
    world_mesh.clear(); collisions.clear();
    
    // Lobby Floor & Walls
    addBox(-6, 0, 5, 12, 0.01f, -15, 0.22f, 0.15f, 0.1f, false);
    addBox(-6, 0, 5, 0.1f, 1.8f, -15, 0.3f, 0.3f, 0.3f, true); // Left Wall
    addBox(6, 0, 5, 0.1f, 1.8f, -15, 0.3f, 0.3f, 0.3f, true);  // Right Wall
    
    // Front Desk
    addBox(-5.5f, 0, -2, 4.5f, 0.75f, -1.2f, 0.25f, 0.15f, 0.1f, true);
    
    // The Golden Key (Only draws if you haven't picked it up)
    if(!hasKey) addBox(-5.8f, 1.0f, -4.0f, 0.05f, 0.15f, 0.05f, 1.0f, 0.84f, 0.0f, false);

    // Random Rooms
    for(int i=0; i<8; i++) {
        float z = -10 - (i * 10);
        addBox(-2, 0, z, 4, 0.01f, -10, 0.2f, 0.1f, 0.05f, false); // Floor
        if(rand()%2==0) addBox(1.3f, 0, z-5, 0.6f, 1.4f, -0.6f, 0.3f, 0.2f, 0.1f, true); // Cabinet
        else addBox(-1.8f, 0, z-5, 1.4f, 0.4f, -2.5f, 0.4f, 0.1f, 0.1f, true); // Bed
    }
}

int main() {
    // 1. Initialize Hardware
    gfxInitDefault(); 
    gfxSet3D(false); 
    irrstInit(); 
    srand(time(NULL));
    
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    // 2. Generate the world
    buildWorld();

    // 3. Setup Shaders
    DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgram_s program; 
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]); 
    C3D_BindProgram(&program);

    int uLoc_proj = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx");
    
    C3D_AttrInfo* attr = C3D_GetAttrInfo(); 
    AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 4); 
    AttrInfo_AddLoader(attr, 1, GPU_FLOAT, 4);

    // 4. Setup VBO (Vertex Buffer Object)
    void* vbo_ptr = linearAlloc(world_mesh.size() * sizeof(vertex));
    memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex));
    GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));

    C3D_BufInfo* buf = C3D_GetBufInfo(); 
    BufInfo_Init(buf);
    BufInfo_Add(buf, vbo_ptr, sizeof(vertex), 2, 0x10);
    
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    // 5. TEXENV FIX (The anti-black-screen magic!)
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    // 6. Starting Coordinates
    float camX = 0, camZ = 4, camYaw = 0, camPitch = 0;

    // 7. Main Game Loop
    while (aptMainLoop()) {
        hidScanInput(); 
        irrstScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // Key Pickup Logic
        if(!hasKey && (kDown & KEY_A) && camX < -4.0f && camZ < -3.0f) {
            hasKey = true; 
            buildWorld(); // Rebuild world without the key
            
            // Safely refresh the 3DS Graphics memory
            linearFree(vbo_ptr);
            vbo_ptr = linearAlloc(world_mesh.size() * sizeof(vertex));
            memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex));
            GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
            BufInfo_Init(buf);
            BufInfo_Add(buf, vbo_ptr, sizeof(vertex), 2, 0x10);
        }

        // Camera Looking
        circlePosition cStick, cPad;
        irrstCstickRead(&cStick); 
        hidCircleRead(&cPad);
        
        if (abs(cStick.dx) > 10) camYaw -= cStick.dx / 1560.0f * 0.15f;
        if (abs(cStick.dy) > 10) camPitch += cStick.dy / 1560.0f * 0.15f;
        
        // Movement & Collision
        if (abs(cPad.dy) > 10 || abs(cPad.dx) > 10) {
            float s = 0.1f, sy = cPad.dy/1560.0f, sx = cPad.dx/1560.0f;
            float nextX = camX - (sinf(camYaw) * sy - cosf(camYaw) * sx) * s;
            float nextZ = camZ - (cosf(camYaw) * sy + sinf(camYaw) * sx) * s;
            
            // Apply movement only if the next step isn't inside a solid box
            if(!checkCollision(nextX, camZ)) camX = nextX;
            if(!checkCollision(camX, nextZ)) camZ = nextZ;
        }

        // 8. Draw the Frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); 
        C3D_FrameDrawOn(target);

        // Matrix Math for 3D perspective
        C3D_Mtx proj, view;
        Mtx_PerspTilt(&proj, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        Mtx_Identity(&view);
        Mtx_RotateX(&view, -camPitch, true); 
        Mtx_RotateY(&view, -camYaw, true);
        Mtx_Translate(&view, -camX, -0.75f, -camZ, true);
        Mtx_Multiply(&view, &proj, &view);
        
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_proj, &view);
        C3D_DrawArrays(GPU_TRIANGLES, 0, world_mesh.size());
        
        C3D_FrameEnd(0);
    }
    
    // 9. Cleanup
    linearFree(vbo_ptr); 
    C3D_Fini(); 
    gfxExit();
    return 0;
}
