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

#define MAX_VERTS 55000 

typedef struct { float pos[4]; float clr[4]; } vertex;
typedef struct { float minX, minY, minZ, maxX, maxY, maxZ; int type; } BBox;
typedef enum { NOT_HIDING, IN_CABINET, UNDER_BED } HideState;

std::vector<vertex> world_mesh;
std::vector<BBox> collisions;

struct RoomSetup {
    int slotType[3]; 
    bool drawerOpen[3]; 
    int slotItem[3]; 
    bool isLocked;      
    float lightLevel; 

    int doorPos;     
    int pCount;      
    float pZ[10], pY[10], pW[10], pH[10], pR[10], pG[10], pB[10];
    int pSide[10];   
    
    bool isDupeRoom;
    int correctDupePos; 
    int dupeNumbers[3]; 
} rooms[100];

// --- GAME STATE VARIABLES ---
int playerHealth = 100;
int flashRedFrames = 0; 
bool hasKey = false;
bool lobbyKeyPickedUp = false;
bool doorOpen[100] = {false}; 
bool isCrouching = false;
bool isDead = false; 
HideState hideState = NOT_HIDING; 

// UI Message Variables
int messageTimer = 0;
char uiMessage[50] = "";

// --- ENTITY VARIABLES ---
bool screechActive = false;
int screechTimer = 0;
int screechCooldown = 0; 
float screechX = 0.0f;
float screechZ = 0.0f;

bool rushActive = false;
int rushState = 0; 
int rushTimer = 0;
int rushCooldown = 0; 
float rushZ = 0.0f;
float rushTargetZ = 0.0f;

// --- AUDIO SYSTEM ---
ndspWaveBuf loadWav(const char* path) {
    ndspWaveBuf waveBuf;
    memset(&waveBuf, 0, sizeof(ndspWaveBuf));

    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("Failed to load: %s\n", path);
        return waveBuf; 
    }

    fseek(file, 0, SEEK_END);
    u32 fileSize = ftell(file);
    
    fseek(file, 44, SEEK_SET); 
    u32 dataSize = fileSize - 44;

    s16* buffer = (s16*)linearAlloc(dataSize);
    if (!buffer) { fclose(file); return waveBuf; }

    fread(buffer, 1, dataSize, file);
    fclose(file);

    DSP_FlushDataCache(buffer, dataSize);

    waveBuf.data_vaddr = buffer;
    waveBuf.nsamples = dataSize / 2; 
    waveBuf.looping = false;
    waveBuf.status = NDSP_WBUF_FREE;

    return waveBuf;
}

