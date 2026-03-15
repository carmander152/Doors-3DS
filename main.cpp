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

#define MAX_VERTS 30000 

typedef struct { float pos[4]; float clr[4]; } vertex;

// --- UPGRADED BBOX: Now stores 'type' (0=Wall, 1=Cabinet, 2=Bed) ---
typedef struct { float minX, minY, minZ, maxX, maxY, maxZ; int type; } BBox;

typedef enum { NOT_HIDING, IN_CABINET, UNDER_BED } HideState;

std::vector<vertex> world_mesh;
std::vector<BBox> collisions;

// --- GAME STATE VARIABLES ---
bool hasKey = false;
bool firstDoorUnlocked = false;
int roomSequence[100]; 
int doorPositions[100]; 
bool doorOpen[100] = {false}; 
bool isCrouching = false;
bool isDead = false; 

// The Dupe Room Logic
const int DUPE_ROOM_INDEX = 3; 
int correctDupeDoorPos = 1; 
int dupeDoorNumbers[3]; 

// addBox now accepts a collision type! Defaults to 0 (Wall).
void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide, int colType = 0) {
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
    if(collide) collisions.push_back({fmin(x,x2), fmin(y,y2), fmin(z,z2), fmax(x,x2), fmax(y,y2), fmax(z,z2), colType});
}

bool checkCollision(float x, float y, float z, float h) {
    if (isDead) return true; 
    float r = 0.2f; 
    for(auto& b : collisions) {
        if(x + r > b.minX && x - r < b.maxX && z + r > b.minZ && z - r < b.maxZ) {
            if(y + h > b.minY && y < b.maxY) return true; 
        }
    }
    return false;
}

void addWallWithDoors(float z, bool leftDoor, bool leftOpen, bool centerDoor, bool centerOpen, bool rightDoor, bool rightOpen, int roomIndex) {
    addBox(-3.0f, 0, z, 0.4f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true);
    
    if(!leftDoor) addBox(-2.6f, 0, z, 1.2f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true);
    else addBox(-2.6f, 1.4f, z, 1.2f, 0.4f, -0.2f, 0.2f, 0.15f, 0.1f, false);

    addBox(-1.4f, 0, z, 0.8f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true);

    if(!centerDoor) addBox(-0.6f, 0, z, 1.2f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true);
    else addBox(-0.6f, 1.4f, z, 1.2f, 0.4f, -0.2f, 0.2f, 0.15f, 0.1f, false);

    addBox(0.6f, 0, z, 0.8f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true);

    if(!rightDoor) addBox(1.4f, 0, z, 1.2f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true);
    else addBox(1.4f, 1.4f, z, 1.2f, 0.4f, -0.2f, 0.2f, 0.15f, 0.1f, false);

    addBox(2.6f, 0, z, 0.4f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true);

    auto drawDoor = [&](float dx, bool isOpen, bool isFirst) {
        if (!isOpen) {
            addBox(dx, 0.0f, z, 1.2f, 1.4f, -0.1f, 0.15f, 0.08f, 0.05f, true); 
            addBox(dx+0.4f, 1.1f, z+0.02f, 0.4f, 0.12f, 0.02f, 0.8f, 0.7f, 0.2f, false); 
            if(isFirst && !firstDoorUnlocked) addBox(dx+0.5f, 0.7f, z+0.05f, 0.2f, 0.2f, 0.05f, 0.6f, 0.6f, 0.6f, false); 
        } else {
            addBox(dx, 0.0f, z, 0.1f, 1.4f, -1.2f, 0.3f, 0.15f, 0.08f, true); 
            addBox(dx+0.1f, 1.1f, z-0.8f, 0.02f, 0.12f, 0.4f, 0.8f, 0.7f, 0.2f, false); 
        }
    };

    if(leftDoor) drawDoor(-2.6f, leftOpen, false);
    if(centerDoor) drawDoor(-0.6f, centerOpen, (roomIndex==0));
    if(rightDoor) drawDoor(1.4f, rightOpen, false);
}

