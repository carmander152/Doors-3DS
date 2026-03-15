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

#define MAX_VERTS 45000 

typedef struct { float pos[4]; float clr[4]; } vertex;
typedef struct { float minX, minY, minZ, maxX, maxY, maxZ; int type; } BBox;
typedef enum { NOT_HIDING, IN_CABINET, UNDER_BED } HideState;

std::vector<vertex> world_mesh;
std::vector<BBox> collisions;

struct RoomSetup {
    int slotType[3]; 
    int doorPos;     
    int pCount;      
    float pZ[4], pY[4], pW[4], pH[4], pR[4], pG[4], pB[4];
    int pSide[4];    
    
    bool isDupeRoom;
    int correctDupePos; 
    int dupeNumbers[3]; 
} rooms[100];

bool hasKey = false;
bool firstDoorUnlocked = false;
bool doorOpen[100] = {false}; 
bool isCrouching = false;
bool isDead = false; 

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

// Reverted Cabinets (Flush to wall, facing outward)
void buildCabinet(float zCenter, bool isLeft) {
    float backX = isLeft ? -2.3f : 2.2f;
    float frontX = isLeft ? -1.7f : 1.6f;
    float topX = isLeft ? -2.3f : 1.6f; 
    
    addBox(backX, 0, zCenter - 0.5f, 0.1f, 1.5f, 1.0f, 0.3f, 0.18f, 0.1f, false); // Back
    addBox(topX, 1.5f, zCenter - 0.5f, 0.7f, 0.1f, 1.0f, 0.3f, 0.18f, 0.1f, false); // Top
    addBox(topX, 0, zCenter - 0.5f, 0.7f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false); // Side 1
    addBox(topX, 0, zCenter + 0.4f, 0.7f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false); // Side 2
    addBox(frontX, 0, zCenter - 0.4f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false); // Door 1
    addBox(frontX, 0, zCenter + 0.05f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false); // Door 2
    
    collisions.push_back({isLeft ? -2.3f : 1.6f, 0.0f, zCenter - 0.5f, isLeft ? -1.6f : 2.3f, 1.5f, zCenter + 0.5f, 1});
}