void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide, int colType = 0, float light = 1.0f) {
    r *= light; g *= light; b *= light; 
    if (r > 1.0f) r = 1.0f; if (g > 1.0f) g = 1.0f; if (b > 1.0f) b = 1.0f;

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

void buildCabinet(float zCenter, bool isLeft, float L = 1.0f) {
    float backX = isLeft ? -2.95f : 2.85f; 
    float topX = isLeft ? -2.95f : 2.15f;  
    float frontX = isLeft ? -2.25f : 2.15f; 
    
    addBox(backX, 0, zCenter - 0.5f, 0.1f, 1.5f, 1.0f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(topX, 1.5f, zCenter - 0.5f, 0.8f, 0.1f, 1.0f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(topX, 0, zCenter - 0.5f, 0.8f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(topX, 0, zCenter + 0.4f, 0.8f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(frontX, 0, zCenter - 0.4f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(frontX, 0, zCenter + 0.05f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false, 0, L); 

    if (hideState != IN_CABINET) {
        float voidX = isLeft ? -2.85f : 2.25f;
        addBox(voidX, 0.01f, zCenter - 0.4f, 0.6f, 1.48f, 0.8f, 0.0f, 0.0f, 0.0f, false, 0, L);
    }
    
    collisions.push_back({isLeft ? -2.9f : 2.1f, 0.0f, zCenter - 0.5f, isLeft ? -2.1f : 2.9f, 1.5f, zCenter + 0.5f, 1});
}

void buildBed(float zCenter, bool isLeft, int itemType, float L = 1.0f) {
    float x = isLeft ? -2.95f : 1.55f; 
    float skirtX = isLeft ? -1.65f : 1.55f; 
    float pillowX = isLeft ? -2.65f : 2.15f; 
    
    addBox(x, 0.4f, zCenter + 1.25f, 1.4f, 0.1f, -2.5f, 0.4f, 0.1f, 0.1f, true, 0, L); 
    addBox(x, 0.0f, zCenter + 1.25f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L); 
    addBox(x + 1.3f, 0.0f, zCenter + 1.25f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L);
    addBox(x, 0.0f, zCenter - 1.15f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L);
    addBox(x + 1.3f, 0.0f, zCenter - 1.15f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L);
    addBox(skirtX, 0.2f, zCenter + 1.25f, 0.1f, 0.2f, -2.5f, 0.4f, 0.1f, 0.1f, true, 0, L); 
    addBox(pillowX, 0.5f, zCenter + 1.0f, 0.5f, 0.08f, -0.6f, 0.9f, 0.9f, 0.9f, false, 0, L); 
    
    if (itemType == 1) { 
        float itemX = isLeft ? -2.2f : 2.1f;
        addBox(itemX, 0.52f, zCenter, 0.1f, 0.05f, 0.1f, 1.0f, 0.84f, 0.0f, false, 0, L);
    }

    collisions.push_back({x, 0.0f, zCenter - 1.25f, x + 1.4f, 0.6f, zCenter + 1.25f, 2});
}

void buildDresser(float zCenter, bool isLeft, bool isOpen, int itemType, float L = 1.0f) {
    float frameX = isLeft ? -2.95f : 2.45f;
    float openOffset = isOpen ? (isLeft ? 0.35f : -0.35f) : 0.0f;
    float trayX = isLeft ? (-2.9f + openOffset) : (2.4f + openOffset); 
    float handleX = isLeft ? (-2.4f + openOffset) : (2.35f + openOffset);
    float itemX = isLeft ? (-2.6f + openOffset) : (2.5f + openOffset);

    addBox(frameX + 0.05f, 0.0f, zCenter - 0.35f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L);
    addBox(frameX + 0.05f, 0.0f, zCenter + 0.3f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L);
    addBox(frameX + 0.4f, 0.0f, zCenter - 0.35f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L);
    addBox(frameX + 0.4f, 0.0f, zCenter + 0.3f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L);

    addBox(frameX, 0.2f, zCenter - 0.4f, 0.5f, 0.3f, 0.8f, 0.3f, 0.15f, 0.1f, true, 3, L);
    addBox(trayX, 0.3f, zCenter - 0.35f, 0.5f, 0.15f, 0.7f, 0.25f, 0.12f, 0.08f, false, 0, L);
    addBox(handleX, 0.35f, zCenter - 0.1f, 0.05f, 0.05f, 0.2f, 0.8f, 0.8f, 0.8f, false, 0, L);

    if (isOpen) {
        if (itemType == 1) addBox(itemX, 0.46f, zCenter - 0.05f, 0.1f, 0.05f, 0.1f, 1.0f, 0.84f, 0.0f, false, 0, L);
        else if (itemType == 2) addBox(itemX, 0.46f, zCenter - 0.05f, 0.15f, 0.02f, 0.08f, 0.8f, 0.6f, 0.4f, false, 0, L);
    }
}

void addWallWithDoors(float z, bool leftDoor, bool leftOpen, bool centerDoor, bool centerOpen, bool rightDoor, bool rightOpen, int roomIndex, float L = 1.0f) {
    addBox(-3.0f, 0, z, 0.4f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);
    if(!leftDoor) addBox(-2.6f, 0, z, 1.2f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);
    else addBox(-2.6f, 1.4f, z, 1.2f, 0.4f, -0.2f, 0.2f, 0.15f, 0.1f, false, 0, L);
    addBox(-1.4f, 0, z, 0.8f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);
    if(!centerDoor) addBox(-0.6f, 0, z, 1.2f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);
    else addBox(-0.6f, 1.4f, z, 1.2f, 0.4f, -0.2f, 0.2f, 0.15f, 0.1f, false, 0, L);
    addBox(0.6f, 0, z, 0.8f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);
    if(!rightDoor) addBox(1.4f, 0, z, 1.2f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);
    else addBox(1.4f, 1.4f, z, 1.2f, 0.4f, -0.2f, 0.2f, 0.15f, 0.1f, false, 0, L);
    addBox(2.6f, 0, z, 0.4f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);

    auto drawDoor = [&](float dx, bool isOpen) {
        if (!isOpen) {
            addBox(dx, 0.0f, z, 1.2f, 1.4f, -0.1f, 0.15f, 0.08f, 0.05f, true, 0, L); 
            addBox(dx+0.4f, 1.1f, z+0.02f, 0.4f, 0.12f, 0.02f, 0.8f, 0.7f, 0.2f, false, 0, L); 
            if(rooms[roomIndex].isLocked) addBox(dx+0.5f, 0.7f, z+0.05f, 0.2f, 0.2f, 0.05f, 0.8f, 0.8f, 0.8f, false, 0, L); 
        } else {
            addBox(dx, 0.0f, z, 0.1f, 1.4f, -1.2f, 0.3f, 0.15f, 0.08f, true, 0, L); 
            addBox(dx+0.1f, 1.1f, z-0.8f, 0.02f, 0.12f, 0.4f, 0.8f, 0.7f, 0.2f, false, 0, L); 
        }
    };
    if(leftDoor) drawDoor(-2.6f, leftOpen);
    if(centerDoor) drawDoor(-0.6f, centerOpen);
    if(rightDoor) drawDoor(1.4f, rightOpen);
}

void buildWorld(int currentChunk, int playerCurrentRoom) {
    world_mesh.clear(); 
    collisions.clear();
    
    if (screechActive) {
        addBox(screechX - 0.2f, 0.8f, screechZ - 0.2f, 0.4f, 0.4f, 0.4f, 0.05f, 0.05f, 0.05f, false);
        addBox(screechX - 0.22f, 0.9f, screechZ - 0.22f, 0.44f, 0.05f, 0.44f, 0.9f, 0.9f, 0.9f, false);
        addBox(screechX - 0.22f, 1.05f, screechZ - 0.22f, 0.44f, 0.05f, 0.44f, 0.9f, 0.9f, 0.9f, false);
    }

    if (rushActive && rushState == 2) {
        addBox(-1.2f, 0.2f, rushZ - 0.5f, 2.4f, 2.0f, 1.0f, 0.05f, 0.05f, 0.05f, false); 
        addBox(-0.8f, 1.4f, rushZ - 0.55f, 0.4f, 0.4f, 0.1f, 0.9f, 0.9f, 0.9f, false); 
        addBox(0.4f, 1.4f, rushZ - 0.55f, 0.4f, 0.4f, 0.1f, 0.9f, 0.9f, 0.9f, false);  
        addBox(-0.6f, 0.5f, rushZ - 0.55f, 1.2f, 0.6f, 0.1f, 0.8f, 0.8f, 0.8f, false); 
    }
    
    if (currentChunk < 2) {
        addBox(-6, 0, 5.0f, 12, 0.01f, -15.0f, 0.22f, 0.15f, 0.1f, false); 
        addBox(-6, 1.8f, 5.0f, 12, 0.01f, -15.0f, 0.1f, 0.1f, 0.1f, false); 
        addBox(-6, 0, 5.0f, 0.1f, 1.8f, -15.0f, 0.3f, 0.3f, 0.3f, true); 
        addBox(6, 0, 5.0f, 0.1f, 1.8f, -15.0f, 0.3f, 0.3f, 0.3f, true);  
        addBox(-6.0f, 0, -10.0f, 3.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true); 
        addBox(3.0f, 0, -10.0f, 3.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true);  
        addBox(-6.0f, 0, 5.0f, 12.0f, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true); 

        addBox(-6.0f, 0.0f, -7.0f, 3.5f, 0.8f, -0.8f, 0.3f, 0.15f, 0.1f, true); 
        addBox(-3.3f, 0.0f, -7.8f, 0.8f, 0.8f, -1.0f, 0.3f, 0.15f, 0.1f, true); 
        addBox(-2.5f, 0.1f, -8.6f, 1.0f, 0.05f, -1.4f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.15f, -8.6f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-1.55f, 0.15f, -8.6f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.15f, -9.95f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-1.55f, 0.15f, -9.95f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.6f, -8.6f, 1.0f, 0.05f, -1.4f, 0.8f, 0.7f, 0.2f, true); 

        addBox(-4.5f, 0.0f, 4.9f, 2.0f, 1.5f, 0.1f, 0.4f, 0.4f, 0.4f, false);
        addBox(-4.4f, 0.0f, 4.85f, 0.9f, 1.4f, 0.1f, 0.6f, 0.6f, 0.6f, false);
        addBox(-3.4f, 0.0f, 4.85f, 0.9f, 1.4f, 0.1f, 0.6f, 0.6f, 0.6f, false);
        
        addBox(2.5f, 0.0f, 4.9f, 2.0f, 1.5f, 0.1f, 0.4f, 0.4f, 0.4f, false);
        addBox(2.6f, 0.0f, 4.85f, 0.9f, 1.4f, 0.1f, 0.6f, 0.6f, 0.6f, false);
        addBox(3.6f, 0.0f, 4.85f, 0.9f, 1.4f, 0.1f, 0.6f, 0.6f, 0.6f, false);

        addBox(-1.0f, 0, 4.9f, 2.0f, 1.5f, -0.4f, 0.4f, 0.2f, 0.1f, true); 
        addBox(-0.9f, 0, 4.5f, 1.8f, 1.4f, -0.2f, 0.5f, 0.5f, 0.5f, false); 

        if(!lobbyKeyPickedUp) {
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
        float L = rooms[i].lightLevel; 
        
        if (rooms[i].isDupeRoom) {
            if (playerCurrentRoom >= i) { 
                bool doorIsOpen = doorOpen[i];
                bool isL = (rooms[i].correctDupePos == 0);
                bool isC = (rooms[i].correctDupePos == 1);
                bool isR = (rooms[i].correctDupePos == 2);
                addWallWithDoors(z, isL, (isL && doorIsOpen), isC, (isC && doorIsOpen), isR, (isR && doorIsOpen), i, L);
            } else {
                bool leftOpen = (rooms[i].correctDupePos == 0 && doorOpen[i]);
                bool centerOpen = (rooms[i].correctDupePos == 1 && doorOpen[i]);
                bool rightOpen = (rooms[i].correctDupePos == 2 && doorOpen[i]);
                addWallWithDoors(z, true, leftOpen, true, centerOpen, true, rightOpen, i, L);
            }
        } else {
            bool isL = (rooms[i].doorPos == 0);
            bool isC = (rooms[i].doorPos == 1);
            bool isR = (rooms[i].doorPos == 2);
            addWallWithDoors(z, isL, (isL && doorOpen[i]), isC, (isC && doorOpen[i]), isR, (isR && doorOpen[i]), i, L);
        }

        addBox(-3, 0, z, 6, 0.01f, -10, 0.2f, 0.1f, 0.05f, false, 0, L); 
        addBox(-3, 1.8f, z, 6, 0.01f, -10, 0.15f, 0.15f, 0.15f, false, 0, L); 
        addBox(-3, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true, 0, L); 
        addBox(2.9f, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true, 0, L); 
        
        addBox(-0.4f, 1.78f, z - 5.4f, 0.8f, 0.02f, 0.8f, (L > 0.5f ? 0.9f : 0.2f), (L > 0.5f ? 0.9f : 0.2f), (L > 0.5f ? 0.8f : 0.2f), false);

        for(int s=0; s<3; s++) {
            float zCenter = z - 2.5f - (s * 2.5f); 
            int type = rooms[i].slotType[s];
            if (type == 1) buildCabinet(zCenter, true, L);
            else if (type == 2) buildCabinet(zCenter, false, L);
            else if (type == 3) buildBed(zCenter, true, rooms[i].slotItem[s], L);
            else if (type == 4) buildBed(zCenter, false, rooms[i].slotItem[s], L);
            else if (type == 5) buildDresser(zCenter, true, rooms[i].drawerOpen[s], rooms[i].slotItem[s], L);
            else if (type == 6) buildDresser(zCenter, false, rooms[i].drawerOpen[s], rooms[i].slotItem[s], L);
        }

        for(int p=0; p<rooms[i].pCount; p++) {
            float pZ = rooms[i].pZ[p]; 
            float pH = rooms[i].pH[p];
            float pW = rooms[i].pW[p];
            float pY = rooms[i].pY[p];

            if (rooms[i].pSide[p] == 0) {
                addBox(-2.95f, pY - 0.05f, z - pZ + 0.05f, 0.05f, pH + 0.1f, -pW - 0.1f, 0.1f, 0.05f, 0.02f, false, 0, L); 
                addBox(-2.94f, pY, z - pZ, 0.05f, pH, -pW, rooms[i].pR[p], rooms[i].pG[p], rooms[i].pB[p], false, 0, L); 
            } else if (rooms[i].pSide[p] == 1) {
                addBox(2.90f, pY - 0.05f, z - pZ + 0.05f, 0.05f, pH + 0.1f, -pW - 0.1f, 0.1f, 0.05f, 0.02f, false, 0, L); 
                addBox(2.89f, pY, z - pZ, 0.05f, pH, -pW, rooms[i].pR[p], rooms[i].pG[p], rooms[i].pB[p], false, 0, L); 
            }
        }
    }
}

void generateRooms() {
    for(int i=0; i<100; i++) {
        rooms[i].doorPos = rand() % 3; 
        rooms[i].isLocked = false;
        
        if (i > 0 && rand() % 100 < 8) rooms[i].lightLevel = 0.3f; 
        else rooms[i].lightLevel = 1.0f;
        
        for(int s=0; s<3; s++) {
            rooms[i].drawerOpen[s] = false;
            rooms[i].slotItem[s] = 0; 
        }

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

        bool bandaidSpawned = false;
        for(int s=0; s<3; s++) {
            int r = rand() % 100;
            if (r < 15) rooms[i].slotType[s] = 1;      
            else if (r < 30) rooms[i].slotType[s] = 2; 
            else if (r < 45) rooms[i].slotType[s] = 3; 
            else if (r < 60) rooms[i].slotType[s] = 4; 
            else if (r < 75) {
                rooms[i].slotType[s] = 5; 
                if (!bandaidSpawned && rand() % 100 < 15) { rooms[i].slotItem[s] = 2; bandaidSpawned = true; }
            }
            else if (r < 90) {
                rooms[i].slotType[s] = 6; 
                if (!bandaidSpawned && rand() % 100 < 15) { rooms[i].slotItem[s] = 2; bandaidSpawned = true; }
            }
            else rooms[i].slotType[s] = 0;             
        }

        int inDoor = rooms[i].doorPos;
        int outDoor = (i < 99) ? rooms[i+1].doorPos : 1;
        
        for(int s=0; s<3; s++) {
            if (rooms[i].slotType[s] == 3) { 
                if (s == 0 && inDoor == 0) rooms[i].slotType[s] = 5; 
                if (s == 2 && outDoor == 0) rooms[i].slotType[s] = 5; 
            }
            if (rooms[i].slotType[s] == 4) { 
                if (s == 0 && inDoor == 2) rooms[i].slotType[s] = 6; 
                if (s == 2 && outDoor == 2) rooms[i].slotType[s] = 6;
            }
        }

        rooms[i].pCount = rand() % 5 + 3; 
        for(int p=0; p<rooms[i].pCount; p++) {
            bool overlap;
            int tries = 0;
            do {
                overlap = false;
                rooms[i].pSide[p] = rand() % 2; 
                rooms[i].pZ[p] = 1.0f + (rand() % 70) / 10.0f; 
                rooms[i].pY[p] = 0.5f + (rand() % 70) / 100.0f; 
                rooms[i].pW[p] = 0.3f + (rand() % 60) / 100.0f; 
                rooms[i].pH[p] = 0.3f + (rand() % 60) / 100.0f; 
                
                for(int op=0; op<p; op++) {
                    if (rooms[i].pSide[p] == rooms[i].pSide[op]) {
                        float zDist = abs(rooms[i].pZ[p] - rooms[i].pZ[op]);
                        float yDist = abs(rooms[i].pY[p] - rooms[i].pY[op]);
                        if (zDist < (rooms[i].pW[p] + rooms[i].pW[op]) * 0.6f && 
                            yDist < (rooms[i].pH[p] + rooms[i].pH[op]) * 0.6f) {
                            overlap = true; break;
                        }
                    }
                }
                tries++;
            } while (overlap && tries < 10);
            
            rooms[i].pR[p] = 0.15f + (rand() % 35) / 100.0f; 
            rooms[i].pG[p] = 0.15f + (rand() % 35) / 100.0f; 
            rooms[i].pB[p] = 0.15f + (rand() % 35) / 100.0f; 
        }
    }
    
    rooms[0].doorPos = 1; 
    rooms[0].isDupeRoom = false; 
    rooms[0].isLocked = true; 
    
    for(int i=2; i<98; i++) {
        if (!rooms[i].isDupeRoom && !rooms[i-1].isDupeRoom && (rand() % 3 == 0)) {
            rooms[i].isLocked = true;
            int s = rand() % 3;
            if (rand() % 100 < 15) rooms[i-1].slotType[s] = (rand() % 2 == 0) ? 3 : 4; 
            else rooms[i-1].slotType[s] = (rand() % 2 == 0) ? 5 : 6; 
            rooms[i-1].slotItem[s] = 1; 
        }
    }
}

int main() {
    gfxInitDefault(); gfxSet3D(false); irrstInit(); srand(time(NULL));
    consoleInit(GFX_BOTTOM, NULL);

    romfsInit();

    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, 44100);
    ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);

    // --- NEW: Load all three Screech files ---
    ndspWaveBuf sndPsst = loadWav("romfs:/Screech_Psst.wav");
    ndspWaveBuf sndAttack = loadWav("romfs:/Screech_Attack.wav");
    ndspWaveBuf sndCaught = loadWav("romfs:/Screech_Caught.wav");

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    generateRooms(); 

    int currentChunk = 0;
    int playerCurrentRoom = -1;
    buildWorld(currentChunk, playerCurrentRoom);

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

    float camX = 0, camZ = -1.0f, camYaw = 0, camPitch = 0; 
    const char symbols[] = "@!$#&*%?";

    while (aptMainLoop()) {
        hidScanInput(); irrstScanInput();
        u32 kDown = hidKeysDown();
        
        if (kDown & KEY_START) {
            if (isDead) {
                isDead = false; hasKey = false; lobbyKeyPickedUp = false; 
                isCrouching = false; hideState = NOT_HIDING;
                playerHealth = 100; screechActive = false; flashRedFrames = 0;
                screechCooldown = 1800; rushActive = false; rushState = 0;
                rushCooldown = 0; 
                messageTimer = 0;
                camX = 0.0f; camZ = -1.0f; camYaw = 0.0f; camPitch = 0.0f;
                currentChunk = 0;
                playerCurrentRoom = -1;
                for(int i=0; i<100; i++) doorOpen[i] = false;
                
                generateRooms(); 
                C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                buildWorld(currentChunk, playerCurrentRoom);
                memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex));
                GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
                consoleClear(); 
                continue; 
            } else break; 
        }

        bool needsVBOUpdate = false;
        
        playerCurrentRoom = (camZ > -10.0f) ? -1 : (int)((-camZ - 10.0f) / 10.0f);
        if (playerCurrentRoom < -1) playerCurrentRoom = -1;
        if (playerCurrentRoom > 98) playerCurrentRoom = 98;
        
        bool isGlitching = false;
        int targetDupeRoom = -1;
        for (int k = 1; k < 100; k++) {
            if (rooms[k].isDupeRoom && playerCurrentRoom == (k - 1)) {
                isGlitching = true;
                targetDupeRoom = k;
                break;
            }
        }

        if (messageTimer > 0) messageTimer--;
        if (screechCooldown > 0) screechCooldown--;
        if (rushCooldown > 0) rushCooldown--; 

        printf("\x1b[1;1H"); 

        printf("==============================\n");
        if (isDead) {
            printf("         YOU DIED!            \n");
            printf("==============================\n\n");
            printf("                              \n");
            printf("                              \n\n\n");
            printf("    [PRESS START TO RESTART]  \n");
            for(int i=0; i<8; i++) printf("                              \n"); 
        } else {
            if (screechActive) {
                printf("  >> SCREECH ATTACK!! <<      \n");
                printf("                              \n");
                printf("     (PSST!)                  \n");
                printf("    LOOK AROUND QUICKLY!      \n");
                printf("                              \n");
                for(int i=0; i<8; i++) printf("                              \n"); 
            } else {
                printf("       PLAYER STATUS          \n");
                printf("==============================\n\n");
                
                if (playerCurrentRoom == -1) {
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
                    printf(" Current Room : %03d         \n", playerCurrentRoom + 1);
                    printf(" Next Door    : %03d         \n\n", playerCurrentRoom + 2);
                }

                int nextDoorIndex = playerCurrentRoom + 1;
                if (nextDoorIndex >= 0 && nextDoorIndex < 100 && !screechActive) {
                    float nextDoorZ = -10.0f - (nextDoorIndex * 10.0f);
                    if (abs(camZ - nextDoorZ) < 4.0f) {
                        if (isGlitching && targetDupeRoom == nextDoorIndex) {
                            if (camX < -1.4f) printf(" >> PLAQUE READS: %03d <<  \n\n", rooms[targetDupeRoom].dupeNumbers[0]);
                            else if (camX >= -1.4f && camX <= 0.6f) printf(" >> PLAQUE READS: %03d <<  \n\n", rooms[targetDupeRoom].dupeNumbers[1]);
                            else if (camX > 0.6f) printf(" >> PLAQUE READS: %03d <<  \n\n", rooms[targetDupeRoom].dupeNumbers[2]);
                            else printf("                           \n\n");
                        } else {
                            printf(" >> PLAQUE READS: %03d <<  \n\n", nextDoorIndex + 1);
                        }
                    } else {
                        printf("                           \n\n");
                    }
                } else if (!screechActive) {
                    printf("                           \n\n");
                }

                printf(" Health       : %d / 100   \n", playerHealth);
                printf(" Golden Key   : %s         \n", hasKey ? "EQUIPPED" : "None    ");
                
                if (messageTimer > 0) printf("\n ** %s ** \n", uiMessage);
                else if (rushActive && rushState == 1) printf("\n ** The lights are flickering... ** \n");
                else printf("\n                                    \n");

                for(int i=0; i<4; i++) printf("                                    \n");
            }
        }

        if (!isDead) {
            
            int screechChance = (playerCurrentRoom > 0 && rooms[playerCurrentRoom].lightLevel < 0.5f) ? 400 : 2000;
            if (!screechActive && screechCooldown <= 0 && hideState == NOT_HIDING && playerCurrentRoom > 0 && (rand() % screechChance == 0)) {
                screechActive = true;
                screechTimer = 180; 
                screechX = camX + sinf(camYaw) * 2.0f; 
                screechZ = camZ + cosf(camYaw) * 2.0f;
                needsVBOUpdate = true;
                
                // --- PLAY PSST SOUND ---
                ndspChnWaveBufClear(0); // Instantly stop anything else on this channel
                if (sndPsst.data_vaddr) {
                    DSP_FlushDataCache(sndPsst.data_vaddr, sndPsst.nsamples * 2);
                    ndspChnWaveBufAdd(0, &sndPsst); 
                }
            }

            if (screechActive) {
                screechTimer--;
                float fx = -sinf(camYaw); float fz = -cosf(camYaw);
                float vx = screechX - camX; float vz = screechZ - camZ;
                float dist = sqrt(vx*vx + vz*vz);
                if (dist > 0.0f) { vx /= dist; vz /= dist; }
                
                float dotProduct = (fx * vx) + (fz * vz);
                if (dotProduct > 0.85f) { 
                    screechActive = false; 
                    screechCooldown = 1800; 
                    needsVBOUpdate = true;
                    sprintf(uiMessage, "Dodged Screech!"); messageTimer = 90;
                    
                    // --- PLAY CAUGHT SOUND ---
                    ndspChnWaveBufClear(0);
                    if (sndCaught.data_vaddr) {
                        DSP_FlushDataCache(sndCaught.data_vaddr, sndCaught.nsamples * 2);
                        ndspChnWaveBufAdd(0, &sndCaught); 
                    }

                } else if (screechTimer <= 0) {
                    screechActive = false; 
                    screechCooldown = 1800; 
                    needsVBOUpdate = true;
                    playerHealth -= 20; flashRedFrames = 25; 
                    sprintf(uiMessage, "Screech bit you! (-20 HP)"); messageTimer = 90; 
                    if (playerHealth <= 0) isDead = true;

                    // --- PLAY ATTACK SOUND ---
                    ndspChnWaveBufClear(0);
                    if (sndAttack.data_vaddr) {
                        DSP_FlushDataCache(sndAttack.data_vaddr, sndAttack.nsamples * 2);
                        ndspChnWaveBufAdd(0, &sndAttack); 
                    }
                }
            }

            if (rushActive) {
                if (rushState == 1) { 
                    rushTimer--;
                    if (rushTimer % 10 == 0) { 
                        float flicker = (rand() % 2 == 0) ? 0.3f : 1.0f;
                        if (playerCurrentRoom >= 0 && playerCurrentRoom < 100) rooms[playerCurrentRoom].lightLevel = flicker;
                        needsVBOUpdate = true;
                    }

                    if (rushTimer <= 0) {
                        rushState = 2; 
                        rushZ = camZ + 30.0f; 
                        rushTargetZ = camZ - 20.0f; 
                    }
                } else if (rushState == 2) { 
                    rushZ -= 0.6f; 
                    needsVBOUpdate = true; 

                    int rushRoomIndex = (int)((-rushZ - 10.0f) / 10.0f);
                    if (rushRoomIndex >= 0 && rushRoomIndex < 100) {
                        rooms[rushRoomIndex].lightLevel = 0.3f;
                    }

                    if (abs(rushZ - camZ) < 3.0f && hideState == NOT_HIDING) {
                        playerHealth = 0; isDead = true; 
                        flashRedFrames = 50;
                    }

                    if (rushZ < rushTargetZ) { 
                        rushActive = false; rushState = 0; needsVBOUpdate = true;
                        rushCooldown = 1800; 
                    }
                }
            }

            if (kDown & KEY_X) {
                if (hideState == NOT_HIDING) {
                    bool interactedWithDrawer = false;
                    
                    if (playerCurrentRoom >= 0 && playerCurrentRoom < 100) {
                        for(int s=0; s<3; s++) {
                            float zCenter = (-10.0f - (playerCurrentRoom * 10.0f)) - 2.5f - (s * 2.5f);
                            int type = rooms[playerCurrentRoom].slotType[s];
                            
                            if (type == 5 || type == 6) {
                                float slotX = (type == 5) ? -2.4f : 2.4f; 
                                if (abs(camZ - zCenter) < 1.5f && abs(camX - slotX) < 1.8f) {
                                    rooms[playerCurrentRoom].drawerOpen[s] = !rooms[playerCurrentRoom].drawerOpen[s];
                                    needsVBOUpdate = true;
                                    interactedWithDrawer = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (!interactedWithDrawer) {
                        float reach = 0.5f; 
                        for(auto& b : collisions) {
                            if (b.type == 1 || b.type == 2) { 
                                if (camX + reach > b.minX && camX - reach < b.maxX && camZ + reach > b.minZ && camZ - reach < b.maxZ) {
                                    if (b.type == 1) { 
                                        hideState = IN_CABINET; camZ = (b.minZ + b.maxZ) / 2.0f;
                                        if (b.minX < 0) { camX = -2.5f; camYaw = -1.57f; } else { camX = 2.5f; camYaw = 1.57f; } 
                                    } else { 
                                        hideState = UNDER_BED; camZ = (b.minZ + b.maxZ) / 2.0f; 
                                        if (b.minX < 0) { camX = -2.2f; camYaw = -1.57f; } else { camX = 2.2f; camYaw = 1.57f; } 
                                    }
                                    isCrouching = false; needsVBOUpdate = true; break; 
                                }
                            }
                        }
                    }
                } else {
                    hideState = NOT_HIDING; camX = 0.0f; camYaw = 0.0f; needsVBOUpdate = true; 
                }
            }

            if ((kDown & KEY_A) && hideState == NOT_HIDING) {
                
                if (playerCurrentRoom >= 0 && playerCurrentRoom < 100) {
                    for(int s=0; s<3; s++) {
                        float zCenter = (-10.0f - (playerCurrentRoom * 10.0f)) - 2.5f - (s * 2.5f);
                        int type = rooms[playerCurrentRoom].slotType[s];
                        if (type == 0) continue;
                        
                        float slotX = (type % 2 != 0) ? -2.4f : 2.4f; 
                        
                        if (abs(camZ - zCenter) < 1.5f && abs(camX - slotX) < 1.8f) {
                            if (type == 5 || type == 6) { 
                                if (rooms[playerCurrentRoom].drawerOpen[s]) {
                                    if (rooms[playerCurrentRoom].slotItem[s] == 1) { 
                                        hasKey = true;
                                        rooms[playerCurrentRoom].slotItem[s] = 0;
                                        needsVBOUpdate = true;
                                        sprintf(uiMessage, "Grabbed the Golden Key!"); messageTimer = 90;
                                    } else if (rooms[playerCurrentRoom].slotItem[s] == 2) { 
                                        playerHealth += 10;
                                        if (playerHealth > 100) playerHealth = 100;
                                        rooms[playerCurrentRoom].slotItem[s] = 0;
                                        needsVBOUpdate = true;
                                        sprintf(uiMessage, "Used a Bandaid! (+10 HP)"); messageTimer = 90;
                                    } else {
                                        sprintf(uiMessage, "Drawer is empty..."); messageTimer = 60;
                                    }
                                }
                            } else if (type == 3 || type == 4) { 
                                if (rooms[playerCurrentRoom].slotItem[s] == 1) {
                                    hasKey = true;
                                    rooms[playerCurrentRoom].slotItem[s] = 0;
                                    needsVBOUpdate = true;
                                    sprintf(uiMessage, "Grabbed Key off the bed!"); messageTimer = 90;
                                }
                            }
                        }
                    }
                }

                if(!lobbyKeyPickedUp && rooms[0].isLocked && camX < -3.5f && camZ < -8.5f) {
                    lobbyKeyPickedUp = true; hasKey = true; needsVBOUpdate = true;
                    sprintf(uiMessage, "Found the Lobby Key!"); messageTimer = 90;
                }

                if (hasKey) {
                    for(int i=0; i<100; i++) {
                        if (rooms[i].isLocked) {
                            float doorZ = -10.0f - (i * 10.0f);
                            float doorX = (rooms[i].doorPos == 0) ? -2.0f : ((rooms[i].doorPos == 1) ? 0.0f : 2.0f);
                            
                            if (abs(camZ - doorZ) < 2.5f && abs(camX - doorX) < 2.0f) {
                                rooms[i].isLocked = false;
                                hasKey = false;
                                needsVBOUpdate = true;
                                sprintf(uiMessage, "Door Unlocked!"); messageTimer = 60;
                                break; 
                            }
                        }
                    }
                }

                int nextRoom = playerCurrentRoom + 1;
                if (nextRoom >= 0 && nextRoom < 100) {
                    int interactRoom = rooms[nextRoom].isDupeRoom ? nextRoom : -1;
                    if (interactRoom != -1 && !doorOpen[interactRoom]) {
                        float lockDoorZ = -10.0f - (nextRoom * 10.0f);
                        if (abs(camZ - lockDoorZ) < 1.8f) {
                            bool leftT = (camX < -1.4f);
                            bool centerT = (camX >= -1.4f && camX <= 0.6f);
                            bool rightT = (camX > 0.6f);
                            int correctPos = rooms[interactRoom].correctDupePos;

                            if ((leftT && correctPos == 0) || (centerT && correctPos == 1) || (rightT && correctPos == 2)) {
                                doorOpen[interactRoom] = true; needsVBOUpdate = true;
                            } else if (leftT || centerT || rightT) {
                                playerHealth -= 34; flashRedFrames = 25; camZ += 2.0f; 
                                if (playerHealth <= 0) isDead = true; 
                            }
                        }
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
            
            if (newChunk != currentChunk || needsVBOUpdate) { 
                if (newChunk != currentChunk && playerCurrentRoom > 1 && !rushActive && rushCooldown <= 0 && rand() % 100 < 12) {
                    rushActive = true;
                    rushState = 1; 
                    rushTimer = 300 + (rand() % 120); 
                }
                currentChunk = newChunk; 
                needsVBOUpdate = true; 
            }

            int startRoom = currentChunk - 1;
            int endRoom = currentChunk + 2;
            if (startRoom < 0) startRoom = 0;
            if (endRoom > 99) endRoom = 99;

            for(int i = startRoom; i <= endRoom; i++) {
                if (rooms[i].isDupeRoom) continue; 
                float wallZ = -10.0f - (i * 10.0f);
                float targetX = (rooms[i].doorPos == 0) ? -2.0f : ((rooms[i].doorPos == 1) ? 0.0f : 2.0f);
                
                bool shouldBeOpen = (abs(camZ - wallZ) < 1.5f && abs(camX - targetX) < 1.5f);
                if (rooms[i].isLocked) shouldBeOpen = false; 
                
                if (doorOpen[i] != shouldBeOpen) {
                    doorOpen[i] = shouldBeOpen; needsVBOUpdate = true;
                }
            }

            if (needsVBOUpdate) {
                buildWorld(currentChunk, playerCurrentRoom);
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
                if (abs(cStick.dx) > 10) camYaw -= cStick.dx / 1560.0f * 0.8f;
                if (abs(cStick.dy) > 10) camPitch += cStick.dy / 1560.0f * 0.8f;
                if (camPitch > 1.57f) camPitch = 1.57f; 
                if (camPitch < -1.57f) camPitch = -1.57f;
                
                if (abs(cPad.dy) > 10 || abs(cPad.dx) > 10) {
                    float s = isCrouching ? 0.16f : 0.28f; 
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

        if (flashRedFrames > 0 && !isDead) {
            C3D_TexEnvColor(env, 0xFF0000FF); 
            C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
            flashRedFrames--;
        } else if (!isDead) {
            C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        }

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
    
    // Clean up audio memory!
    if (sndPsst.data_vaddr) linearFree((void*)sndPsst.data_vaddr);
    if (sndAttack.data_vaddr) linearFree((void*)sndAttack.data_vaddr);
    if (sndCaught.data_vaddr) linearFree((void*)sndCaught.data_vaddr);
    
    romfsExit();
    ndspExit();
    C3D_Fini(); gfxExit();
    return 0;
}