void buildWorld(int currentChunk) {
    world_mesh.clear(); 
    collisions.clear();
    
    if (currentChunk < 2) {
        addBox(-6, 0, 5, 12, 0.01f, -15, 0.22f, 0.15f, 0.1f, false); 
        addBox(-6, 1.8f, 5, 12, 0.01f, -15, 0.1f, 0.1f, 0.1f, false); 
        addBox(-6, 0, 5, 0.1f, 1.8f, -15, 0.3f, 0.3f, 0.3f, true); 
        addBox(6, 0, 5, 0.1f, 1.8f, -15, 0.3f, 0.3f, 0.3f, true);  
        
        addBox(-6.0f, 0, -10.0f, 3.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true); 
        addBox(3.0f, 0, -10.0f, 3.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true);  

        addBox(-6.0f, 0, 5.0f, 12.0f, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true); 
        addBox(-0.6f, 0, 4.9f, 1.2f, 1.5f, 0.2f, 0.4f, 0.2f, 0.1f, false); 
        addBox(-0.5f, 0, 4.8f, 1.0f, 1.4f, 0.2f, 0.5f, 0.5f, 0.5f, false); 

        addBox(-6.0f, 0.0f, -7.0f, 3.5f, 0.8f, -0.8f, 0.3f, 0.15f, 0.1f, true); 
        addBox(-3.3f, 0.0f, -7.8f, 0.8f, 0.8f, -1.0f, 0.3f, 0.15f, 0.1f, true); 

        addBox(-2.5f, 0.1f, -8.6f, 1.0f, 0.1f, -1.4f, 0.7f, 0.6f, 0.1f, false); 
        addBox(-2.4f, 0.2f, -8.7f, 0.05f, 0.4f, 0.05f, 0.8f, 0.8f, 0.8f, false); 
        addBox(-1.6f, 0.2f, -8.7f, 0.05f, 0.4f, 0.05f, 0.8f, 0.8f, 0.8f, false); 
        addBox(-2.5f, 0.6f, -8.6f, 1.0f, 1.0f, -1.4f, 0.4f, 0.2f, 0.2f, true); 

        if(!hasKey && !firstDoorUnlocked) {
            addBox(-4.8f, 0.9f, -9.9f, 0.2f, 0.2f, 0.05f, 0.3f, 0.2f, 0.1f, false); 
            addBox(-4.7f, 0.7f, -9.85f, 0.05f, 0.15f, 0.05f, 1.0f, 0.84f, 0.0f, false); 
        }
    }

    int startRoom = currentChunk - 1;
    int endRoom = currentChunk + 2;
    if (startRoom < 0) startRoom = 0;
    if (endRoom > 99) endRoom = 99;

    for(int i = startRoom; i <= endRoom; i++) {
        float z = -10 - (i * 10);
        
        if (i == DUPE_ROOM_INDEX) {
            bool leftOpen = (correctDupeDoorPos == 0 && doorOpen[i]);
            bool centerOpen = (correctDupeDoorPos == 1 && doorOpen[i]);
            bool rightOpen = (correctDupeDoorPos == 2 && doorOpen[i]);
            addWallWithDoors(z, true, leftOpen, true, centerOpen, true, rightOpen, i);
        } else {
            bool isL = (doorPositions[i] == 0);
            bool isC = (doorPositions[i] == 1);
            bool isR = (doorPositions[i] == 2);
            addWallWithDoors(z, isL, (isL && doorOpen[i]), isC, (isC && doorOpen[i]), isR, (isR && doorOpen[i]), i);
        }

        addBox(-3, 0, z, 6, 0.01f, -10, 0.2f, 0.1f, 0.05f, false); 
        addBox(-3, 1.8f, z, 6, 0.01f, -10, 0.15f, 0.15f, 0.15f, false); 
        addBox(-3, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true); 
        addBox(2.9f, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true); 

        if(roomSequence[i] == 0) {
            // CABINET Visuals
            addBox(2.2f, 0, z-6.0f, 0.1f, 1.5f, 1.0f, 0.3f, 0.18f, 0.1f, false); 
            addBox(1.6f, 1.5f, z-6.0f, 0.7f, 0.1f, 1.0f, 0.3f, 0.18f, 0.1f, false); 
            addBox(1.6f, 0, z-6.0f, 0.7f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false); 
            addBox(1.6f, 0, z-5.1f, 0.7f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false); 
            addBox(1.6f, 0, z-5.9f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false); 
            addBox(1.6f, 0, z-5.45f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false); 
            
            // --- NEW: Cabinet BBox (type 1) ---
            collisions.push_back({1.6f, 0.0f, z-6.0f, 2.3f, 1.5f, z-5.0f, 1});
        } else {
            // BED Visuals
            addBox(-2.9f, 0.4f, z-6.5f, 1.4f, 0.1f, -2.5f, 0.4f, 0.1f, 0.1f, true); 
            addBox(-2.9f, 0.0f, z-6.5f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true); 
            addBox(-1.6f, 0.0f, z-6.5f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true);
            addBox(-2.9f, 0.0f, z-8.9f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true);
            addBox(-1.6f, 0.0f, z-8.9f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true);
            addBox(-1.6f, 0.2f, z-6.5f, 0.1f, 0.2f, -2.5f, 0.4f, 0.1f, 0.1f, true);
            
            // --- NEW: Bed BBox (type 2) ---
            collisions.push_back({-2.9f, 0.0f, z-9.0f, -1.5f, 0.6f, z-6.5f, 2});
        }
    }
}