void buildBed(float zCenter, bool isLeft) {
    float x = isLeft ? -2.9f : 1.5f;
    float skirtX = isLeft ? -1.6f : 1.5f;
    float pillowX = isLeft ? -2.6f : 2.0f;
    addBox(x, 0.4f, zCenter + 1.25f, 1.4f, 0.1f, -2.5f, 0.4f, 0.1f, 0.1f, true); 
    addBox(x, 0.0f, zCenter + 1.25f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true); 
    addBox(x + 1.3f, 0.0f, zCenter + 1.25f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true);
    addBox(x, 0.0f, zCenter - 1.15f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true);
    addBox(x + 1.3f, 0.0f, zCenter - 1.15f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true);
    addBox(skirtX, 0.2f, zCenter + 1.25f, 0.1f, 0.2f, -2.5f, 0.4f, 0.1f, 0.1f, true); 
    addBox(pillowX, 0.5f, zCenter + 1.0f, 0.5f, 0.08f, -0.6f, 0.9f, 0.9f, 0.9f, false); 
    collisions.push_back({x, 0.0f, zCenter - 1.25f, x + 1.4f, 0.6f, zCenter + 1.25f, 2});
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

        // TROLLEY UPDATE: Luggage removed, Roof lowered to 0.6 to force crouching!
        addBox(-2.5f, 0.1f, -8.6f, 1.0f, 0.05f, -1.4f, 0.8f, 0.7f, 0.2f, false); // Base
        addBox(-2.5f, 0.15f, -8.6f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); // Poles
        addBox(-1.55f, 0.15f, -8.6f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.15f, -9.95f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-1.55f, 0.15f, -9.95f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.6f, -8.6f, 1.0f, 0.05f, -1.4f, 0.8f, 0.7f, 0.2f, true); // Lowered Roof (Crouch Block)

        // Luggage placed cleanly on the floor NEXT to the trolley
        addBox(-1.4f, 0.0f, -9.0f, 0.8f, 0.4f, -0.6f, 0.4f, 0.2f, 0.1f, true); 
        addBox(-1.4f, 0.4f, -9.0f, 0.8f, 0.4f, -1.0f, 0.3f, 0.3f, 0.3f, true); 

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
        
        if (rooms[i].isDupeRoom) {
            bool leftOpen = (rooms[i].correctDupePos == 0 && doorOpen[i]);
            bool centerOpen = (rooms[i].correctDupePos == 1 && doorOpen[i]);
            bool rightOpen = (rooms[i].correctDupePos == 2 && doorOpen[i]);
            addWallWithDoors(z, true, leftOpen, true, centerOpen, true, rightOpen, i);
        } else {
            bool isL = (rooms[i].doorPos == 0);
            bool isC = (rooms[i].doorPos == 1);
            bool isR = (rooms[i].doorPos == 2);
            addWallWithDoors(z, isL, (isL && doorOpen[i]), isC, (isC && doorOpen[i]), isR, (isR && doorOpen[i]), i);
        }

        addBox(-3, 0, z, 6, 0.01f, -10, 0.2f, 0.1f, 0.05f, false); 
        addBox(-3, 1.8f, z, 6, 0.01f, -10, 0.15f, 0.15f, 0.15f, false); 
        addBox(-3, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true); 
        addBox(2.9f, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true); 

        for(int s=0; s<3; s++) {
            float zCenter = z - 2.5f - (s * 2.5f); 
            int type = rooms[i].slotType[s];
            if (type == 1) buildCabinet(zCenter, true);
            else if (type == 2) buildCabinet(zCenter, false);
            else if (type == 3) buildBed(zCenter, true);
            else if (type == 4) buildBed(zCenter, false);
        }

        // PAINTINGS WITH FRAMES & PADDING
        for(int p=0; p<rooms[i].pCount; p++) {
            float pZ = z - rooms[i].pZ[p]; 
            float pH = rooms[i].pH[p];
            float pW = rooms[i].pW[p];
            float pY = rooms[i].pY[p];

            if (rooms[i].pSide[p] == 0) { // Left Wall
                addBox(-2.99f, pY - 0.05f, pZ + 0.05f, 0.04f, pH + 0.1f, -(pW + 0.1f), 0.1f, 0.05f, 0.02f, false); // Frame
                addBox(-2.95f, pY, pZ, 0.02f, pH, -pW, rooms[i].pR[p], rooms[i].pG[p], rooms[i].pB[p], false); // Canvas
            } else { // Right Wall
                addBox(2.95f, pY - 0.05f, pZ + 0.05f, 0.04f, pH + 0.1f, -(pW + 0.1f), 0.1f, 0.05f, 0.02f, false); // Frame
                addBox(2.93f, pY, pZ, 0.02f, pH, -pW, rooms[i].pR[p], rooms[i].pG[p], rooms[i].pB[p], false); // Canvas
            }
        }
    }
}

