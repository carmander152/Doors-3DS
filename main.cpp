#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h> 
#include <math.h> 
#include <vector>
#include <time.h>
#include "vshader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define MAX_VERTS 25000 

typedef struct { float pos[4]; float clr[4]; } vertex;
typedef struct { float minX, minY, minZ, maxX, maxY, maxZ; } BBox;

typedef enum { NOT_HIDING, IN_CABINET, UNDER_BED } HideState;

std::vector<vertex> world_mesh;
std::vector<BBox> collisions;

// --- GAME STATE VARIABLES ---
bool hasKey = false;
bool firstDoorUnlocked = false;
int roomSequence[100]; 
bool doorOpen[100] = {false}; 
bool isCrouching = false;

// 3D Box Builder with Height Tracking
void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide) {
    float x2 = x + w, y2 = y + h, z2 = z + d;
    vertex v[] = {
        {{x, y, z, 1}, {r,g,b,1}}, {{x2, y, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}},
        {{x2, y, z, 1}, {r,g,b,1}}, {{x2, y2, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}},
        {{x, y, z2, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        {{x2, y, z2, 1}, {r,g,b,1}}, {{x2, y2, z2, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        {{x, y, z, 1}, {r,g,b,1}}, {{x, y2, z, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}},
        {{x, y2, z, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}},
        {{x2, y, z, 1}, {r,g,b,1}}, {{x2, y2, z, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}},
        {{x2, y2, z, 1}, {r,g,b,1}}, {{x2, y2, z2, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}},
        {{x, y2, z, 1}, {r,g,b,1}}, {{x2, y2, z, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        {{x2, y2, z, 1}, {r,g,b,1}}, {{x2, y2, z2, 1}, {r,g,b,1}}, {{x, y2, z2, 1}, {r,g,b,1}},
        {{x, y, z, 1}, {r,g,b,1}}, {{x2, y, z, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}},
        {{x2, y, z, 1}, {r,g,b,1}}, {{x2, y, z2, 1}, {r,g,b,1}}, {{x, y, z2, 1}, {r,g,b,1}}
    };
    for(int i=0; i<36; i++) world_mesh.push_back(v[i]);
    if(collide) collisions.push_back({fmin(x,x2), fmin(y,y2), fmin(z,z2), fmax(x,x2), fmax(y,y2), fmax(z,z2)});
}

// True 3D Collision! (Checks X, Z, and Y Head Clearance)
bool checkCollision(float x, float y, float z, float h) {
    float r = 0.2f; 
    for(auto& b : collisions) {
        if(x + r > b.minX && x - r < b.maxX && z + r > b.minZ && z - r < b.maxZ) {
            if(y + h > b.minY && y < b.maxY) return true; // Bonk!
        }
    }
    return false;
}

void buildWorld(int currentChunk) {
    world_mesh.clear(); 
    collisions.clear();
    
    if (currentChunk < 2) {
        // Lobby Floor & Ceiling
        addBox(-6, 0, 5, 12, 0.01f, -15, 0.22f, 0.15f, 0.1f, false); 
        addBox(-6, 1.8f, 5, 12, 0.01f, -15, 0.1f, 0.1f, 0.1f, false); 
        
        // Lobby Outer Walls
        addBox(-6, 0, 5, 0.1f, 1.8f, -15, 0.3f, 0.3f, 0.3f, true); 
        addBox(6, 0, 5, 0.1f, 1.8f, -15, 0.3f, 0.3f, 0.3f, true);  
        
        // Front Transition Walls (Enclosing the Lobby!)
        addBox(-6.0f, 0, -10.0f, 4.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true); // Left of door
        addBox(2.0f, 0, -10.0f, 4.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true); // Right of door

        // --- ELEVATOR WALL ---
        addBox(-6.0f, 0, 5.0f, 2.4f, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true); 
        addBox(-3.6f, 0, 4.9f, 1.2f, 1.5f, 0.2f, 0.4f, 0.2f, 0.1f, true); 
        addBox(-3.5f, 0, 4.8f, 1.0f, 1.4f, 0.2f, 0.5f, 0.5f, 0.5f, true); 
        addBox(-2.4f, 0, 5.0f, 1.8f, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true); 
        addBox(-0.6f, 0, 4.9f, 1.2f, 1.5f, 0.2f, 0.4f, 0.2f, 0.1f, true); 
        addBox(-0.5f, 0, 4.8f, 1.0f, 1.4f, 0.2f, 0.5f, 0.5f, 0.5f, true); 
        addBox(0.6f, 0, 5.0f, 1.8f, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true); 
        addBox(2.4f, 0, 4.9f, 1.2f, 1.5f, 0.2f, 0.4f, 0.2f, 0.1f, true); 
        addBox(2.5f, 0, 4.8f, 1.0f, 1.4f, 0.2f, 0.5f, 0.5f, 0.5f, true); 
        addBox(3.6f, 0, 5.0f, 2.4f, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true); 
        
        // --- THE KEY TRAP ---
        // Desk (Moved to seal the right side of the trap)
        addBox(-4.2f, 0.0f, -8.0f, 3.0f, 0.75f, -2.0f, 0.25f, 0.15f, 0.1f, true); 

        // Luggage Trolley (Forms the tunnel between the Left Wall and the Desk)
        addBox(-5.8f, 0.1f, -8.0f, 1.6f, 0.1f, -1.8f, 0.7f, 0.6f, 0.1f, false); // Base
        addBox(-5.8f, 0.6f, -8.0f, 1.6f, 1.0f, -1.8f, 0.4f, 0.2f, 0.2f, true);  // Luggage (Solid Block!)

        // The Key (Trapped behind the trolley!)
        if(!hasKey && !firstDoorUnlocked) {
            addBox(-5.2f, 0.8f, -9.9f, 0.2f, 0.2f, 0.05f, 0.3f, 0.2f, 0.1f, false); // Hook
            addBox(-5.1f, 0.6f, -9.85f, 0.05f, 0.15f, 0.05f, 1.0f, 0.84f, 0.0f, false); // Golden Key
        }
    }

    int startRoom = currentChunk - 1;
    int endRoom = currentChunk + 2;
    if (startRoom < 0) startRoom = 0;
    if (endRoom > 99) endRoom = 99;

    for(int i = startRoom; i <= endRoom; i++) {
        float z = -10 - (i * 10);
        
        // --- DOORWAY & ROOM PLAQUES ---
        addBox(-2.0f, 0.0f, z, 1.4f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true); 
        addBox(0.6f, 0.0f, z, 1.4f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true);  
        addBox(-0.6f, 1.4f, z, 1.2f, 0.4f, -0.2f, 0.2f, 0.15f, 0.1f, false); 
        
        // Gold Room Number Plaque above the door
        addBox(-0.2f, 1.25f, z+0.05f, 0.4f, 0.12f, 0.02f, 0.8f, 0.7f, 0.2f, false);

        if (!doorOpen[i]) {
            addBox(-0.6f, 0.0f, z, 1.2f, 1.4f, -0.1f, 0.15f, 0.08f, 0.05f, true);
            // Padlock on Door 001
            if (i == 0 && !firstDoorUnlocked) {
                addBox(-0.1f, 0.7f, z+0.05f, 0.2f, 0.2f, 0.05f, 0.7f, 0.7f, 0.7f, false);
            }
        } else {
            addBox(-0.6f, 0.0f, z, 0.1f, 1.4f, -1.2f, 0.3f, 0.15f, 0.08f, true);
        }

        addBox(-2, 0, z, 4, 0.01f, -10, 0.2f, 0.1f, 0.05f, false); 
        addBox(-2, 1.8f, z, 4, 0.01f, -10, 0.15f, 0.15f, 0.15f, false); 
        addBox(-2, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true); 
        addBox(1.9f, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true); 

        if(roomSequence[i] == 0) {
            // CABINET
            addBox(1.8f, 0, z-6.0f, 0.1f, 1.5f, 1.0f, 0.3f, 0.18f, 0.1f, false); 
            addBox(1.2f, 1.5f, z-6.0f, 0.7f, 0.1f, 1.0f, 0.3f, 0.18f, 0.1f, false); 
            addBox(1.2f, 0, z-6.0f, 0.7f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false); 
            addBox(1.2f, 0, z-5.1f, 0.7f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false); 
            addBox(1.2f, 0, z-5.9f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false); 
            addBox(1.2f, 0, z-5.45f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false); 
            collisions.push_back({1.2f, 0.0f, z-6.0f, 1.9f, 1.5f, z-5.0f});
        } else {
            // BED
            addBox(-1.9f, 0.4f, z-6.5f, 1.4f, 0.1f, -2.5f, 0.4f, 0.1f, 0.1f, true); 
            addBox(-1.9f, 0.0f, z-6.5f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true); 
            addBox(-0.6f, 0.0f, z-6.5f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true);
            addBox(-1.9f, 0.0f, z-8.9f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true);
            addBox(-0.6f, 0.0f, z-8.9f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true);
            addBox(-0.6f, 0.2f, z-6.5f, 0.1f, 0.2f, -2.5f, 0.4f, 0.1f, 0.1f, true);
            collisions.push_back({-1.9f, 0.0f, z-9.0f, -0.5f, 0.6f, z-6.5f});
        }
    }
}

int main() {
    gfxInitDefault(); gfxSet3D(false); irrstInit(); srand(time(NULL));
    consoleInit(GFX_BOTTOM, NULL);

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
    C3D_CullFace(GPU_CULL_NONE); 

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    float camX = 0, camZ = 4.0f, camYaw = 0, camPitch = 0;
    HideState hideState = NOT_HIDING;

    while (aptMainLoop()) {
        hidScanInput(); irrstScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        bool needsVBOUpdate = false;

        // UI Math for Room Numbers
        int roomNumber = (camZ > -10.0f) ? 0 : (int)((abs(camZ) - 10.0f) / 10.0f) + 1;

        printf("\x1b[1;1H"); 
        printf("==============================\n");
        printf("       PLAYER STATUS          \n");
        printf("==============================\n\n");
        if (roomNumber == 0) {
            printf(" Current Room : 000 (Lobby) \n");
        } else {
            printf(" Current Room : %03d         \n", roomNumber);
        }
        printf(" Next Door    : %03d         \n\n", roomNumber + 1);
        printf(" Hiding State : %s         \n", hideState == NOT_HIDING ? "None" : (hideState == IN_CABINET ? "In Cabinet" : "Under Bed"));
        printf(" Posture      : %s         \n", isCrouching ? "Crouching" : "Standing");
        printf(" Golden Key   : %s         \n", hasKey ? "EQUIPPED" : "None    ");
        printf("\n\nControls:\n - Circle Pad: Move\n - C-Stick: Look\n - A: Interact\n - B: Toggle Crouch\n - X: Hide/Unhide");

        // 1. Key Collection (Trapped behind trolley!)
        if(!hasKey && !firstDoorUnlocked && (kDown & KEY_A) && camX < -4.5f && camZ < -8.5f && hideState == NOT_HIDING) {
            hasKey = true; 
            needsVBOUpdate = true;
        }

        // 2. Unlocking the Door
        if(hasKey && !firstDoorUnlocked && (kDown & KEY_A) && hideState == NOT_HIDING) {
            if (abs(camZ - (-10.0f)) < 1.8f && camX > -1.0f) { 
                firstDoorUnlocked = true;
                hasKey = false; 
                needsVBOUpdate = true; 
            }
        }

        // 3. Crouching Toggle with "Stand-Up Collision Prevention"
        if ((kDown & KEY_B) && hideState == NOT_HIDING) {
            if (isCrouching) {
                // Before standing up, check if your head (1.1f tall) will hit the ceiling/trolley!
                if (!checkCollision(camX, 0.0f, camZ, 1.1f)) {
                    isCrouching = false;
                }
            } else {
                isCrouching = true;
            }
        }

        // 4. Room Chunking
        int newChunk = 0;
        if (camZ < -10.0f) {
            newChunk = (int)((abs(camZ) - 10.0f) / 10.0f) + 1;
        }
        if (newChunk != currentChunk) {
            currentChunk = newChunk;
            needsVBOUpdate = true;
        }

        // 5. Proximity Auto-Doors
        int startRoom = currentChunk - 1;
        int endRoom = currentChunk + 2;
        if (startRoom < 0) startRoom = 0;
        if (endRoom > 99) endRoom = 99;

        for(int i = startRoom; i <= endRoom; i++) {
            float doorZ = -10.0f - (i * 10.0f);
            bool shouldBeOpen = (abs(camZ - doorZ) < 1.5f);
            if (i == 0 && !firstDoorUnlocked) shouldBeOpen = false; 
            
            if (doorOpen[i] != shouldBeOpen) {
                doorOpen[i] = shouldBeOpen;
                needsVBOUpdate = true;
            }
        }

        // 6. Hitboxes for Hiding (Greatly Expanded!)
        int roomIndex = (int)((abs(camZ) - 5.0f) / 10.0f);
        if (roomIndex < 0) roomIndex = 0;
        
        float baseZ = -10.0f - (roomIndex * 10.0f);
        bool nearCabinet = false;
        bool nearBed = false;

        // Expanded aura check
        if (roomSequence[roomIndex] == 0) {
            if (camX >= 0.5f && camX <= 2.5f && camZ >= baseZ - 6.5f && camZ <= baseZ - 4.5f) nearCabinet = true;
        } else {
            if (camX >= -2.5f && camX <= 0.0f && camZ >= baseZ - 9.5f && camZ <= baseZ - 5.5f) nearBed = true;
        }

        if (kDown & KEY_X) {
            if (hideState == NOT_HIDING) {
                if (nearCabinet) { 
                    hideState = IN_CABINET; 
                    camX = 1.35f; camZ = baseZ - 5.5f; 
                    camYaw = 1.57f; 
                    isCrouching = false; 
                }
                else if (nearBed) { 
                    hideState = UNDER_BED; 
                    camX = -1.2f; camZ = baseZ - 7.7f; 
                    camYaw = -1.57f; 
                    isCrouching = false; 
                }
            } else {
                hideState = NOT_HIDING; 
                camX = 0.0f; 
                camYaw = 0.0f; 
            }
        }

        if (needsVBOUpdate) {
            buildWorld(currentChunk);
            memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex));
            GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
        }

        // --- 3D HEIGHT & PHYSICS ---
        float curH = -0.9f; 
        float playerH = 1.1f; 

        if (isCrouching) {
            curH = -0.4f; 
            playerH = 0.5f; 
        }
        if (hideState == IN_CABINET) curH = -0.7f;
        else if (hideState == UNDER_BED) curH = -0.15f; 

        circlePosition cStick, cPad;
        irrstCstickRead(&cStick); hidCircleRead(&cPad);
        
        if (hideState == NOT_HIDING) {
            if (abs(cStick.dx) > 10) camYaw -= cStick.dx / 1560.0f * 0.15f;
            if (abs(cStick.dy) > 10) camPitch += cStick.dy / 1560.0f * 0.15f;
            
            if (abs(cPad.dy) > 10 || abs(cPad.dx) > 10) {
                float s = isCrouching ? 0.08f : 0.15f; 
                float sy = cPad.dy/1560.0f, sx = cPad.dx/1560.0f;
                float nextX = camX - (sinf(camYaw) * sy - cosf(camYaw) * sx) * s;
                float nextZ = camZ - (cosf(camYaw) * sy + sinf(camYaw) * sx) * s;
                
                // Passes height to calculate 3D head clearance!
                if(!checkCollision(nextX, 0.0f, camZ, playerH)) camX = nextX;
                if(!checkCollision(camX, 0.0f, nextZ, playerH)) camZ = nextZ;
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); 
        C3D_FrameDrawOn(target);

        C3D_Mtx proj, view;
        Mtx_PerspTilt(&proj, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        Mtx_Identity(&view);
        Mtx_RotateX(&view, -camPitch, true); Mtx_RotateY(&view, -camYaw, true);
        Mtx_Translate(&view, -camX, curH, -camZ, true); 
        Mtx_Multiply(&view, &proj, &view);
        
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_proj, &view);
        C3D_DrawArrays(GPU_TRIANGLES, 0, world_mesh.size());
        
        C3D_FrameEnd(0);
    }
    
    linearFree(vbo_ptr); C3D_Fini(); gfxExit();
    return 0;
}