int main() {
    gfxInitDefault(); gfxSet3D(false); irrstInit(); srand(time(NULL));
    consoleInit(GFX_BOTTOM, NULL);

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    for(int i=0; i<100; i++) {
        roomSequence[i] = rand() % 2;
        doorPositions[i] = rand() % 3; 
    }
    doorPositions[0] = 1; 

    correctDupeDoorPos = rand() % 3;
    int nextRoomNumber = DUPE_ROOM_INDEX + 1; 
    dupeDoorNumbers[0] = nextRoomNumber + ((rand()%2 == 0) ? -1 : 1);
    dupeDoorNumbers[1] = nextRoomNumber + ((rand()%2 == 0) ? -2 : 2);
    dupeDoorNumbers[2] = nextRoomNumber + ((rand()%2 == 0) ? 3 : -3);
    dupeDoorNumbers[correctDupeDoorPos] = nextRoomNumber; 

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

    const char symbols[] = "@!$#&*%?";

    while (aptMainLoop()) {
        hidScanInput(); irrstScanInput();
        u32 kDown = hidKeysDown();
        
        // --- RESTART LOGIC ---
        if (kDown & KEY_START) {
            if (isDead) {
                // Wipe states and reset player
                isDead = false;
                hasKey = false;
                firstDoorUnlocked = false;
                isCrouching = false;
                hideState = NOT_HIDING;
                camX = 0.0f; camZ = 4.0f; camYaw = 0.0f; camPitch = 0.0f;
                currentChunk = 0;
                for(int i=0; i<100; i++) doorOpen[i] = false;
                
                // Re-roll the Dupe Room puzzle so it's a surprise again!
                correctDupeDoorPos = rand() % 3;
                dupeDoorNumbers[0] = nextRoomNumber + ((rand()%2 == 0) ? -1 : 1);
                dupeDoorNumbers[1] = nextRoomNumber + ((rand()%2 == 0) ? -2 : 2);
                dupeDoorNumbers[2] = nextRoomNumber + ((rand()%2 == 0) ? 3 : -3);
                dupeDoorNumbers[correctDupeDoorPos] = nextRoomNumber;

                // Reset screen back to normal colors
                C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                
                buildWorld(currentChunk);
                memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex));
                GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
                continue; // Skip the rest of the frame
            } else {
                break; // Exit the game normally
            }
        }

        bool needsVBOUpdate = false;
        int roomNumber = (camZ > -10.0f) ? 0 : (int)((abs(camZ) - 10.0f) / 10.0f) + 1;

        printf("\x1b[1;1H"); 
        printf("==============================\n");
        if (isDead) {
            printf("         YOU DIED!            \n");
            printf("==============================\n\n");
            printf(" You opened the wrong door.   \n");
            printf(" Pay attention to the numbers!\n\n\n");
            printf("    [PRESS START TO RESTART]  \n");
        } else {
            printf("       PLAYER STATUS          \n");
            printf("==============================\n\n");

            if (roomNumber == 0) {
                printf(" Current Room : 000 (Lobby) \n");
                printf(" Next Door    : 001         \n\n");
            } else {
                char g1[4], g2[4];
                for(int i=0; i<3; i++) { g1[i]=symbols[rand()%8]; g2[i]=symbols[rand()%8]; }
                g1[3]='\0'; g2[3]='\0';
                printf(" Current Room : %s         \n", g1);
                printf(" Next Door    : %s         \n\n", g2);
            }

            float dupeZ = -10.0f - (DUPE_ROOM_INDEX * 10.0f);
            bool nearLeft = (roomNumber == DUPE_ROOM_INDEX && abs(camZ - dupeZ) < 2.0f && camX < -1.4f);
            bool nearCenter = (roomNumber == DUPE_ROOM_INDEX && abs(camZ - dupeZ) < 2.0f && camX >= -1.4f && camX <= 0.6f);
            bool nearRight = (roomNumber == DUPE_ROOM_INDEX && abs(camZ - dupeZ) < 2.0f && camX > 0.6f);

            if (nearLeft) printf(" >> PLAQUE READS: %03d <<  \n\n", dupeDoorNumbers[0]);
            else if (nearCenter) printf(" >> PLAQUE READS: %03d <<  \n\n", dupeDoorNumbers[1]);
            else if (nearRight) printf(" >> PLAQUE READS: %03d <<  \n\n", dupeDoorNumbers[2]);
            else printf("                           \n\n");

            printf(" Hiding State : %s         \n", hideState == NOT_HIDING ? "None" : (hideState == IN_CABINET ? "In Cabinet" : "Under Bed"));
            printf(" Posture      : %s         \n", isCrouching ? "Crouching" : "Standing");
            printf(" Golden Key   : %s         \n", hasKey ? "EQUIPPED" : "None    ");
        }

        if (!isDead) {
            // Key Collection
            if(!hasKey && !firstDoorUnlocked && (kDown & KEY_A) && camX < -3.5f && camZ < -8.5f && hideState == NOT_HIDING) {
                hasKey = true; needsVBOUpdate = true;
            }

            // Unlocking First Door
            if(hasKey && !firstDoorUnlocked && (kDown & KEY_A) && hideState == NOT_HIDING) {
                if (abs(camZ - (-10.0f)) < 1.8f && camX > -1.0f && camX < 1.0f) { 
                    firstDoorUnlocked = true; hasKey = false; needsVBOUpdate = true; 
                }
            }

            // Dupe Room Death Trap
            if (roomNumber == DUPE_ROOM_INDEX && (kDown & KEY_A)) {
                float dupeZ = -10.0f - (DUPE_ROOM_INDEX * 10.0f);
                if (abs(camZ - dupeZ) < 1.8f) {
                    bool leftT = (camX < -1.4f);
                    bool centerT = (camX >= -1.4f && camX <= 0.6f);
                    bool rightT = (camX > 0.6f);

                    if ((leftT && correctDupeDoorPos == 0) || (centerT && correctDupeDoorPos == 1) || (rightT && correctDupeDoorPos == 2)) {
                        doorOpen[DUPE_ROOM_INDEX] = true; needsVBOUpdate = true;
                    } else if (leftT || centerT || rightT) {
                        isDead = true; 
                        C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
                        C3D_TexEnvColor(env, 0xFF0000FF); 
                    }
                }
            }

            if ((kDown & KEY_B) && hideState == NOT_HIDING) {
                if (isCrouching) {
                    if (!checkCollision(camX, 0.0f, camZ, 1.1f)) isCrouching = false;
                } else isCrouching = true;
            }

            int newChunk = 0;
            if (camZ < -10.0f) newChunk = (int)((abs(camZ) - 10.0f) / 10.0f) + 1;
            if (newChunk != currentChunk) { currentChunk = newChunk; needsVBOUpdate = true; }

            int startRoom = currentChunk - 1;
            int endRoom = currentChunk + 2;
            if (startRoom < 0) startRoom = 0;
            if (endRoom > 99) endRoom = 99;

            for(int i = startRoom; i <= endRoom; i++) {
                if (i == DUPE_ROOM_INDEX) continue; 
                
                float doorZ = -10.0f - (i * 10.0f);
                float targetX = (doorPositions[i] == 0) ? -2.0f : ((doorPositions[i] == 1) ? 0.0f : 2.0f);
                bool shouldBeOpen = (abs(camZ - doorZ) < 1.5f && abs(camX - targetX) < 1.5f);
                if (i == 0 && !firstDoorUnlocked) shouldBeOpen = false; 
                
                if (doorOpen[i] != shouldBeOpen) {
                    doorOpen[i] = shouldBeOpen; needsVBOUpdate = true;
                }
            }

            // --- NEW: COLLISION-BASED HIDING ---
            if (kDown & KEY_X) {
                if (hideState == NOT_HIDING) {
                    float reach = 0.5f; // Bumper distance to interact with furniture
                    for(auto& b : collisions) {
                        if (b.type == 1 || b.type == 2) { 
                            // Check if player's interaction bubble hits the furniture's physical box
                            if (camX + reach > b.minX && camX - reach < b.maxX && camZ + reach > b.minZ && camZ - reach < b.maxZ) {
                                if (b.type == 1) { 
                                    // Teleport perfectly into the cabinet!
                                    hideState = IN_CABINET; 
                                    camX = 1.75f; 
                                    camZ = (b.minZ + b.maxZ) / 2.0f; // Snaps to the middle of it
                                    camYaw = 1.57f; 
                                } else { 
                                    // Teleport perfectly under the bed!
                                    hideState = UNDER_BED; 
                                    camX = -2.2f; 
                                    camZ = (b.minZ + b.maxZ) / 2.0f; 
                                    camYaw = -1.57f; 
                                }
                                isCrouching = false;
                                break; 
                            }
                        }
                    }
                } else {
                    hideState = NOT_HIDING; camX = 0.0f; camYaw = 0.0f; 
                }
            }

            if (needsVBOUpdate) {
                buildWorld(currentChunk);
                memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex));
                GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
            }

            float curH = -0.9f; float playerH = 1.1f; 
            if (isCrouching) { curH = -0.4f; playerH = 0.5f; }
            if (hideState == IN_CABINET) curH = -0.7f;
            else if (hideState == UNDER_BED) curH = -0.15f; 

            circlePosition cStick, cPad;
            irrstCstickRead(&cStick); hidCircleRead(&cPad);
            
            if (hideState == NOT_HIDING) {
                if (abs(cStick.dx) > 10) camYaw -= cStick.dx / 1560.0f * 0.15f;
                if (abs(cStick.dy) > 10) camPitch += cStick.dy / 1560.0f * 0.15f;
                if (camPitch > 1.57f) camPitch = 1.57f; 
                if (camPitch < -1.57f) camPitch = -1.57f;
                
                if (abs(cPad.dy) > 10 || abs(cPad.dx) > 10) {
                    float s = isCrouching ? 0.08f : 0.15f; 
                    float sy = cPad.dy/1560.0f, sx = cPad.dx/1560.0f;
                    float nextX = camX - (sinf(camYaw) * sy - cosf(camYaw) * sx) * s;
                    float nextZ = camZ - (cosf(camYaw) * sy + sinf(camYaw) * sx) * s;
                    
                    if(!checkCollision(nextX, 0.0f, camZ, playerH)) camX = nextX;
                    if(!checkCollision(camX, 0.0f, nextZ, playerH)) camZ = nextZ;
                }
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); 
        C3D_FrameDrawOn(target);

        C3D_Mtx proj, view;
        Mtx_PerspTilt(&proj, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        Mtx_Identity(&view);
        Mtx_RotateX(&view, -camPitch, true); Mtx_RotateY(&view, -camYaw, true);
        Mtx_Translate(&view, -camX, isDead ? -0.1f : (isCrouching ? -0.4f : (hideState==NOT_HIDING ? -0.9f : (hideState==IN_CABINET?-0.7f:-0.15f))), -camZ, true); 
        Mtx_Multiply(&view, &proj, &view);
        
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_proj, &view);
        C3D_DrawArrays(GPU_TRIANGLES, 0, world_mesh.size());
        
        C3D_FrameEnd(0);
    }
    
    linearFree(vbo_ptr); C3D_Fini(); gfxExit();
    return 0;
}