void generateRooms() {
    for(int i=0; i<100; i++) {
        rooms[i].doorPos = rand() % 3; 
        rooms[i].isDupeRoom = (i > 1 && (rand() % 100 < 15));
        if (rooms[i].isDupeRoom) {
            rooms[i].correctDupePos = rand() % 3;
            int nextRoomNumber = i + 1; 
            int fake1 = nextRoomNumber + (rand() % 3 + 1);
            int fake2 = nextRoomNumber - (rand() % 3 + 1);
            if (fake2 <= 0) fake2 = nextRoomNumber + 4; 
            rooms[i].dupeNumbers[0] = fake1;
            rooms[i].dupeNumbers[1] = fake2;
            rooms[i].dupeNumbers[2] = nextRoomNumber + 5;
            rooms[i].dupeNumbers[rooms[i].correctDupePos] = nextRoomNumber;
        }
        for(int s=0; s<3; s++) {
            int r = rand() % 100;
            if (r < 15) rooms[i].slotType[s] = 1; 
            else if (r < 30) rooms[i].slotType[s] = 2; 
            else if (r < 45) rooms[i].slotType[s] = 3; 
            else if (r < 60) rooms[i].slotType[s] = 4; 
            else rooms[i].slotType[s] = 0; 
        }
        rooms[i].pCount = rand() % 4; 
        for(int p=0; p<rooms[i].pCount; p++) {
            rooms[i].pZ[p] = 1.0f + (rand() % 80) / 10.0f; 
            rooms[i].pY[p] = 0.6f + (rand() % 60) / 100.0f; 
            rooms[i].pW[p] = 0.4f + (rand() % 80) / 100.0f; 
            rooms[i].pH[p] = 0.4f + (rand() % 60) / 100.0f; 
            rooms[i].pR[p] = (rand() % 100) / 100.0f; 
            rooms[i].pG[p] = (rand() % 100) / 100.0f; 
            rooms[i].pB[p] = (rand() % 100) / 100.0f; 
            rooms[i].pSide[p] = rand() % 2; 
        }
    }
    rooms[0].doorPos = 1; 
    rooms[0].isDupeRoom = false; 
}

int main() {
    gfxInitDefault(); gfxSet3D(false); irrstInit(); srand(time(NULL));
    consoleInit(GFX_BOTTOM, NULL);

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    generateRooms(); 

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
        
        if (kDown & KEY_START) {
            if (isDead) {
                isDead = false; hasKey = false; firstDoorUnlocked = false;
                isCrouching = false; hideState = NOT_HIDING;
                camX = 0.0f; camZ = 4.0f; camYaw = 0.0f; camPitch = 0.0f;
                currentChunk = 0;
                for(int i=0; i<100; i++) doorOpen[i] = false;
                
                generateRooms(); 
                C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                buildWorld(currentChunk);
                memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex));
                GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
                consoleClear(); // FIX FOR STICKY RESTART TEXT
                continue; 
            } else break; 
        }

        bool needsVBOUpdate = false;
        int roomNumber = (camZ > -10.0f) ? 0 : (int)((abs(camZ) - 10.0f) / 10.0f) + 1;
        
        // --- UPGRADED GLITCH LOGIC ---
        // Glitches if you are currently IN a dupe room, OR if you just opened the door LEADING to a dupe room!
        bool isGlitching = (rooms[roomNumber].isDupeRoom && !doorOpen[roomNumber]) || 
                           (rooms[roomNumber + 1].isDupeRoom && doorOpen[roomNumber]);

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
                if (isGlitching) {
                    char g2[4]; for(int i=0; i<3; i++) g2[i]=symbols[rand()%8]; g2[3]='\0';
                    printf(" Next Door    : %s         \n\n", g2);
                } else {
                    printf(" Next Door    : 001         \n\n");
                }
            } else if (isGlitching) {
                char g1[4], g2[4];
                for(int i=0; i<3; i++) { g1[i]=symbols[rand()%8]; g2[i]=symbols[rand()%8]; }
                g1[3]='\0'; g2[3]='\0';
                printf(" Current Room : %s         \n", g1);
                printf(" Next Door    : %s         \n\n", g2);
            } else {
                printf(" Current Room : %03d         \n", roomNumber);
                printf(" Next Door    : %03d         \n\n", roomNumber + 1);
            }

            int plaqueTargetRoom = rooms[roomNumber].isDupeRoom ? roomNumber : (rooms[roomNumber + 1].isDupeRoom ? roomNumber + 1 : -1);
            float wallZ = -10.0f - (plaqueTargetRoom * 10.0f);
            
            if (isGlitching && plaqueTargetRoom != -1 && abs(camZ - wallZ) < 2.0f) {
                if (camX < -1.4f) printf(" >> PLAQUE READS: %03d <<  \n\n", rooms[plaqueTargetRoom].dupeNumbers[0]);
                else if (camX >= -1.4f && camX <= 0.6f) printf(" >> PLAQUE READS: %03d <<  \n\n", rooms[plaqueTargetRoom].dupeNumbers[1]);
                else if (camX > 0.6f) printf(" >> PLAQUE READS: %03d <<  \n\n", rooms[plaqueTargetRoom].dupeNumbers[2]);
                else printf("                           \n\n");
            } else {
                printf("                           \n\n");
            }

            printf(" Hiding State : %s         \n", hideState == NOT_HIDING ? "None" : (hideState == IN_CABINET ? "In Cabinet" : "Under Bed"));
            printf(" Posture      : %s         \n", isCrouching ? "Crouching" : "Standing");
            printf(" Golden Key   : %s         \n", hasKey ? "EQUIPPED" : "None    ");
        }

        if (!isDead) {
            if(!hasKey && !firstDoorUnlocked && (kDown & KEY_A) && camX < -3.5f && camZ < -8.5f && hideState == NOT_HIDING) {
                hasKey = true; needsVBOUpdate = true;
            }

            if(hasKey && !firstDoorUnlocked && (kDown & KEY_A) && hideState == NOT_HIDING) {
                if (abs(camZ - (-10.0f)) < 1.8f && camX > -1.0f && camX < 1.0f) { 
                    firstDoorUnlocked = true; hasKey = false; needsVBOUpdate = true; 
                }
            }

            int interactRoom = rooms[roomNumber].isDupeRoom ? roomNumber : (rooms[roomNumber + 1].isDupeRoom ? roomNumber + 1 : -1);
            if (interactRoom != -1 && !doorOpen[interactRoom] && (kDown & KEY_A)) {
                float wallZ = -10.0f - (interactRoom * 10.0f);
                if (abs(camZ - wallZ) < 1.8f) {
                    bool leftT = (camX < -1.4f);
                    bool centerT = (camX >= -1.4f && camX <= 0.6f);
                    bool rightT = (camX > 0.6f);
                    int correctPos = rooms[interactRoom].correctDupePos;

                    if ((leftT && correctPos == 0) || (centerT && correctPos == 1) || (rightT && correctPos == 2)) {
                        doorOpen[interactRoom] = true; needsVBOUpdate = true;
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
                if (rooms[i].isDupeRoom) continue; 
                float doorZ = -10.0f - (i * 10.0f);
                float targetX = (rooms[i].doorPos == 0) ? -2.0f : ((rooms[i].doorPos == 1) ? 0.0f : 2.0f);
                bool shouldBeOpen = (abs(camZ - doorZ) < 1.5f && abs(camX - targetX) < 1.5f);
                if (i == 0 && !firstDoorUnlocked) shouldBeOpen = false; 
                if (doorOpen[i] != shouldBeOpen) {
                    doorOpen[i] = shouldBeOpen; needsVBOUpdate = true;
                }
            }

            if (kDown & KEY_X) {
                if (hideState == NOT_HIDING) {
                    float reach = 0.5f; 
                    for(auto& b : collisions) {
                        if (b.type == 1 || b.type == 2) { 
                            if (camX + reach > b.minX && camX - reach < b.maxX && camZ + reach > b.minZ && camZ - reach < b.maxZ) {
                                if (b.type == 1) { 
                                    hideState = IN_CABINET; 
                                    camZ = (b.minZ + b.maxZ) / 2.0f;
                                    if (b.minX < 0) { camX = -1.65f; camYaw = -1.57f; } // Left cabinet, face right
                                    else { camX = 1.65f; camYaw = 1.57f; } // Right cabinet, face left
                                } else { 
                                    hideState = UNDER_BED; 
                                    camZ = (b.minZ + b.maxZ) / 2.0f; 
                                    if (b.minX < 0) { camX = -2.2f; camYaw = -1.57f; } 
                                    else { camX = 2.2f; camYaw = 1.57f; } 
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
