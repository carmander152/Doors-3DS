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

#define MAX_VERTS 120000 
#define TOTAL_ROOMS 102 

typedef struct { float pos[4]; float clr[4]; } vertex;
typedef struct { float minX, minY, minZ, maxX, maxY, maxZ; int type; } BBox;
typedef enum { NOT_HIDING, IN_CABINET, UNDER_BED, BEHIND_DOOR } HideState;

std::vector<vertex> world_mesh;
std::vector<BBox> collisions;

float globalTintR = 1.0f, globalTintG = 1.0f, globalTintB = 1.0f;

struct RoomSetup {
    int slotType[3]; bool drawerOpen[3]; int slotItem[3]; 
    bool isLocked; bool isJammed; float lightLevel; 
    int doorPos; int pCount;      
    float pZ[10], pY[10], pW[10], pH[10], pR[10], pG[10], pB[10];
    int pSide[10];   

    bool hasLeftRoom; bool leftDoorOpen; float leftDoorOffset;
    int leftRoomSlotTypeL[3]; int leftRoomSlotItemL[3]; bool leftRoomDrawerOpenL[3];
    int leftRoomSlotTypeR[3]; int leftRoomSlotItemR[3]; bool leftRoomDrawerOpenR[3];

    bool hasRightRoom; bool rightDoorOpen; float rightDoorOffset;
    int rightRoomSlotTypeL[3]; int rightRoomSlotItemL[3]; bool rightRoomDrawerOpenL[3];
    int rightRoomSlotTypeR[3]; int rightRoomSlotItemR[3]; bool rightRoomDrawerOpenR[3];
    
    bool isDupeRoom; int correctDupePos; int dupeNumbers[3]; 
    bool hasEyes; float eyesX, eyesY, eyesZ; 
    
    bool hasSeekEyes; int seekEyeCount;
    bool isSeekHallway; bool isSeekChase; bool isSeekFinale;  
} rooms[TOTAL_ROOMS];

int playerHealth = 100, flashRedFrames = 0, seekStartRoom = 0, playerCoins = 0;
bool hasKey = false, lobbyKeyPickedUp = false, isCrouching = false, isDead = false;
bool doorOpen[TOTAL_ROOMS] = {false}; 
HideState hideState = NOT_HIDING; 

bool inElevator = true, elevatorDoorsOpen = false, elevatorClosing = false, elevatorJamFinished = false;
int elevatorTimer = 1593, messageTimer = 0;
float elevatorDoorOffset = 0.0f; 
char uiMessage[50] = "";

bool screechActive = false, rushActive = false, seekActive = false, inEyesRoom = false, isLookingAtEyes = false;
int screechTimer = 0, screechCooldown = 0, rushState = 0, rushTimer = 0, rushCooldown = 0;
int seekState = 0, seekTimer = 0, eyesDamageTimer = 0, eyesDamageAccumulator = 0, eyesGraceTimer = 0, eyesSoundCooldown = 0;
float screechX = 0.0f, screechY = 0.0f, screechZ = 0.0f, screechOffsetX = 0.0f, screechOffsetY = 0.0f, screechOffsetZ = 0.0f;
float rushStartTimer = 1.0f, rushZ = 0.0f, rushTargetZ = 0.0f;
float seekZ = 0.0f, seekSpeed = 0.0f, seekMaxSpeed = 0.038f; 

int getDisplayRoom(int idx) { return (idx < 0) ? 0 : idx + 1; }
int getNextDoorIndex(int currentIdx) { return (currentIdx >= seekStartRoom && currentIdx <= seekStartRoom + 2) ? seekStartRoom + 3 : currentIdx + 1; }

ndspWaveBuf loadWav(const char* path) {
    ndspWaveBuf waveBuf; memset(&waveBuf, 0, sizeof(ndspWaveBuf));
    FILE* file = fopen(path, "rb");
    if (!file) return waveBuf; 
    fseek(file, 12, SEEK_SET);
    char chunkId[4]; u32 chunkSize; bool foundData = false;
    while (fread(chunkId, 1, 4, file) == 4) {
        fread(&chunkSize, 4, 1, file);
        if (strncmp(chunkId, "data", 4) == 0) { foundData = true; break; }
        fseek(file, chunkSize, SEEK_CUR);
    }
    if (!foundData) { fclose(file); return waveBuf; }
    s16* buffer = (s16*)linearAlloc(chunkSize);
    if (!buffer) { fclose(file); return waveBuf; }
    fread(buffer, 1, chunkSize, file); fclose(file);
    DSP_FlushDataCache(buffer, chunkSize);
    waveBuf.data_vaddr = buffer; waveBuf.nsamples = chunkSize / 2; waveBuf.looping = false; waveBuf.status = NDSP_WBUF_FREE;
    return waveBuf;
}

void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide, int colType = 0, float light = 1.0f) {
    r *= light * globalTintR; g *= light * globalTintG; b *= light * globalTintB; 
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
        if (b.type == 4) continue; 
        if(x + r > b.minX && x - r < b.maxX && z + r > b.minZ && z - r < b.maxZ) {
            if(y + h > b.minY && y < b.maxY) return true; 
        }
    }
    return false;
}

bool checkLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2) {
    for(auto& b : collisions) {
        if (b.type == 4) continue; // Ignore door interaction zones
        
        float tmin = 0.0f, tmax = 1.0f;
        
        // Check X axis
        float dx = x2 - x1;
        if (fabsf(dx) > 0.0001f) {
            float tx1 = (b.minX - x1) / dx, tx2 = (b.maxX - x1) / dx;
            tmin = fmaxf(tmin, fminf(tx1, tx2)); tmax = fminf(tmax, fmaxf(tx1, tx2));
        } else if (x1 <= b.minX || x1 >= b.maxX) continue;
        
        // Check Y axis
        float dy = y2 - y1;
        if (fabsf(dy) > 0.0001f) {
            float ty1 = (b.minY - y1) / dy, ty2 = (b.maxY - y1) / dy;
            tmin = fmaxf(tmin, fminf(ty1, ty2)); tmax = fminf(tmax, fmaxf(ty1, ty2));
        } else if (y1 <= b.minY || y1 >= b.maxY) continue;
        
        // Check Z axis
        float dz = z2 - z1;
        if (fabsf(dz) > 0.0001f) {
            float tz1 = (b.minZ - z1) / dz, tz2 = (b.maxZ - z1) / dz;
            tmin = fmaxf(tmin, fminf(tz1, tz2)); tmax = fminf(tmax, fmaxf(tz1, tz2));
        } else if (z1 <= b.minZ || z1 >= b.maxZ) continue;
        
        // If the line segment intersects the box, LOS is blocked
        if (tmin <= tmax && tmax > 0.0f && tmin < 1.0f) return false; 
    }
    return true; // Clear line of sight
}

void buildLamp(float x, float z, float L = 1.0f) {
    addBox(x-0.15f, 0.46f, z-0.15f, 0.3f, 0.05f, 0.3f, 0.3f, 0.15f, 0.05f, false, 0, L);
    addBox(x-0.05f, 0.51f, z-0.05f, 0.1f, 0.2f, 0.1f, 0.2f, 0.3f, 0.4f, false, 0, L);
    addBox(x-0.2f, 0.71f, z-0.2f, 0.4f, 0.25f, 0.4f, 0.9f, 0.9f, 0.8f, false, 0, L);
}

void buildCabinet(float zCenter, bool isLeft, float L = 1.0f, float offsetX = 0.0f, int itemType = 0) {
    float backX = (isLeft ? -2.95f : 2.85f) + offsetX, topX = (isLeft ? -2.95f : 2.15f) + offsetX, frontX = (isLeft ? -2.25f : 2.15f) + offsetX; 
    addBox(backX, 0, zCenter - 0.5f, 0.1f, 1.5f, 1.0f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(topX, 1.5f, zCenter - 0.5f, 0.8f, 0.1f, 1.0f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(topX, 0, zCenter - 0.5f, 0.8f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(topX, 0, zCenter + 0.4f, 0.8f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(frontX, 0, zCenter - 0.4f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(frontX, 0, zCenter + 0.05f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    float handleX = (isLeft ? frontX + 0.1f : frontX - 0.03f), hR = 0.9f, hG = 0.75f, hB = 0.1f; 
    addBox(handleX, 0.6f, zCenter - 0.15f, 0.03f, 0.15f, 0.04f, hR, hG, hB, false, 0, L); 
    addBox(handleX, 0.6f, zCenter + 0.11f, 0.03f, 0.15f, 0.04f, hR, hG, hB, false, 0, L); 
    if (hideState != IN_CABINET) addBox((isLeft ? -2.85f : 2.25f) + offsetX, 0.01f, zCenter - 0.4f, 0.6f, 1.48f, 0.8f, 0.0f, 0.0f, 0.0f, false, 0, L);
    collisions.push_back({(isLeft ? -2.9f : 2.1f) + offsetX, 0.0f, zCenter - 0.5f, (isLeft ? -2.1f : 2.9f) + offsetX, 1.5f, zCenter + 0.5f, 1});
}

void buildBed(float zCenter, bool isLeft, int itemType, float L = 1.0f, float offsetX = 0.0f) {
    float x = (isLeft ? -2.95f : 1.55f) + offsetX, skirtX = (isLeft ? -1.65f : 1.55f) + offsetX, pillowX = (isLeft ? -2.65f : 2.15f) + offsetX; 
    addBox(x, 0.4f, zCenter + 1.25f, 1.4f, 0.1f, -2.5f, 0.4f, 0.1f, 0.1f, true, 0, L); 
    addBox(x, 0.0f, zCenter + 1.25f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L); addBox(x + 1.3f, 0.0f, zCenter + 1.25f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L);
    addBox(x, 0.0f, zCenter - 1.15f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L); addBox(x + 1.3f, 0.0f, zCenter - 1.15f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L);
    addBox(skirtX, 0.2f, zCenter + 1.25f, 0.1f, 0.2f, -2.5f, 0.4f, 0.1f, 0.1f, true, 0, L); addBox(pillowX, 0.5f, zCenter + 1.0f, 0.5f, 0.08f, -0.6f, 0.9f, 0.9f, 0.9f, false, 0, L); 
    if (itemType == 1) { 
        float ix = (isLeft ? -2.2f : 2.1f) + offsetX; float kR=0.9f, kG=0.75f, kB=0.1f;
        addBox(ix, 0.52f, zCenter - 0.1f, 0.14f, 0.035f, 0.035f, kR,kG,kB, false, 0, L);
        addBox(ix + 0.1f, 0.52f, zCenter - 0.13f, 0.07f, 0.035f, 0.1f, kR,kG,kB, false, 0, L);
        addBox(ix + 0.03f, 0.52f, zCenter - 0.065f, 0.035f, 0.035f, 0.05f, kR,kG,kB, false, 0, L);
    } else if (itemType == 3) { 
        addBox((isLeft ? -2.2f : 2.1f) + offsetX + 0.02f, 0.52f, zCenter - 0.01f, 0.04f, 0.005f, 0.04f, 1.0f, 0.85f, 0.0f, false, 0, L);
    }
    collisions.push_back({x, 0.0f, zCenter - 1.25f, x + 1.4f, 0.6f, zCenter + 1.25f, 2});
}

void buildDresser(float zCenter, bool isLeft, bool isOpen, int itemType, float L = 1.0f, float offsetX = 0.0f, bool hasLamp = false) {
    float frameX = (isLeft ? -2.95f : 2.45f) + offsetX, openOffset = isOpen ? (isLeft ? 0.35f : -0.35f) : 0.0f;
    float trayX = (isLeft ? (-2.9f + openOffset) : (2.4f + openOffset)) + offsetX, handleX = (isLeft ? (-2.4f + openOffset) : (2.35f + openOffset)) + offsetX, itemX = (isLeft ? (-2.6f + openOffset) : (2.5f + openOffset)) + offsetX;
    addBox(frameX + 0.05f, 0.0f, zCenter - 0.35f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L);
    addBox(frameX + 0.05f, 0.0f, zCenter + 0.3f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L);
    addBox(frameX + 0.4f, 0.0f, zCenter - 0.35f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L);
    addBox(frameX + 0.4f, 0.0f, zCenter + 0.3f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L);
    addBox(frameX, 0.2f, zCenter - 0.4f, 0.5f, 0.3f, 0.8f, 0.3f, 0.15f, 0.1f, true, 3, L);
    addBox(trayX, 0.3f, zCenter - 0.35f, 0.5f, 0.15f, 0.7f, 0.25f, 0.12f, 0.08f, false, 0, L);
    addBox(handleX, 0.35f, zCenter - 0.1f, 0.05f, 0.05f, 0.2f, 0.8f, 0.8f, 0.8f, false, 0, L);
    if (isOpen) {
        if (itemType == 1) { float kR=0.9f, kG=0.75f, kB=0.1f; 
            addBox(itemX, 0.46f, zCenter - 0.1f, 0.14f, 0.035f, 0.035f, kR,kG,kB, false, 0, L);
            addBox(itemX + 0.1f, 0.46f, zCenter - 0.13f, 0.07f, 0.035f, 0.1f, kR,kG,kB, false, 0, L);
            addBox(itemX + 0.03f, 0.46f, zCenter - 0.065f, 0.035f, 0.035f, 0.05f, kR,kG,kB, false, 0, L);
        }
        else if (itemType == 2) addBox(itemX, 0.46f, zCenter - 0.05f, 0.15f, 0.02f, 0.08f, 0.8f, 0.6f, 0.4f, false, 0, L);
        else if (itemType == 3) addBox(itemX + 0.02f, 0.46f, zCenter - 0.05f, 0.04f, 0.005f, 0.04f, 1.0f, 0.85f, 0.0f, false, 0, L);
    }
    
    if (hasLamp) {
        float lampX = (isLeft ? -2.7f : 2.7f) + offsetX;
        buildLamp(lampX, zCenter, L);
    }
}

void buildChest(float x, float z, bool isOpen, float L = 1.0f) {
    addBox(x - 0.4f, 0.0f, z - 0.3f, 0.8f, 0.4f, 0.6f, 0.3f, 0.15f, 0.05f, true, 0, L);
    addBox(x - 0.42f, 0.0f, z - 0.32f, 0.05f, 0.4f, 0.05f, 0.8f, 0.7f, 0.1f, false, 0, L);
    addBox(x + 0.37f, 0.0f, z - 0.32f, 0.05f, 0.4f, 0.05f, 0.8f, 0.7f, 0.1f, false, 0, L);
    if (!isOpen) { addBox(x - 0.4f, 0.4f, z - 0.3f, 0.8f, 0.2f, 0.6f, 0.35f, 0.18f, 0.08f, true, 0, L); addBox(x - 0.05f, 0.3f, z + 0.3f, 0.1f, 0.15f, 0.05f, 0.8f, 0.8f, 0.8f, false, 0, L); }
    else { addBox(x - 0.4f, 0.4f, z - 0.4f, 0.8f, 0.6f, 0.1f, 0.35f, 0.18f, 0.08f, true, 0, L); addBox(x - 0.2f, 0.35f, z - 0.1f, 0.4f, 0.05f, 0.2f, 1.0f, 0.85f, 0.0f, false, 0, L); }
}

void addWallWithDoors(float z, bool leftDoor, bool leftOpen, bool centerDoor, bool centerOpen, bool rightDoor, bool rightOpen, int roomIndex, float L = 1.0f) {
    addBox(-3.0f, 0, z, 0.4f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);
    if(!leftDoor) addBox(-2.6f, 0, z, 1.2f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L); else addBox(-2.6f, 1.4f, z, 1.2f, 0.4f, -0.2f, 0.2f, 0.15f, 0.1f, false, 0, L);
    addBox(-1.4f, 0, z, 0.8f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);
    if(!centerDoor) addBox(-0.6f, 0, z, 1.2f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L); else addBox(-0.6f, 1.4f, z, 1.2f, 0.4f, -0.2f, 0.2f, 0.15f, 0.1f, false, 0, L);
    addBox(0.6f, 0, z, 0.8f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);
    if(!rightDoor) addBox(1.4f, 0, z, 1.2f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L); else addBox(1.4f, 1.4f, z, 1.2f, 0.4f, -0.2f, 0.2f, 0.15f, 0.1f, false, 0, L);
    addBox(2.6f, 0, z, 0.4f, 1.8f, -0.2f, 0.2f, 0.15f, 0.1f, true, 0, L);

    auto drawDoor = [&](float dx, bool isOpen) {
        if (!isOpen) {
            addBox(dx, 0.0f, z, 1.2f, 1.4f, -0.1f, 0.15f, 0.08f, 0.05f, true, 0, L); 
            addBox(dx+0.4f, 1.1f, z+0.02f, 0.4f, 0.12f, 0.02f, 0.8f, 0.7f, 0.2f, false, 0, L); 
            addBox(dx+1.05f, 0.7f, z+0.02f, 0.05f, 0.15f, 0.03f, 0.6f, 0.6f, 0.6f, false, 0, L); 
            if(rooms[roomIndex].isLocked) addBox(dx+0.9f, 0.6f, z+0.05f, 0.2f, 0.2f, 0.05f, 0.8f, 0.8f, 0.8f, false, 0, L); 
        } else {
            addBox(dx, 0.0f, z, 0.1f, 1.4f, -1.2f, 0.3f, 0.15f, 0.08f, true, 0, L); 
            addBox(dx+0.1f, 1.1f, z-0.8f, 0.02f, 0.12f, 0.4f, 0.8f, 0.7f, 0.2f, false, 0, L); 
            addBox(dx+0.1f, 0.7f, z-1.05f, 0.03f, 0.15f, 0.05f, 0.6f, 0.6f, 0.6f, false, 0, L); 
        }
    };
    if(leftDoor) drawDoor(-2.6f, leftOpen); if(centerDoor) drawDoor(-0.6f, centerOpen); if(rightDoor) drawDoor(1.4f, rightOpen);
}

void buildWorld(int currentChunk, int playerCurrentRoom) {
    world_mesh.clear(); collisions.clear();
    
    if (screechActive) {
        addBox(screechX - 0.2f, screechY, screechZ - 0.2f, 0.4f, 0.4f, 0.4f, 0.05f, 0.05f, 0.05f, false);
        addBox(screechX - 0.22f, screechY + 0.1f, screechZ - 0.22f, 0.44f, 0.05f, 0.44f, 0.9f, 0.9f, 0.9f, false);
        addBox(screechX - 0.22f, screechY + 0.25f, screechZ - 0.22f, 0.44f, 0.05f, 0.44f, 0.9f, 0.9f, 0.9f, false);
    }

    if (rushActive && rushState == 2) {
        addBox(-1.2f, 0.2f, rushZ - 0.5f, 2.4f, 2.0f, 1.0f, 0.05f, 0.05f, 0.05f, false); 
        addBox(-0.8f, 1.4f, rushZ - 0.55f, 0.4f, 0.4f, 0.1f, 0.9f, 0.9f, 0.9f, false); 
        addBox(0.4f, 1.4f, rushZ - 0.55f, 0.4f, 0.4f, 0.1f, 0.9f, 0.9f, 0.9f, false);  
        addBox(-0.6f, 0.5f, rushZ - 0.55f, 1.2f, 0.6f, 0.1f, 0.8f, 0.8f, 0.8f, false); 
    }

    if (seekActive) {
        float sY = 0.0f, sH = 1.1f; 
        if (seekState == 1) {
            if (seekTimer <= 130) sY = -1.1f + (seekTimer / 130.0f) * 1.1f; 
            else { sY = 0.0f; sH = 1.1f; }
            srand(seekTimer); 
            for (int d = 0; d < 8; d++) {
                float dropX = -1.5f + (rand() % 30) / 10.0f, dropZ = seekZ - 0.5f - (rand() % 30) / 10.0f, dropY = 1.8f - fmod((seekTimer + d * 20) * 0.05f, 1.8f);
                addBox(dropX, dropY, dropZ, 0.08f, 0.2f, 0.08f, 0.05f, 0.05f, 0.05f, false);
            }
            srand(time(NULL));
        } else if (seekState == 2) { sY = 0.0f; sH = 1.1f; }
        
        addBox(-0.3f, sY, seekZ - 0.3f, 0.6f, sH, 0.6f, 0.05f, 0.05f, 0.05f, false); 
        addBox(-0.15f, sY + 0.8f, seekZ - 0.35f, 0.3f, 0.2f, 0.06f, 0.9f, 0.9f, 0.9f, false, 0, 1.5f); 
        addBox(-0.05f, sY + 0.8f, seekZ - 0.38f, 0.1f, 0.2f, 0.04f, 0.0f, 0.0f, 0.0f, false, 0, 1.5f); 
        if (seekState == 1) addBox(-1.0f, 0.01f, seekZ - 1.0f, 2.0f, 0.01f, 2.0f, 0.02f, 0.02f, 0.02f, false);
    }
    
    if (currentChunk < 2) {
        globalTintR = 1.0f; globalTintG = 1.0f; globalTintB = 1.0f;
        addBox(-2.0f, 0.0f, 5.0f, 4.0f, 0.01f, 4.0f, 0.2f, 0.2f, 0.2f, false); addBox(-2.0f, 2.0f, 5.0f, 4.0f, 0.1f, 4.0f, 0.8f, 0.8f, 0.8f, false); 
        addBox(-2.0f, 0.0f, 9.0f, 4.0f, 2.0f, 0.1f, 0.4f, 0.3f, 0.2f, true); addBox(-2.0f, 0.0f, 5.0f, 0.1f, 2.0f, 4.0f, 0.4f, 0.3f, 0.2f, true); addBox(1.9f, 0.0f, 5.0f, 0.1f, 2.0f, 4.0f, 0.4f, 0.3f, 0.2f, true);   
        addBox(1.8f, 0.6f, 6.5f, 0.15f, 0.3f, 0.2f, 0.1f, 0.1f, 0.1f, false); addBox(1.75f, 0.7f, 6.55f, 0.05f, 0.1f, 0.1f, 0.0f, 0.8f, 0.0f, false, 0, 1.5f); 
        addBox(-2.0f - elevatorDoorOffset, 0.0f, 5.05f, 2.0f, 2.0f, 0.1f, 0.6f, 0.6f, 0.6f, true); addBox(0.0f + elevatorDoorOffset, 0.0f, 5.05f, 2.0f, 2.0f, 0.1f, 0.6f, 0.6f, 0.6f, true);  
        if (elevatorDoorOffset < 0.05f) addBox(-0.02f, 0.0f, 5.04f, 0.04f, 2.0f, 0.12f, 0.0f, 0.0f, 0.0f, false);
        if (elevatorClosing) collisions.push_back({-2.0f, 0.0f, 4.8f, 2.0f, 2.0f, 5.1f, 0});
        addBox(-6, 0, 5.0f, 12, 0.01f, -15.0f, 0.22f, 0.15f, 0.1f, false); addBox(-6, 1.8f, 5.0f, 12, 0.01f, -15.0f, 0.1f, 0.1f, 0.1f, false); 
        addBox(-6, 0, 5.0f, 0.1f, 1.8f, -15.0f, 0.3f, 0.3f, 0.3f, true); addBox(6, 0, 5.0f, 0.1f, 1.8f, -15.0f, 0.3f, 0.3f, 0.3f, true);  
        addBox(-6.0f, 0, -10.0f, 3.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true); addBox(3.0f, 0, -10.0f, 3.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true);  
        addBox(-6.0f, 0.0f, 4.9f, 4.0f, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true); addBox(2.0f, 0.0f, 4.9f, 4.0f, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true);  
        addBox(-6.0f, 0.0f, -7.0f, 3.5f, 0.8f, -0.8f, 0.3f, 0.15f, 0.1f, true); addBox(-3.3f, 0.0f, -7.8f, 0.8f, 0.8f, -1.0f, 0.3f, 0.15f, 0.1f, true); 
        addBox(-2.5f, 0.1f, -8.6f, 1.0f, 0.05f, -1.4f, 0.8f, 0.7f, 0.2f, false); addBox(-2.5f, 0.15f, -8.6f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-1.55f, 0.15f, -8.6f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); addBox(-2.5f, 0.15f, -9.95f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-1.55f, 0.15f, -9.95f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); addBox(-2.5f, 0.6f, -8.6f, 1.0f, 0.05f, -1.4f, 0.8f, 0.7f, 0.2f, true); 
        if(!lobbyKeyPickedUp) { addBox(-4.8f, 0.9f, -9.9f, 0.2f, 0.2f, 0.05f, 0.3f, 0.2f, 0.1f, false); addBox(-4.72f, 0.75f, -9.86f, 0.035f, 0.1f, 0.035f, 1.0f, 0.84f, 0.0f, false); }
    }

    int startRoom = playerCurrentRoom - 1, endRoom = playerCurrentRoom + 2; 
    if (playerCurrentRoom >= seekStartRoom && playerCurrentRoom <= seekStartRoom + 2) { startRoom = seekStartRoom; endRoom = seekStartRoom + 3; }
    if (startRoom < 0) startRoom = 0; if (endRoom > TOTAL_ROOMS - 1) endRoom = TOTAL_ROOMS - 1;

    for(int i = startRoom; i <= endRoom; i++) {
        float z = -10 - (i * 10), L = rooms[i].lightLevel, wallL = (i > 0) ? rooms[i-1].lightLevel : 1.0f;
        bool isTwoAheadNormal = (!rooms[i].isSeekChase && !rooms[i].isSeekHallway && !rooms[i].isSeekFinale && i == playerCurrentRoom + 2);

        if (seekState == 1) { globalTintR = 1.0f; globalTintG = 0.2f; globalTintB = 0.2f; } 
        else { globalTintR = 1.0f; globalTintG = 1.0f; globalTintB = 1.0f; }

        if (i == seekStartRoom + 9 && rooms[i].isLocked) addBox(-3.0f, 0.0f, z + 0.2f, 6.0f, 1.8f, 0.1f, 0.4f, 0.7f, 1.0f, true, 0, 1.5f);

        if (!(i == seekStartRoom + 1 || i == seekStartRoom + 2)) {
            if (rooms[i].isDupeRoom) {
                if (playerCurrentRoom >= i) addWallWithDoors(z, (rooms[i].correctDupePos == 0), ((rooms[i].correctDupePos == 0) && doorOpen[i]), (rooms[i].correctDupePos == 1), ((rooms[i].correctDupePos == 1) && doorOpen[i]), (rooms[i].correctDupePos == 2), ((rooms[i].correctDupePos == 2) && doorOpen[i]), i, wallL);
                else addWallWithDoors(z, true, (rooms[i].correctDupePos == 0 && doorOpen[i]), true, (rooms[i].correctDupePos == 1 && doorOpen[i]), true, (rooms[i].correctDupePos == 2 && doorOpen[i]), i, wallL);
            } else {
                addWallWithDoors(z, (rooms[i].doorPos == 0), ((rooms[i].doorPos == 0) && doorOpen[i]), (rooms[i].doorPos == 1), ((rooms[i].doorPos == 1) && doorOpen[i]), (rooms[i].doorPos == 2), ((rooms[i].doorPos == 2) && doorOpen[i]), i, wallL);
            }
        }

        if (seekState == 1) { globalTintR = 1.0f; globalTintG = 0.2f; globalTintB = 0.2f; } 
        else if (rooms[i].hasEyes) { globalTintR = 0.8f; globalTintG = 0.3f; globalTintB = 1.0f; } 
        else { globalTintR = 1.0f; globalTintG = 1.0f; globalTintB = 1.0f; }

        bool renderEyes = !(rooms[i].isSeekChase || rooms[i].hasSeekEyes) || (i >= playerCurrentRoom && i <= playerCurrentRoom + 1);

        if (renderEyes && (rooms[i].hasSeekEyes || rooms[i].isSeekChase)) {
            srand(i * 12345); int wallEyeCount = rooms[i].hasSeekEyes ? rooms[i].seekEyeCount : 15; 
            for (int e = 0; e < wallEyeCount; e++) {
                bool isLeftWall = (rand() % 2 == 0); float eyeZ = z - 0.5f - (rand() % 90) / 10.0f, eyeY = 0.2f + (rand() % 160) / 100.0f;   
                if (isLeftWall) { addBox(-2.95f, eyeY, eyeZ, 0.1f, 0.3f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L); addBox(-2.88f, eyeY + 0.05f, eyeZ + 0.05f, 0.06f, 0.2f, 0.3f, 0.9f, 0.9f, 0.9f, false, 0, L); addBox(-2.84f, eyeY + 0.1f, eyeZ + 0.12f, 0.04f, 0.1f, 0.16f, 0.0f, 0.0f, 0.0f, false, 0, 1.5f); } 
                else { addBox(2.85f, eyeY, eyeZ, 0.1f, 0.3f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L); addBox(2.82f, eyeY + 0.05f, eyeZ + 0.05f, 0.06f, 0.2f, 0.3f, 0.9f, 0.9f, 0.9f, false, 0, L); addBox(2.80f, eyeY + 0.1f, eyeZ + 0.12f, 0.04f, 0.1f, 0.16f, 0.0f, 0.0f, 0.0f, false, 0, 1.5f); }
            } srand(time(NULL)); 
        }

        if (rooms[i].isSeekChase) {
            srand(i * 777); int obType = rand() % 3; float obZ = z - 5.0f;    
            if (obType == 0) addBox(-3.0f, 0.7f, obZ, 6.0f, 1.1f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L); 
            else if (obType == 1) { addBox(-3.0f, 0.0f, obZ, 3.0f, 1.8f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L); addBox(0.0f, 0.7f, obZ, 3.0f, 1.1f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L); } 
            else { addBox(0.0f, 0.0f, obZ, 3.0f, 1.8f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L); addBox(-3.0f, 0.7f, obZ, 3.0f, 1.1f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L); }
            srand(time(NULL));
        } else if (rooms[i].isSeekFinale) {
            addBox(-2.95f, 0.4f, z - 8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L); addBox(2.85f, 0.4f, z - 8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L);
            addBox(-3.0f, 0.0f, z - 2.0f, 3.5f, 1.8f, 0.4f, 0.05f, 0.05f, 0.05f, true, 0, L); addBox(-0.1f, 0.5f, z - 2.1f, 0.6f, 0.6f, 0.6f, 1.0f, 0.0f, 0.0f, false, 0, 1.5f); rooms[i].pW[0] = 2.6f; rooms[i].pZ[0] = z - 2.0f; rooms[i].pSide[0] = 1; addBox(2.0f, 0.8f, z - 2.2f, 1.0f, 0.2f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L);
            rooms[i].pW[1] = 1.8f; rooms[i].pZ[1] = z - 3.5f; rooms[i].pSide[1] = 0; addBox(1.4f, 0.0f, z - 3.9f, 0.8f, 0.3f, 0.8f, 1.0f, 0.4f, 0.0f, false, 0, L); addBox(1.6f, 0.3f, z - 3.7f, 0.4f, 0.4f, 0.4f, 1.0f, 0.8f, 0.0f, false, 0, L);
            addBox(-0.5f, 0.0f, z - 5.0f, 3.5f, 1.8f, 0.4f, 0.05f, 0.05f, 0.05f, true, 0, L); addBox(-0.5f, 0.5f, z - 5.1f, 0.6f, 0.6f, 0.6f, 1.0f, 0.0f, 0.0f, false, 0, 1.5f); rooms[i].pW[2] = -2.6f; rooms[i].pZ[2] = z - 5.0f; rooms[i].pSide[2] = 1; addBox(-3.0f, 0.8f, z - 5.2f, 1.0f, 0.2f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L);
            rooms[i].pW[3] = -1.8f; rooms[i].pZ[3] = z - 6.5f; rooms[i].pSide[3] = 0; addBox(-2.2f, 0.0f, z - 6.9f, 0.8f, 0.3f, 0.8f, 1.0f, 0.4f, 0.0f, false, 0, L); addBox(-2.0f, 0.3f, z - 6.7f, 0.4f, 0.4f, 0.4f, 1.0f, 0.8f, 0.0f, false, 0, L);
            addBox(-3.0f, 0.0f, z - 8.0f, 3.5f, 1.8f, 0.4f, 0.05f, 0.05f, 0.05f, true, 0, L); addBox(-0.1f, 0.5f, z - 8.1f, 0.6f, 0.6f, 0.6f, 1.0f, 0.0f, 0.0f, false, 0, 1.5f); rooms[i].pW[4] = 2.6f; rooms[i].pZ[4] = z - 8.0f; rooms[i].pSide[4] = 1; addBox(2.0f, 0.8f, z - 8.2f, 1.0f, 0.2f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L);
            rooms[i].pW[5] = 0.8f; rooms[i].pZ[5] = z - 9.0f; rooms[i].pSide[5] = 0; addBox(0.4f, 0.0f, z - 9.4f, 0.8f, 0.3f, 0.8f, 1.0f, 0.4f, 0.0f, false, 0, L); addBox(0.6f, 0.3f, z - 9.2f, 0.4f, 0.4f, 0.4f, 1.0f, 0.8f, 0.0f, false, 0, L);
        } else if (rooms[i].isSeekHallway) {
            addBox(-2.95f, 0.4f, z - 8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L); addBox(2.85f, 0.4f, z - 8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L);  
        } else if (rooms[i].hasEyes && renderEyes) {
            float ex = rooms[i].eyesX, ey = rooms[i].eyesY, ez = rooms[i].eyesZ;
            addBox(ex - 0.2f, ey - 0.2f, ez - 0.2f, 0.4f, 0.4f, 0.4f, 0.6f, 0.1f, 0.8f, false, 0, L); addBox(ex - 0.25f, ey - 0.1f, ez - 0.1f, 0.5f, 0.2f, 0.2f, 0.9f, 0.9f, 0.9f, false, 0, L);
            addBox(ex - 0.1f, ey - 0.25f, ez - 0.1f, 0.2f, 0.5f, 0.2f, 0.9f, 0.9f, 0.9f, false, 0, L); addBox(ex - 0.26f, ey - 0.05f, ez - 0.05f, 0.02f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, false, 0, 1.5f); addBox(ex + 0.24f, ey - 0.05f, ez - 0.05f, 0.02f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, false, 0, 1.5f);
        } 

        addBox(-3, 0, z, 6, 0.01f, -10, 0.2f, 0.1f, 0.05f, false, 0, L); addBox(-3, 1.8f, z, 6, 0.01f, -10, 0.15f, 0.15f, 0.15f, false, 0, L); 

        if (rooms[i].hasLeftRoom) {
            float doorZ = z + rooms[i].leftDoorOffset, beforeLen = abs(doorZ - z), afterLen = abs((z - 10.0f) - (doorZ - 1.2f));
            if (beforeLen > 0.05f) addBox(-3.0f, 0, z, 0.1f, 1.8f, -beforeLen, 0.25f, 0.2f, 0.15f, true, 0, L); 
            if (afterLen > 0.05f) addBox(-3.0f, 0, doorZ - 1.2f, 0.1f, 1.8f, -afterLen, 0.25f, 0.2f, 0.15f, true, 0, L); 
            addBox(-3.0f, 1.4f, doorZ, 0.1f, 0.4f, -1.2f, 0.25f, 0.2f, 0.15f, false, 0, L); 
            addBox(-2.95f, 0, doorZ, 0.05f, 1.4f, -0.05f, 0.15f, 0.1f, 0.05f, false, 0, L); addBox(-2.95f, 0, doorZ - 1.15f, 0.05f, 1.4f, -0.05f, 0.15f, 0.1f, 0.05f, false, 0, L); addBox(-2.95f, 1.35f, doorZ, 0.05f, 0.05f, -1.2f, 0.15f, 0.1f, 0.05f, false, 0, L); 
            if (rooms[i].leftDoorOpen) { addBox(-4.1f, 0, doorZ - 1.15f, 1.1f, 1.4f, 0.05f, 0.12f, 0.06f, 0.03f, true, 0, L); addBox(-4.0f, 0.7f, doorZ - 1.10f, 0.1f, 0.05f, 0.05f, 0.8f, 0.7f, 0.2f, false, 0, L); collisions.push_back({-4.1f, 0.0f, doorZ - 1.15f, -3.0f, 1.8f, doorZ - 1.10f, 4}); } 
            else { addBox(-3.0f, 0, doorZ - 0.05f, 0.05f, 1.4f, -1.1f, 0.12f, 0.06f, 0.03f, true, 0, L); addBox(-2.95f, 0.7f, doorZ - 0.15f, 0.05f, 0.1f, -0.1f, 0.8f, 0.7f, 0.2f, false, 0, L); } 
            
            float srZ = doorZ + 2.5f; 
            addBox(-9.0f, 0, srZ, 6.0f, 0.01f, -5.0f, 0.18f, 0.1f, 0.05f, false, 0, L); addBox(-9.0f, 1.8f, srZ, 6.0f, 0.01f, -5.0f, 0.12f, 0.12f, 0.12f, false, 0, L); 
            addBox(-9.0f, 0, srZ, 0.1f, 1.8f, -5.0f, 0.25f, 0.2f, 0.15f, true, 0, L); addBox(-9.0f, 0, srZ, 6.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true, 0, L); addBox(-9.0f, 0, srZ - 5.0f, 6.0f, 1.8f, -0.1f, 0.25f, 0.2f, 0.15f, true, 0, L); 

            srand(i * 123);
            if (rand() % 2 == 0) { 
                float pY = 0.6f + (rand() % 50) / 100.0f, pZ = srZ - 1.5f - (rand() % 20) / 10.0f, pW = 0.5f + (rand() % 40) / 100.0f, pH = 0.5f + (rand() % 40) / 100.0f;
                float pR = 0.15f + (rand() % 35) / 100.0f, pG = 0.15f + (rand() % 35) / 100.0f, pB = 0.15f + (rand() % 35) / 100.0f;
                addBox(-8.95f, pY - 0.05f, pZ + 0.05f, 0.06f, pH + 0.1f, -pW - 0.1f, 0.1f, 0.05f, 0.02f, false, 0, L); addBox(-8.9f, pY, pZ, 0.07f, pH, -pW, pR, pG, pB, false, 0, L); 
            } srand(time(NULL));

            for (int s=0; s<3; s++) {
                float fZ = srZ - 0.9f - (s * 1.6f);
                int typeL = rooms[i].leftRoomSlotTypeL[s];
                if (typeL == 1) buildCabinet(fZ, true, L, -6.0f, rooms[i].leftRoomSlotItemL[s]); 
                else if (typeL == 3) buildBed(fZ, true, rooms[i].leftRoomSlotItemL[s], L, -6.0f); 
                else if (typeL == 5) buildDresser(fZ, true, rooms[i].leftRoomDrawerOpenL[s], rooms[i].leftRoomSlotItemL[s], L, -6.0f, true);
                else if (typeL == 7 || typeL == 8) buildChest(-8.4f, fZ, rooms[i].leftRoomDrawerOpenL[s], L);

                int typeR = rooms[i].leftRoomSlotTypeR[s];
                if (typeR == 2) buildCabinet(fZ, false, L, -6.0f, rooms[i].leftRoomSlotItemR[s]); 
                else if (typeR == 4) buildBed(fZ, false, rooms[i].leftRoomSlotItemR[s], L, -6.0f); 
                else if (typeR == 6) buildDresser(fZ, false, rooms[i].leftRoomDrawerOpenR[s], rooms[i].leftRoomSlotItemR[s], L, -6.0f, true);
                else if (typeR == 7 || typeR == 8) buildChest(-3.6f, fZ, rooms[i].leftRoomDrawerOpenR[s], L);
            }
        } else addBox(-3, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true, 0, L); 

        if (rooms[i].hasRightRoom) {
            float doorZ = z + rooms[i].rightDoorOffset, beforeLen = abs(doorZ - z), afterLen = abs((z - 10.0f) - (doorZ - 1.2f));
            if (beforeLen > 0.05f) addBox(2.9f, 0, z, 0.1f, 1.8f, -beforeLen, 0.25f, 0.2f, 0.15f, true, 0, L); 
            if (afterLen > 0.05f) addBox(2.9f, 0, doorZ - 1.2f, 0.1f, 1.8f, -afterLen, 0.25f, 0.2f, 0.15f, true, 0, L); 
            addBox(2.9f, 1.4f, doorZ, 0.1f, 0.4f, -1.2f, 0.25f, 0.2f, 0.15f, false, 0, L); 
            addBox(2.9f, 0, doorZ, 0.05f, 1.4f, -0.05f, 0.15f, 0.1f, 0.05f, false, 0, L); addBox(2.9f, 0, doorZ - 1.15f, 0.05f, 1.4f, -0.05f, 0.15f, 0.1f, 0.05f, false, 0, L); addBox(2.9f, 1.35f, doorZ, 0.05f, 0.05f, -1.2f, 0.15f, 0.1f, 0.05f, false, 0, L); 
            if (rooms[i].rightDoorOpen) { addBox(3.0f, 0, doorZ - 1.15f, 1.1f, 1.4f, 0.05f, 0.12f, 0.06f, 0.03f, true, 0, L); addBox(3.9f, 0.7f, doorZ - 1.10f, 0.1f, 0.05f, 0.05f, 0.8f, 0.7f, 0.2f, false, 0, L); collisions.push_back({3.0f, 0.0f, doorZ - 1.15f, 4.1f, 1.8f, doorZ - 1.10f, 4}); } 
            else { addBox(2.95f, 0, doorZ - 0.05f, 0.05f, 1.4f, -1.1f, 0.12f, 0.06f, 0.03f, true, 0, L); addBox(2.9f, 0.7f, doorZ - 0.15f, 0.05f, 0.1f, -0.1f, 0.8f, 0.7f, 0.2f, false, 0, L); }
            
            float srZ = doorZ + 2.5f; 
            addBox(3.0f, 0, srZ, 6.0f, 0.01f, -5.0f, 0.18f, 0.1f, 0.05f, false, 0, L); addBox(3.0f, 1.8f, srZ, 6.0f, 0.01f, -5.0f, 0.12f, 0.12f, 0.12f, false, 0, L); 
            addBox(8.9f, 0, srZ, 0.1f, 1.8f, -5.0f, 0.25f, 0.2f, 0.15f, true, 0, L); addBox(3.0f, 0, srZ, 6.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true, 0, L); addBox(3.0f, 0, srZ - 5.0f, 6.0f, 1.8f, -0.1f, 0.25f, 0.2f, 0.15f, true, 0, L); 

            srand(i * 321);
            if (rand() % 2 == 0) { 
                float pY = 0.6f + (rand() % 50) / 100.0f, pZ = srZ - 1.5f - (rand() % 20) / 10.0f, pW = 0.5f + (rand() % 40) / 100.0f, pH = 0.5f + (rand() % 40) / 100.0f;
                float pR = 0.15f + (rand() % 35) / 100.0f, pG = 0.15f + (rand() % 35) / 100.0f, pB = 0.15f + (rand() % 35) / 100.0f;
                addBox(8.89f, pY - 0.05f, pZ + 0.05f, 0.06f, pH + 0.1f, -pW - 0.1f, 0.1f, 0.05f, 0.02f, false, 0, L); addBox(8.83f, pY, pZ, 0.07f, pH, -pW, pR, pG, pB, false, 0, L); 
            } srand(time(NULL));

            for (int s=0; s<3; s++) {
                float fZ = srZ - 0.9f - (s * 1.6f);
                int typeL = rooms[i].rightRoomSlotTypeL[s];
                if (typeL == 1) buildCabinet(fZ, true, L, 6.0f, rooms[i].rightRoomSlotItemL[s]); 
                else if (typeL == 3) buildBed(fZ, true, rooms[i].rightRoomSlotItemL[s], L, 6.0f); 
                else if (typeL == 5) buildDresser(fZ, true, rooms[i].rightRoomDrawerOpenL[s], rooms[i].rightRoomSlotItemL[s], L, 6.0f, true);
                else if (typeL == 7 || typeL == 8) buildChest(3.6f, fZ, rooms[i].rightRoomDrawerOpenL[s], L);

                int typeR = rooms[i].rightRoomSlotTypeR[s];
                if (typeR == 2) buildCabinet(fZ, false, L, 6.0f, rooms[i].rightRoomSlotItemR[s]); 
                else if (typeR == 4) buildBed(fZ, false, rooms[i].rightRoomSlotItemR[s], L, 6.0f); 
                else if (typeR == 6) buildDresser(fZ, false, rooms[i].rightRoomDrawerOpenR[s], rooms[i].rightRoomSlotItemR[s], L, 6.0f, true);
                else if (typeR == 7 || typeR == 8) buildChest(8.4f, fZ, rooms[i].rightRoomDrawerOpenR[s], L);
            }
        } else addBox(2.9f, 0, z, 0.1f, 1.8f, -10, 0.25f, 0.2f, 0.15f, true, 0, L); 

        addBox(-0.4f, 1.78f, z - 5.4f, 0.8f, 0.02f, 0.8f, (L > 0.5f ? 0.9f : 0.2f), (L > 0.5f ? 0.9f : 0.2f), (L > 0.5f ? 0.8f : 0.2f), false);

        if (!isTwoAheadNormal) {
            for(int s=0; s<3; s++) {
                float zCenter = z - 2.5f - (s * 2.5f); int type = rooms[i].slotType[s];
                if (type == 1) buildCabinet(zCenter, true, L); else if (type == 2) buildCabinet(zCenter, false, L);
                else if (type == 5) buildDresser(zCenter, true, rooms[i].drawerOpen[s], rooms[i].slotItem[s], L); else if (type == 6) buildDresser(zCenter, false, rooms[i].drawerOpen[s], rooms[i].slotItem[s], L);
            }

            for(int p=0; p<rooms[i].pCount; p++) {
                float pZ = rooms[i].pZ[p], pH = rooms[i].pH[p], pW = rooms[i].pW[p], pY = rooms[i].pY[p];
                bool isSeekPainting = rooms[i].hasSeekEyes; 
                float canvasR = isSeekPainting ? 0.02f : rooms[i].pR[p], canvasG = isSeekPainting ? 0.02f : rooms[i].pG[p], canvasB = isSeekPainting ? 0.02f : rooms[i].pB[p];

                if (rooms[i].pSide[p] == 0) {
                    addBox(-2.95f, pY - 0.05f, z - pZ + 0.05f, 0.06f, pH + 0.1f, -pW - 0.1f, 0.1f, 0.05f, 0.02f, false, 0, L); addBox(-2.95f, pY, z - pZ, 0.07f, pH, -pW, canvasR, canvasG, canvasB, false, 0, L); 
                    if (isSeekPainting && renderEyes) {
                        srand(i * 100 + p); float area = pH * pW; int numEyes = 2 + (int)(area * 30.0f) + (rand() % 4); 
                        for(int e=0; e<numEyes; e++) { float eY = pY + 0.02f + (rand() % (int)((pH - 0.04f) * 100)) / 100.0f, eZ = z - pZ - 0.02f - (rand() % (int)((pW - 0.04f) * 100)) / 100.0f;
                            addBox(-2.88f, eY, eZ, 0.01f, 0.04f, 0.06f, 0.9f, 0.9f, 0.9f, false, 0, L); addBox(-2.875f, eY + 0.01f, eZ - 0.01f, 0.01f, 0.02f, 0.04f, 0.0f, 0.0f, 0.0f, false, 0, 1.5f); } srand(time(NULL));
                    }
                } else if (rooms[i].pSide[p] == 1) {
                    addBox(2.89f, pY - 0.05f, z - pZ + 0.05f, 0.06f, pH + 0.1f, -pW - 0.1f, 0.1f, 0.05f, 0.02f, false, 0, L); addBox(2.88f, pY, z - pZ, 0.07f, pH, -pW, canvasR, canvasG, canvasB, false, 0, L); 
                    if (isSeekPainting && renderEyes) {
                        srand(i * 100 + p); float area = pH * pW; int numEyes = 2 + (int)(area * 30.0f) + (rand() % 4); 
                        for(int e=0; e<numEyes; e++) { float eY = pY + 0.02f + (rand() % (int)((pH - 0.04f) * 100)) / 100.0f, eZ = z - pZ - 0.02f - (rand() % (int)((pW - 0.04f) * 100)) / 100.0f;
                            addBox(2.87f, eY, eZ, 0.01f, 0.04f, 0.06f, 0.9f, 0.9f, 0.9f, false, 0, L); addBox(2.865f, eY + 0.01f, eZ - 0.01f, 0.01f, 0.02f, 0.04f, 0.0f, 0.0f, 0.0f, false, 0, 1.5f); } srand(time(NULL));
                    }
                }
            }
        }
    }
    globalTintR = 1.0f; globalTintG = 1.0f; globalTintB = 1.0f; 
}

void generateRooms() {
    seekStartRoom = 30 + (rand() % 9);
    for(int i=0; i<TOTAL_ROOMS; i++) {
        rooms[i].doorPos = rand() % 3; rooms[i].isLocked = false; rooms[i].isJammed = false; rooms[i].hasSeekEyes = false;
        rooms[i].isSeekHallway = false; rooms[i].isSeekChase = false; rooms[i].isSeekFinale = false; rooms[i].seekEyeCount = 0;
        
        bool isSeekEvent = (i >= seekStartRoom - 5 && i <= seekStartRoom + 9); 
        
        if (i > 0 && rand() % 100 < 8 && !isSeekEvent) rooms[i].lightLevel = 0.3f; else rooms[i].lightLevel = 1.0f; 
        for(int s=0; s<3; s++) { rooms[i].drawerOpen[s] = false; rooms[i].slotItem[s] = 0; }
        if (i >= seekStartRoom - 5 && i < seekStartRoom) { rooms[i].hasSeekEyes = true; rooms[i].seekEyeCount = ((i - (seekStartRoom - 5) + 1) * 2) + 3; }
        if (i >= seekStartRoom && i <= seekStartRoom + 2) { rooms[i].isSeekHallway = true; rooms[i].doorPos = 1; }
        if (i >= seekStartRoom + 3 && i <= seekStartRoom + 7) { rooms[i].isSeekChase = true; rooms[i].doorPos = 1; }
        if (i == seekStartRoom + 8) { rooms[i].isSeekFinale = true; rooms[i].doorPos = 1; }

        bool isSeekChaseEvent = (i >= seekStartRoom && i <= seekStartRoom + 9);
        rooms[i].isDupeRoom = (!isSeekChaseEvent && i > 1 && (rand() % 100 < 15));
        if (rooms[i].isDupeRoom) {
            rooms[i].correctDupePos = rand() % 3; int dispNext = getDisplayRoom(i); 
            int fake1 = dispNext + (rand() % 3 + 1), fake2 = dispNext - (rand() % 3 + 1); if (fake2 <= 0) fake2 = dispNext + 4; 
            rooms[i].dupeNumbers[0] = fake1; rooms[i].dupeNumbers[1] = fake2; rooms[i].dupeNumbers[2] = dispNext + 5; rooms[i].dupeNumbers[rooms[i].correctDupePos] = dispNext;
        }

        rooms[i].hasEyes = (!isSeekEvent && i > 2 && !rooms[i].isDupeRoom && rand() % 100 < 8);
        if (rooms[i].hasEyes) { rooms[i].eyesX = 0.0f; rooms[i].eyesY = 0.9f; rooms[i].eyesZ = -10.0f - (i * 10.0f) - 5.0f; }

        if (!rooms[i].isSeekChase && !rooms[i].isSeekHallway && !rooms[i].isSeekFinale) {
            rooms[i].hasLeftRoom = (!isSeekEvent && !rooms[i].isDupeRoom && i > 0 && rand() % 100 < 45);
            rooms[i].hasRightRoom = (!isSeekEvent && !rooms[i].isDupeRoom && i > 0 && rand() % 100 < 45);
            rooms[i].leftDoorOpen = false; rooms[i].rightDoorOpen = false;

            if (rooms[i].hasLeftRoom) {
                rooms[i].leftDoorOffset = -3.0f - (rand() % 40) / 10.0f;
                for(int s=0; s<3; s++) {
                    rooms[i].leftRoomDrawerOpenL[s] = false;
                    if (rand() % 100 < 45) {
                        int r = rand() % 100; rooms[i].leftRoomSlotTypeL[s] = (r < 25) ? 1 : ((r < 50) ? 3 : ((r < 75) ? 5 : 7));
                        rooms[i].leftRoomSlotItemL[s] = (rooms[i].leftRoomSlotTypeL[s] >= 3 && rooms[i].leftRoomSlotTypeL[s] <= 6 && rand()%100<30) ? 3 : 0;
                    } else { rooms[i].leftRoomSlotTypeL[s] = 0; rooms[i].leftRoomSlotItemL[s] = 0; }
                    
                    rooms[i].leftRoomDrawerOpenR[s] = false;
                    if (s == 1) { rooms[i].leftRoomSlotTypeR[s] = 99; rooms[i].leftRoomSlotItemR[s] = 0; } 
                    else {
                        if (rand() % 100 < 45) {
                            int r = rand() % 100; rooms[i].leftRoomSlotTypeR[s] = (r < 25) ? 2 : ((r < 50) ? 4 : ((r < 75) ? 6 : 8));
                            rooms[i].leftRoomSlotItemR[s] = (rooms[i].leftRoomSlotTypeR[s] >= 3 && rooms[i].leftRoomSlotTypeR[s] <= 6 && rand()%100<30) ? 3 : 0;
                        } else { rooms[i].leftRoomSlotTypeR[s] = 0; rooms[i].leftRoomSlotItemR[s] = 0; }
                    }
                }
            }
            if (rooms[i].hasRightRoom) {
                rooms[i].rightDoorOffset = -3.0f - (rand() % 40) / 10.0f;
                for(int s=0; s<3; s++) {
                    rooms[i].rightRoomDrawerOpenL[s] = false;
                    if (s == 1) { rooms[i].rightRoomSlotTypeL[s] = 99; rooms[i].rightRoomSlotItemL[s] = 0; } 
                    else {
                        if (rand() % 100 < 45) {
                            int r = rand() % 100; rooms[i].rightRoomSlotTypeL[s] = (r < 25) ? 1 : ((r < 50) ? 3 : ((r < 75) ? 5 : 7));
                            rooms[i].rightRoomSlotItemL[s] = (rooms[i].rightRoomSlotTypeL[s] >= 3 && rooms[i].rightRoomSlotTypeL[s] <= 6 && rand()%100<30) ? 3 : 0;
                        } else { rooms[i].rightRoomSlotTypeL[s] = 0; rooms[i].rightRoomSlotItemL[s] = 0; }
                    }
                    
                    rooms[i].rightRoomDrawerOpenR[s] = false;
                    if (rand() % 100 < 45) {
                        int r = rand() % 100; rooms[i].rightRoomSlotTypeR[s] = (r < 25) ? 2 : ((r < 50) ? 4 : ((r < 75) ? 6 : 8));
                        rooms[i].rightRoomSlotItemR[s] = (rooms[i].rightRoomSlotTypeR[s] >= 3 && rooms[i].rightRoomSlotTypeR[s] <= 6 && rand()%100<30) ? 3 : 0;
                    } else { rooms[i].rightRoomSlotTypeR[s] = 0; rooms[i].rightRoomSlotItemR[s] = 0; }
                }
            }

            bool bandaidSpawned = false;
            for(int s=0; s<3; s++) {
                float slotZRel = -2.5f - (s * 2.5f); int r = rand() % 100;
                if (r < 25) rooms[i].slotType[s] = 1; else if (r < 50) rooms[i].slotType[s] = 2; 
                else if (r < 75) { rooms[i].slotType[s] = 5; if (!bandaidSpawned && rand() % 100 < 15) { rooms[i].slotItem[s] = 2; bandaidSpawned = true; } else if (rand() % 100 < 30) rooms[i].slotItem[s] = 3; } 
                else { rooms[i].slotType[s] = 6; if (!bandaidSpawned && rand() % 100 < 15) { rooms[i].slotItem[s] = 2; bandaidSpawned = true; } else if (rand() % 100 < 30) rooms[i].slotItem[s] = 3; }          

                if (rooms[i].hasLeftRoom && abs(rooms[i].leftDoorOffset - slotZRel) < 2.6f && (rooms[i].slotType[s] == 1 || rooms[i].slotType[s] == 3 || rooms[i].slotType[s] == 5)) { rooms[i].slotType[s] = 0; rooms[i].slotItem[s] = 0; }
                if (rooms[i].hasRightRoom && abs(rooms[i].rightDoorOffset - slotZRel) < 2.6f && (rooms[i].slotType[s] == 2 || rooms[i].slotType[s] == 4 || rooms[i].slotType[s] == 6)) { rooms[i].slotType[s] = 0; rooms[i].slotItem[s] = 0; }
            }
        } else {
            rooms[i].slotType[0] = 0; rooms[i].slotType[1] = 0; rooms[i].slotType[2] = 0;
            rooms[i].hasLeftRoom = false; rooms[i].hasRightRoom = false;
        }

        if (rooms[i].isSeekHallway || rooms[i].isSeekFinale || rooms[i].isSeekChase) {
            rooms[i].pCount = 0; 
        } else if (!isSeekEvent || rooms[i].hasSeekEyes) {
            rooms[i].pCount = rand() % 5 + 3; 
            for(int p=0; p<rooms[i].pCount; p++) {
                bool overlap; int tries = 0;
                do {
                    overlap = false; rooms[i].pSide[p] = rand() % 2; rooms[i].pZ[p] = 1.0f + (rand() % 70) / 10.0f; 
                    rooms[i].pY[p] = 0.5f + (rand() % 70) / 100.0f; rooms[i].pW[p] = 0.3f + (rand() % 60) / 100.0f; rooms[i].pH[p] = 0.3f + (rand() % 60) / 100.0f; 
                    
                    if (rooms[i].pSide[p] == 0 && rooms[i].hasLeftRoom && abs(rooms[i].pZ[p] - abs(rooms[i].leftDoorOffset)) < 2.8f) overlap = true;
                    if (rooms[i].pSide[p] == 1 && rooms[i].hasRightRoom && abs(rooms[i].pZ[p] - abs(rooms[i].rightDoorOffset)) < 2.8f) overlap = true;
                    
                    for(int op=0; op<p; op++) {
                        if (rooms[i].pSide[p] == rooms[i].pSide[op]) {
                            if (abs(rooms[i].pZ[p] - rooms[i].pZ[op]) < (rooms[i].pW[p] + rooms[i].pW[op]) * 0.6f && abs(rooms[i].pY[p] - rooms[i].pY[op]) < (rooms[i].pH[p] + rooms[i].pH[op]) * 0.6f) { overlap = true; break; }
                        }
                    } tries++;
                } while (overlap && tries < 10);
                
                if (overlap) { rooms[i].pCount--; p--; } 
                else { rooms[i].pR[p] = 0.15f + (rand() % 35) / 100.0f; rooms[i].pG[p] = 0.15f + (rand() % 35) / 100.0f; rooms[i].pB[p] = 0.15f + (rand() % 35) / 100.0f; }
            }
        } else rooms[i].pCount = 0; 
    }
    
    rooms[0].doorPos = 1; rooms[0].isDupeRoom = false; rooms[0].isLocked = true; rooms[0].isJammed = false; rooms[0].hasEyes = false;
    rooms[48].doorPos = 1; rooms[49].doorPos = 1; 
    
    for(int i=2; i<TOTAL_ROOMS - 1; i++) {
        if (!rooms[i].isDupeRoom && !rooms[i-1].isDupeRoom && !(i >= seekStartRoom && i <= seekStartRoom + 9) && !((i-1) >= seekStartRoom && (i-1) <= seekStartRoom + 9) && (rand() % 3 == 0)) {
            rooms[i].isLocked = true;
            struct SlotLoc { int type; int index; }; std::vector<SlotLoc> validSlots;
            for(int s=0; s<3; s++) if (rooms[i-1].slotType[s] > 2) validSlots.push_back({0, s});
            
            if (rooms[i-1].hasLeftRoom) {
                for(int s=0; s<3; s++) {
                    if (rooms[i-1].leftRoomSlotTypeL[s] >= 3 && rooms[i-1].leftRoomSlotTypeL[s] <= 6) validSlots.push_back({1, s}); 
                    if (rooms[i-1].leftRoomSlotTypeR[s] >= 3 && rooms[i-1].leftRoomSlotTypeR[s] <= 6) validSlots.push_back({2, s}); 
                }
            }
            if (rooms[i-1].hasRightRoom) {
                for(int s=0; s<3; s++) {
                    if (rooms[i-1].rightRoomSlotTypeL[s] >= 3 && rooms[i-1].rightRoomSlotTypeL[s] <= 6) validSlots.push_back({3, s});
                    if (rooms[i-1].rightRoomSlotTypeR[s] >= 3 && rooms[i-1].rightRoomSlotTypeR[s] <= 6) validSlots.push_back({4, s});
                }
            }

            if (!validSlots.empty()) {
                SlotLoc chosen = validSlots[rand() % validSlots.size()];
                if (chosen.type == 0) rooms[i-1].slotItem[chosen.index] = 1; 
                else if (chosen.type == 1) rooms[i-1].leftRoomSlotItemL[chosen.index] = 1; 
                else if (chosen.type == 2) rooms[i-1].leftRoomSlotItemR[chosen.index] = 1;
                else if (chosen.type == 3) rooms[i-1].rightRoomSlotItemL[chosen.index] = 1;
                else if (chosen.type == 4) rooms[i-1].rightRoomSlotItemR[chosen.index] = 1;
            } else { rooms[i-1].slotType[1] = 5; rooms[i-1].slotItem[1] = 1; }
        }
    }
}

int main() {
    gfxInitDefault(); gfxSet3D(false); irrstInit(); srand(time(NULL));
    consoleInit(GFX_BOTTOM, NULL);
    romfsInit();

    bool audio_ok = R_SUCCEEDED(ndspInit());
    ndspWaveBuf sndPsst = {0}, sndAttack = {0}, sndCaught = {0}, sndDoor = {0}, sndLockedDoor = {0}, sndDupeAttack = {0}; 
    ndspWaveBuf sndRushScream = {0}, sndEyesAppear = {0}, sndEyesGarble = {0}, sndEyesAttack = {0}, sndEyesHit = {0};
    ndspWaveBuf sndSeekRise = {0}, sndSeekChase = {0}, sndSeekEscaped = {0}, sndDeath = {0}, sndElevatorJam = {0}, sndElevatorJamEnd = {0};

    if (audio_ok) {
        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        for(int i=0; i<=10; i++) { ndspChnSetInterp(i, NDSP_INTERP_LINEAR); ndspChnSetRate(i, 44100); ndspChnSetFormat(i, NDSP_FORMAT_MONO_PCM16); }
        sndPsst = loadWav("romfs:/Screech_Psst.wav"); sndAttack = loadWav("romfs:/Screech_Attack.wav"); sndCaught = loadWav("romfs:/Screech_Caught.wav");
        sndDoor = loadWav("romfs:/Door_Open.wav"); sndLockedDoor = loadWav("romfs:/Locked_Door.wav"); sndDupeAttack = loadWav("romfs:/Dupe_Attack.wav"); 
        sndRushScream = loadWav("romfs:/Rush_Scream.wav"); sndEyesAppear = loadWav("romfs:/Eyes_Appear.wav"); sndEyesGarble = loadWav("romfs:/Eyes_Garble.wav"); sndEyesGarble.looping = true; 
        sndEyesAttack = loadWav("romfs:/Eyes_Attack.wav"); sndEyesHit = loadWav("romfs:/Eyes_Hit.wav"); sndSeekRise = loadWav("romfs:/Seek_Rise.wav"); 
        sndSeekChase = loadWav("romfs:/Seek_Chase.wav"); sndSeekChase.looping = true; sndSeekEscaped = loadWav("romfs:/Seek_Escaped.wav"); 
        sndDeath = loadWav("romfs:/Player_Death.wav"); sndElevatorJam = loadWav("romfs:/Elevator_Jam.wav"); sndElevatorJamEnd = loadWav("romfs:/Elevator_Jam_End.wav");
    }

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    generateRooms(); 

    int currentChunk = 0, playerCurrentRoom = -1;
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
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL); C3D_CullFace(GPU_CULL_NONE); 

    C3D_TexEnv* env = C3D_GetTexEnv(0); C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    float camX = 0.0f, camZ = 7.5f, camYaw = 0.0f, camPitch = 0.0f; 
    const char symbols[] = "@!$#&*%?";
    static float startTouchX = 0, startTouchY = 0; static bool wasTouching = false;

    while (aptMainLoop()) {
        hidScanInput(); irrstScanInput();
        u32 kDown = hidKeysDown(), kHeld = hidKeysHeld(); 
        
        if (kDown & KEY_SELECT) break;
        
        bool needsVBOUpdate = false; 
        static bool deathSoundPlayed = false;

        if (isDead) {
            if (!deathSoundPlayed) {
                if (audio_ok) {
                    ndspChnWaveBufClear(3); ndspChnWaveBufClear(5); ndspChnWaveBufClear(7); ndspChnWaveBufClear(9); 
                    bool waitForAttackAudio = (sndAttack.status == NDSP_WBUF_PLAYING || sndAttack.status == NDSP_WBUF_QUEUED || sndDupeAttack.status == NDSP_WBUF_PLAYING || sndDupeAttack.status == NDSP_WBUF_QUEUED);
                    if (!waitForAttackAudio) { ndspChnWaveBufClear(8); if (sndDeath.data_vaddr) { sndDeath.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(8, &sndDeath); } deathSoundPlayed = true; }
                } else deathSoundPlayed = true;
            }
        } else deathSoundPlayed = false;

        if (kDown & KEY_START) {
            if (isDead) {
                isDead = false; hasKey = false; lobbyKeyPickedUp = false; isCrouching = false; hideState = NOT_HIDING;
                playerHealth = 100; screechActive = false; flashRedFrames = 0; playerCoins = 0;
                screechCooldown = 1800; rushActive = false; rushState = 0; rushCooldown = 0; messageTimer = 0;
                inElevator = true; elevatorTimer = 1593; elevatorDoorsOpen = false; elevatorClosing = false; elevatorDoorOffset = 0.0f; elevatorJamFinished = false;
                camX = 0.0f; camZ = 7.5f; camYaw = 0.0f; camPitch = 0.0f; currentChunk = 0; playerCurrentRoom = -1;
                for(int i=0; i<TOTAL_ROOMS; i++) doorOpen[i] = false;
                seekActive = false; seekState = 0; seekTimer = 0; eyesSoundCooldown = 0;
                generateRooms(); C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                buildWorld(currentChunk, playerCurrentRoom);
                memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex)); GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
                if (audio_ok) for(int i=3; i<=10; i++) ndspChnWaveBufClear(i); 
                inEyesRoom = false; isLookingAtEyes = false; eyesDamageTimer = 0; eyesDamageAccumulator = 0; eyesGraceTimer = 0;
                consoleClear(); continue; 
            }
        }

        if ((kDown & KEY_Y) && (kHeld & KEY_R) && playerCurrentRoom == -1 && !isDead) {
            camZ = -10.0f - ((seekStartRoom - 1) * 10.0f) + 5.0f; camX = 0.0f; camYaw = 0.0f; camPitch = 0.0f; needsVBOUpdate = true;
            sprintf(uiMessage, "Teleported to Seek!"); messageTimer = 90;
        }
        
        playerCurrentRoom = (camZ > -10.0f) ? -1 : (int)((-camZ - 10.0f) / 10.0f);
        if (playerCurrentRoom < -1) playerCurrentRoom = -1; if (playerCurrentRoom > TOTAL_ROOMS - 2) playerCurrentRoom = TOTAL_ROOMS - 2;
        
        bool isGlitching = false; int targetDupeRoom = -1;
        for (int k = 1; k < TOTAL_ROOMS; k++) { if (rooms[k].isDupeRoom && playerCurrentRoom == (k - 1)) { isGlitching = true; targetDupeRoom = k; break; } }

        if (messageTimer > 0) messageTimer--; if (screechCooldown > 0) screechCooldown--; if (rushCooldown > 0) rushCooldown--; if (eyesSoundCooldown > 0) eyesSoundCooldown--;

        if (inElevator && !elevatorDoorsOpen) {
            if (elevatorTimer == 1593 && audio_ok && sndElevatorJam.data_vaddr) { ndspChnWaveBufClear(9); sndElevatorJam.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(9, &sndElevatorJam); }
            if (elevatorTimer > 0) elevatorTimer--;
            bool timeToOpen = false;
            if (audio_ok && sndElevatorJam.data_vaddr) { if (elevatorTimer < 1590 && sndElevatorJam.status == NDSP_WBUF_DONE && !elevatorJamFinished) timeToOpen = true; } 
            else if (elevatorTimer <= 0 && !elevatorJamFinished) timeToOpen = true;
            if (timeToOpen) {
                elevatorJamFinished = true; elevatorDoorsOpen = true; needsVBOUpdate = true;
                if (audio_ok) { ndspChnWaveBufClear(9); if (sndElevatorJamEnd.data_vaddr) { sndElevatorJamEnd.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(9, &sndElevatorJamEnd); } }
            }
        }
        
        if (elevatorDoorsOpen && !elevatorClosing && elevatorDoorOffset < 2.0f) { elevatorDoorOffset += 0.03f; needsVBOUpdate = true; }
        if (elevatorDoorsOpen && !elevatorClosing && camZ < 2.0f) { inElevator = false; elevatorClosing = true; needsVBOUpdate = true; }
        if (elevatorClosing && elevatorDoorOffset > 0.0f) { elevatorDoorOffset -= 0.04f; if (elevatorDoorOffset <= 0.0f) elevatorDoorOffset = 0.0f; needsVBOUpdate = true; }

        static bool prevScreechActive = false;
        if (screechActive != prevScreechActive) { consoleClear(); prevScreechActive = screechActive; }

        printf("\x1b[1;1H"); printf("==============================\n");
        if (isDead) {
            printf("         YOU DIED!            \n"); printf("==============================\n\n");
            printf("                              \n\n\n"); printf("    [PRESS START TO RESTART]  \n"); printf("\x1b[0J"); 
        } else {
            if (screechActive) {
                printf("  >> SCREECH ATTACK!! <<      \n\n"); printf("     (PSST!)                  \n"); printf("    LOOK AROUND QUICKLY!      \n\n"); printf("\x1b[0J"); 
            } else {
                printf("        PLAYER STATUS         \n"); printf("==============================\n\n");
                int dispCurrent = getDisplayRoom(playerCurrentRoom), nextDoorIdx = getNextDoorIndex(playerCurrentRoom), dispNext = getDisplayRoom(nextDoorIdx);

                if (playerCurrentRoom == -1) {
                    printf(" Current Room : 000 (Lobby) \x1b[K\n");
                    if (isGlitching) { char g2[4]; for(int i=0; i<3; i++) g2[i]=symbols[rand()%8]; g2[3]='\0'; printf(" Next Door    : %s         \x1b[K\n", g2); } 
                    else printf(" Next Door    : 001         \x1b[K\n");
                    printf(" [HOLD R + Y] Tp to Seek    \x1b[K\n\n"); 
                } else if (isGlitching) {
                    char g1[4], g2[4]; for(int i=0; i<3; i++) { g1[i]=symbols[rand()%8]; g2[i]=symbols[rand()%8]; } g1[3]='\0'; g2[3]='\0';
                    printf(" Current Room : %s         \x1b[K\n", g1); printf(" Next Door    : %s         \x1b[K\n", g2);
                    printf("                            \x1b[K\n\n"); 
                } else {
                    printf(" Current Room : %03d         \x1b[K\n", dispCurrent); printf(" Next Door    : %03d         \x1b[K\n", dispNext);
                    printf("                            \x1b[K\n\n"); 
                }

                if (nextDoorIdx >= 0 && nextDoorIdx < TOTAL_ROOMS) {
                    float nextDoorZ = -10.0f - (nextDoorIdx * 10.0f);
                    if (abs(camZ - nextDoorZ) < 4.0f && abs(camX) < 2.0f) {
                        if (isGlitching && targetDupeRoom == nextDoorIdx) {
                            if (camX < -1.4f) printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n", rooms[targetDupeRoom].dupeNumbers[0]);
                            else if (camX >= -1.4f && camX <= 0.6f) printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n", rooms[targetDupeRoom].dupeNumbers[1]);
                            else printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n", rooms[targetDupeRoom].dupeNumbers[2]);
                        } else printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n", dispNext);
                    } else printf("                           \x1b[K\n\n");
                } else printf("                           \x1b[K\n\n");

                printf(" Health       : %d / 100   \x1b[K\n", playerHealth); printf(" Golden Key   : %s         \x1b[K\n", hasKey ? "EQUIPPED" : "None    "); printf(" Coins        : %04d       \x1b[K\n", playerCoins);
                printf("\n        --- CONTROLS ---      \x1b[K\n"); printf(" [A] Interact  [B] Crouch    \x1b[K\n"); printf(" [X] Hide(Cab/Bed) [CPAD] Move \x1b[K\n"); printf(" [TOUCH/CSTICK] Look Around  \x1b[K\n");
                
                if (messageTimer > 0) printf("\n ** %s ** \x1b[K\n", uiMessage); else if (rushActive && rushState == 1) printf("\n ** The lights are flickering... ** \x1b[K\n"); else if (hideState == BEHIND_DOOR) printf("\n ** Hiding behind door... ** \x1b[K\n"); else printf("\n                                    \x1b[K\n");
                if (!audio_ok) { printf("\x1b[31m WARNING: dspfirm.cdc MISSING!\x1b[0m\x1b[K\n"); printf("\x1b[31m Sound chip could not turn on.\x1b[0m\x1b[K\n"); } else printf("                                    \x1b[K\n");
                printf("\x1b[0J"); 
            }
        }

        if (!isDead) {
            if (playerCurrentRoom >= seekStartRoom && playerCurrentRoom <= seekStartRoom + 2) {
                float hallwayEndZ = -10.0f - ((seekStartRoom + 2) * 10.0f) - 8.0f; 
                if (camZ < hallwayEndZ && seekState == 0) {
                    seekState = 1; seekActive = true; seekTimer = 0; seekSpeed = 0.0f; seekZ = -10.0f - (seekStartRoom * 10.0f); needsVBOUpdate = true;
                    if (audio_ok && sndSeekRise.data_vaddr) { ndspChnWaveBufClear(7); sndSeekRise.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(7, &sndSeekRise); }
                }
            }

            if (seekState == 1) {
                seekTimer++; needsVBOUpdate = true; 
                if (seekTimer >= 180 && seekTimer < 230) { if (seekSpeed < 0.12f) seekSpeed += 0.005f; seekZ -= seekSpeed; }
                if (seekTimer >= 230) { 
                    seekState = 2; seekSpeed = 0.0f; sprintf(uiMessage, "RUN!"); messageTimer = 90; flashRedFrames = 10; 
                    if (audio_ok && sndSeekChase.data_vaddr) { ndspChnWaveBufClear(7); sndSeekChase.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(7, &sndSeekChase); }
                }
            } else if (seekState == 2) {
                float firstDoorZ = -10.0f - ((seekStartRoom + 2) * 10.0f); 
                if (seekZ > firstDoorZ) seekSpeed = 0.065f; else seekSpeed = seekMaxSpeed; 
                seekZ -= seekSpeed; needsVBOUpdate = true;
                
                if (abs(seekZ - camZ) < 1.2f) {
                    playerHealth = 0; isDead = true; flashRedFrames = 50; sprintf(uiMessage, "Seek caught you..."); messageTimer = 120;
                    if (audio_ok) ndspChnWaveBufClear(7); 
                }
                
                if (playerCurrentRoom >= 0 && rooms[playerCurrentRoom].isSeekFinale) {
                    for(int h = 0; h < 6; h++) {
                        float hX = rooms[playerCurrentRoom].pW[h], hZ = rooms[playerCurrentRoom].pZ[h]; int type = rooms[playerCurrentRoom].pSide[h];
                        if (abs(camX - hX) < 0.6f && abs(camZ - hZ) < 0.6f) {
                            if (type == 0 && !isDead && messageTimer <= 0) {
                                playerHealth -= 40; flashRedFrames = 25; camZ += 1.5f; sprintf(uiMessage, "Burned! (-40 HP)"); messageTimer = 60;
                                if(playerHealth <= 0) { isDead = true; if (audio_ok) ndspChnWaveBufClear(7); }
                            } else if (type == 1 && !isDead && !isCrouching) { 
                                playerHealth = 0; isDead = true; flashRedFrames = 50; sprintf(uiMessage, "The hands grabbed you!"); messageTimer = 120;
                                if (audio_ok) ndspChnWaveBufClear(7); 
                            }
                        }
                    }
                }
                
                if (seekActive) {
                    float finishLineZ = -10.0f - ((seekStartRoom + 8) * 10.0f) - 10.0f; int safeRoom = seekStartRoom + 9;
                    bool playerSafe = (camZ < finishLineZ - 1.5f);
                    
                    if (playerSafe || seekZ < finishLineZ + 3.0f) {
                        if (!rooms[safeRoom].isJammed) {
                            doorOpen[safeRoom] = false; rooms[safeRoom].isLocked = false; rooms[safeRoom].isJammed = true; needsVBOUpdate = true;
                            if (audio_ok) {
                                if (sndDoor.data_vaddr) { ndspChnWaveBufClear(1); sndDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sndDoor); }
                                ndspChnWaveBufClear(7); if (sndSeekEscaped.data_vaddr) { sndSeekEscaped.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(7, &sndSeekEscaped); }
                            }
                        }
                        if (playerSafe) { seekActive = false; seekState = 0; sprintf(uiMessage, "You escaped!"); messageTimer = 150; } 
                        else { playerHealth = 0; isDead = true; flashRedFrames = 50; sprintf(uiMessage, "The door slammed shut!"); messageTimer = 120; seekActive = false; seekState = 0; if (audio_ok) ndspChnWaveBufClear(7); }
                    }
                }
            }

            bool currentlyInEyesRoom = (playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS && rooms[playerCurrentRoom].hasEyes);
            if (currentlyInEyesRoom && !inEyesRoom) {
                inEyesRoom = true; eyesGraceTimer = 60; 
                if (audio_ok) {
                    ndspChnWaveBufClear(4); if (sndEyesAppear.data_vaddr) { sndEyesAppear.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(4, &sndEyesAppear); }
                    ndspChnWaveBufClear(5); if (sndEyesGarble.data_vaddr) { float garbleMix[12] = {0}; garbleMix[0] = 1.8f; garbleMix[1] = 1.8f; ndspChnSetMix(5, garbleMix); sndEyesGarble.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(5, &sndEyesGarble); }
                }
            } else if (!currentlyInEyesRoom && inEyesRoom) { inEyesRoom = false; if (audio_ok) ndspChnWaveBufClear(5); }
            if (currentlyInEyesRoom) if (eyesGraceTimer > 0) eyesGraceTimer--;

            if (currentlyInEyesRoom && hideState == NOT_HIDING) {
                float ex = rooms[playerCurrentRoom].eyesX, ey = rooms[playerCurrentRoom].eyesY, ez = rooms[playerCurrentRoom].eyesZ, camYOffset = isCrouching ? 0.4f : 0.9f; 
                float vx = ex - camX, vy = ey - camYOffset, vz = ez - camZ, dist = sqrt(vx*vx + vy*vy + vz*vz); if (dist > 0) { vx /= dist; vy /= dist; vz /= dist; }
                float fx = -sinf(camYaw) * cosf(camPitch), fy = sinf(camPitch), fz = -cosf(camYaw) * cosf(camPitch), dotProduct = (fx * vx) + (fy * vy) + (fz * vz);

                // Check dot product AND Line of Sight!
                if (dotProduct > 0.85f && checkLineOfSight(camX, camYOffset, camZ, ex, ey, ez)) { 
                    if (eyesGraceTimer <= 0) {
                        if (!isLookingAtEyes) {
                            isLookingAtEyes = true; eyesDamageTimer = 5; eyesDamageAccumulator = 4; 
                            if (audio_ok && sndEyesAttack.data_vaddr && eyesSoundCooldown <= 0) { ndspChnWaveBufClear(4); sndEyesAttack.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(4, &sndEyesAttack); eyesSoundCooldown = 90; }
                        }
                        if (audio_ok && sndEyesAttack.data_vaddr) {
                            if (sndEyesAttack.status == NDSP_WBUF_DONE || sndEyesAttack.status == NDSP_WBUF_FREE) {
                                if (eyesSoundCooldown <= 0) { ndspChnWaveBufClear(4); sndEyesAttack.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(4, &sndEyesAttack); eyesSoundCooldown = 90; }
                            }
                        }
                        eyesDamageTimer++;
                        if (eyesDamageTimer >= 6) { 
                            playerHealth -= 1; eyesDamageTimer = 0; flashRedFrames = 2; eyesDamageAccumulator++;
                            if (eyesDamageAccumulator >= 5) { eyesDamageAccumulator = 0; if (audio_ok && sndEyesHit.data_vaddr) { ndspChnWaveBufClear(6); sndEyesHit.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(6, &sndEyesHit); } }
                        }
                        if (playerHealth <= 0) { isDead = true; sprintf(uiMessage, "You stared at Eyes!"); messageTimer = 120; }
                    }
                } else { isLookingAtEyes = false; eyesDamageTimer = 0; eyesDamageAccumulator = 0; }
            } else { isLookingAtEyes = false; eyesDamageTimer = 0; eyesDamageAccumulator = 0; }

            bool inSeekEvent = (playerCurrentRoom >= seekStartRoom - 5 && playerCurrentRoom <= seekStartRoom + 9);
            int screechChance = (playerCurrentRoom > 0 && rooms[playerCurrentRoom].lightLevel < 0.5f) ? 400 : 12000;
            if (!screechActive && screechCooldown <= 0 && hideState == NOT_HIDING && playerCurrentRoom > 0 && !inSeekEvent && (rand() % screechChance == 0)) {
                screechActive = true; screechTimer = 240; 
                float angleOffset = 1.57f + ((rand() % 200) / 100.0f) * 1.57f, spawnYaw = camYaw + angleOffset;
                screechOffsetX = -sinf(spawnYaw) * 1.5f; screechOffsetZ = -cosf(spawnYaw) * 1.5f; screechOffsetY = (rand() % 15) / 10.0f; 
                if (audio_ok) { ndspChnWaveBufClear(0); if (sndPsst.data_vaddr) { sndPsst.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(0, &sndPsst); } }
            }

            if (screechActive) {
                screechTimer--; float newScreechX = camX + screechOffsetX, newScreechZ = camZ + screechOffsetZ;
                if (screechX != newScreechX || screechZ != newScreechZ) { screechX = newScreechX; screechZ = newScreechZ; screechY = 0.8f + screechOffsetY; needsVBOUpdate = true; }
                float vx = screechX - camX, vy = screechY - (isCrouching ? 0.4f : 0.9f), vz = screechZ - camZ, dist = sqrt(vx*vx + vy*vy + vz*vz); if (dist > 0.0f) { vx /= dist; vy /= dist; vz /= dist; }
                float fx = -sinf(camYaw) * cosf(camPitch), fy = sinf(camPitch), fz = -cosf(camYaw) * cosf(camPitch), dotProduct = (fx * vx) + (fy * vy) + (fz * vz);

                if (dotProduct > 0.85f) { 
                    screechActive = false; screechCooldown = 1800; needsVBOUpdate = true; sprintf(uiMessage, "Dodged Screech!"); messageTimer = 90;
                    if (audio_ok) { ndspChnWaveBufClear(0); if (sndCaught.data_vaddr) { sndCaught.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(0, &sndCaught); } }
                } else if (screechTimer <= 0) {
                    screechActive = false; screechCooldown = 1800; needsVBOUpdate = true; playerHealth -= 20; flashRedFrames = 25; 
                    sprintf(uiMessage, "Screech bit you! (-20 HP)"); messageTimer = 90; if (playerHealth <= 0) isDead = true;
                    if (audio_ok) { ndspChnWaveBufClear(0); if (sndAttack.data_vaddr) { sndAttack.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(0, &sndAttack); } }
                }
            }

            if (rushActive) {
                if (rushState == 1) { 
                    rushTimer--;
                    if (rushTimer % 10 == 0) { float flicker = (rand() % 2 == 0) ? 0.3f : 1.0f; if (playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) rooms[playerCurrentRoom].lightLevel = flicker; needsVBOUpdate = true; }
                    if (rushTimer <= 0) { rushState = 2; rushZ = camZ + 40.0f; rushTargetZ = camZ - 60.0f; }
                } else if (rushState == 2) { 
                    rushZ -= 0.8f; needsVBOUpdate = true; 
                    if (playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) { rooms[playerCurrentRoom].lightLevel = 0.3f; if (playerCurrentRoom + 1 < TOTAL_ROOMS) rooms[playerCurrentRoom + 1].lightLevel = 0.3f; }
                    if (abs(rushZ - camZ) < 3.0f && abs(camX) < 3.0f && hideState == NOT_HIDING) { playerHealth = 0; isDead = true; flashRedFrames = 50; }
                    if (rushZ < rushTargetZ) { rushActive = false; rushState = 0; needsVBOUpdate = true; rushCooldown = 1800; if (audio_ok) ndspChnWaveBufClear(3); }
                }
                if (audio_ok && sndRushScream.data_vaddr) {
                    float dist = 0.0f; if (rushState == 1) dist = 40.0f + (rushTimer / rushStartTimer) * 110.0f; else if (rushState == 2) { dist = abs(rushZ - camZ); if (rushZ < camZ) dist *= 1.5f; }
                    float maxDist = 150.0f; float vol = 1.0f - (dist / maxDist); if (vol < 0.0f) vol = 0.0f; if (vol > 1.0f) vol = 1.0f; vol = vol * vol * vol; 
                    float mix[12] = {0}; mix[0] = vol * 3.5f; mix[1] = vol * 3.5f; ndspChnSetMix(3, mix);
                }
            }

            if (playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) {
                if (rooms[playerCurrentRoom].hasLeftRoom && !rooms[playerCurrentRoom].leftDoorOpen) {
                    float doorZ = (-10.0f - (playerCurrentRoom * 10.0f)) + rooms[playerCurrentRoom].leftDoorOffset;
                    if (abs(camZ - (doorZ - 0.6f)) < 2.0f && camX < -1.5f) { rooms[playerCurrentRoom].leftDoorOpen = true; needsVBOUpdate = true; if (audio_ok && sndDoor.data_vaddr) { ndspChnWaveBufClear(1); sndDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sndDoor); } }
                }
                if (rooms[playerCurrentRoom].hasRightRoom && !rooms[playerCurrentRoom].rightDoorOpen) {
                    float doorZ = (-10.0f - (playerCurrentRoom * 10.0f)) + rooms[playerCurrentRoom].rightDoorOffset;
                    if (abs(camZ - (doorZ - 0.6f)) < 2.0f && camX > 1.5f) { rooms[playerCurrentRoom].rightDoorOpen = true; needsVBOUpdate = true; if (audio_ok && sndDoor.data_vaddr) { ndspChnWaveBufClear(1); sndDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sndDoor); } }
                }
            }

            if (kDown & KEY_X && hideState == NOT_HIDING) {
                float reach = 0.5f; 
                for(auto& b : collisions) {
                    if ((b.type == 1 || b.type == 2) && camX + reach > b.minX && camX - reach < b.maxX && camZ + reach > b.minZ && camZ - reach < b.maxZ) {
                        float relCenterZ = (b.minZ + b.maxZ) / 2.0f, cabCenterRelX = ((b.minX + b.maxX) / 2.0f), targetOffsetX = 0.0f;
                        if (cabCenterRelX < -4.0f) targetOffsetX = -6.0f; else if (cabCenterRelX > 4.0f) targetOffsetX = 6.0f;
                        if (b.type == 1) { hideState = IN_CABINET; camZ = relCenterZ; camPitch = 0.0f; if ((cabCenterRelX - targetOffsetX) < 0) { camX = -2.5f + targetOffsetX; camYaw = -1.57f; } else { camX = 2.5f + targetOffsetX; camYaw = 1.57f; } } 
                        else { hideState = UNDER_BED; camZ = relCenterZ; camPitch = 0.0f; if ((cabCenterRelX - targetOffsetX) < 0) { camX = -2.2f + targetOffsetX; camYaw = -1.57f; } else { camX = 2.2f + targetOffsetX; camYaw = 1.57f; } }
                        isCrouching = false; needsVBOUpdate = true; break; 
                    }
                }
            } else if (kDown & KEY_X) {
                if (hideState == IN_CABINET || hideState == UNDER_BED) { camX = (camX < -4.0f) ? -6.0f : ((camX > 4.0f) ? 6.0f : 0.0f); hideState = NOT_HIDING; camYaw = 0.0f; needsVBOUpdate = true; }
            }

            if ((kDown & KEY_A) || (kDown & KEY_X && hideState == NOT_HIDING)) {
                auto checkInteract = [&](int type, float zCenter, float slotX, bool& isOpen, int& item) {
                    if (type != 0 && abs(camZ - zCenter) < 1.5f && abs(camX - slotX) < 1.8f) {
                        if (kDown & KEY_X && (type == 5 || type == 6)) { isOpen = !isOpen; needsVBOUpdate = true; return true; }
                        if (kDown & KEY_A) {
                            if ((type == 5 || type == 6) && isOpen) {
                                if (item == 1) { hasKey = true; item = 0; needsVBOUpdate = true; sprintf(uiMessage, "Grabbed the Golden Key!"); messageTimer = 90; return true; }
                                else if (item == 2) { playerHealth += 10; if (playerHealth > 100) playerHealth = 100; item = 0; needsVBOUpdate = true; sprintf(uiMessage, "Used a Bandaid! (+10 HP)"); messageTimer = 90; return true; }
                                else if (item == 3) { playerCoins += (rand() % 15) + 5; item = 0; needsVBOUpdate = true; sprintf(uiMessage, "Looted some Coins!"); messageTimer = 90; return true; }
                                else { sprintf(uiMessage, "Drawer is empty..."); messageTimer = 60; return true; }
                            } else if ((type == 3 || type == 4)) {
                                if (item == 1) { hasKey = true; item = 0; needsVBOUpdate = true; sprintf(uiMessage, "Grabbed Key off the bed!"); messageTimer = 90; return true; }
                                else if (item == 3) { playerCoins += (rand() % 15) + 5; item = 0; needsVBOUpdate = true; sprintf(uiMessage, "Looted Coins off the bed!"); messageTimer = 90; return true; }
                            }
                        }
                    } return false;
                };

                auto checkChest = [&](float cX, float cZ, bool& isOpen) {
                    if (!isOpen && abs(camZ - cZ) < 2.0f && abs(camX - cX) < 2.0f) {
                        isOpen = true; needsVBOUpdate = true; playerCoins += 20;
                        sprintf(uiMessage, "Opened Chest!"); messageTimer = 90; return true;
                    } return false;
                };

                bool interacted = false; bool dummyBool = false;

                if (inElevator && !elevatorDoorsOpen) {
                    if (camX > 0.0f && camZ > 5.0f && camZ < 8.0f) { 
                        elevatorJamFinished = true; elevatorDoorsOpen = true; interacted = true; needsVBOUpdate = true;
                        if (audio_ok) { ndspChnWaveBufClear(9); if (sndElevatorJamEnd.data_vaddr) { sndElevatorJamEnd.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(9, &sndElevatorJamEnd); } }
                    }
                }

                if (!interacted && playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) {
                    for(int s=0; s<3; s++) if (checkInteract(rooms[playerCurrentRoom].slotType[s], (-10.0f - (playerCurrentRoom * 10.0f)) - 2.5f - (s * 2.5f), ((rooms[playerCurrentRoom].slotType[s] % 2 != 0) ? -2.4f : 2.4f), rooms[playerCurrentRoom].drawerOpen[s], rooms[playerCurrentRoom].slotItem[s])) { interacted = true; break; }
                    
                    if (!interacted && rooms[playerCurrentRoom].hasLeftRoom) {
                        float srZ = (-10.0f - (playerCurrentRoom * 10.0f)) + rooms[playerCurrentRoom].leftDoorOffset + 2.5f; 
                        for (int s=0; s<3; s++) {
                            float fZ = srZ - 0.9f - (s * 1.6f);
                            int typeL = rooms[playerCurrentRoom].leftRoomSlotTypeL[s];
                            if (typeL > 0 && typeL != 99) {
                                if (typeL == 7 || typeL == 8) { if (checkChest(-8.4f, fZ, rooms[playerCurrentRoom].leftRoomDrawerOpenL[s])) { interacted = true; break; } }
                                else if (checkInteract(typeL, fZ, -8.4f, rooms[playerCurrentRoom].leftRoomDrawerOpenL[s], rooms[playerCurrentRoom].leftRoomSlotItemL[s])) { interacted = true; break; }
                            }
                            int typeR = rooms[playerCurrentRoom].leftRoomSlotTypeR[s];
                            if (typeR > 0 && typeR != 99) {
                                if (typeR == 7 || typeR == 8) { if (checkChest(-3.6f, fZ, rooms[playerCurrentRoom].leftRoomDrawerOpenR[s])) { interacted = true; break; } }
                                else if (checkInteract(typeR, fZ, -3.6f, rooms[playerCurrentRoom].leftRoomDrawerOpenR[s], rooms[playerCurrentRoom].leftRoomSlotItemR[s])) { interacted = true; break; }
                            }
                        }
                    }

                    if (!interacted && rooms[playerCurrentRoom].hasRightRoom) {
                        float srZ = (-10.0f - (playerCurrentRoom * 10.0f)) + rooms[playerCurrentRoom].rightDoorOffset + 2.5f; 
                        for (int s=0; s<3; s++) {
                            float fZ = srZ - 0.9f - (s * 1.6f);
                            int typeL = rooms[playerCurrentRoom].rightRoomSlotTypeL[s];
                            if (typeL > 0 && typeL != 99) {
                                if (typeL == 7 || typeL == 8) { if (checkChest(3.6f, fZ, rooms[playerCurrentRoom].rightRoomDrawerOpenL[s])) { interacted = true; break; } }
                                else if (checkInteract(typeL, fZ, 3.6f, rooms[playerCurrentRoom].rightRoomDrawerOpenL[s], rooms[playerCurrentRoom].rightRoomSlotItemL[s])) { interacted = true; break; }
                            }
                            int typeR = rooms[playerCurrentRoom].rightRoomSlotTypeR[s];
                            if (typeR > 0 && typeR != 99) {
                                if (typeR == 7 || typeR == 8) { if (checkChest(8.4f, fZ, rooms[playerCurrentRoom].rightRoomDrawerOpenR[s])) { interacted = true; break; } }
                                else if (checkInteract(typeR, fZ, 8.4f, rooms[playerCurrentRoom].rightRoomDrawerOpenR[s], rooms[playerCurrentRoom].rightRoomSlotItemR[s])) { interacted = true; break; }
                            }
                        }
                    }
                }

                if(!interacted && !lobbyKeyPickedUp && rooms[0].isLocked && camX < -3.5f && camZ < -8.5f) {
                    lobbyKeyPickedUp = true; hasKey = true; needsVBOUpdate = true; sprintf(uiMessage, "Found the Lobby Key!"); messageTimer = 90; interacted = true;
                }

                if (!interacted) {
                    for(int i=0; i<TOTAL_ROOMS; i++) {
                        if (i == seekStartRoom + 1 || i == seekStartRoom + 2) continue; 
                        if (rooms[i].isLocked || rooms[i].isJammed) {
                            float doorZ = -10.0f - (i * 10.0f), doorX = (rooms[i].doorPos == 0) ? -2.0f : ((rooms[i].doorPos == 1) ? 0.0f : 2.0f);
                            if (abs(camZ - doorZ) < 2.5f && abs(camX - doorX) < 2.0f) {
                                if (rooms[i].isJammed) { if (audio_ok && sndLockedDoor.data_vaddr) { ndspChnWaveBufClear(1); sndLockedDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sndLockedDoor); } sprintf(uiMessage, "The door is jammed shut!"); messageTimer = 60; } 
                                else if (hasKey) { rooms[i].isLocked = false; hasKey = false; needsVBOUpdate = true; sprintf(uiMessage, "Door Unlocked!"); messageTimer = 60; } 
                                else { if (audio_ok && sndLockedDoor.data_vaddr) { ndspChnWaveBufClear(1); sndLockedDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sndLockedDoor); } sprintf(uiMessage, "It's locked..."); messageTimer = 60; }
                                interacted = true; break; 
                            }
                        }
                    }
                }

                if (!interacted) {
                    int nextRoomIdx = getNextDoorIndex(playerCurrentRoom);
                    if (nextRoomIdx >= 0 && nextRoomIdx < TOTAL_ROOMS) {
                        int interactRoom = rooms[nextRoomIdx].isDupeRoom ? nextRoomIdx : -1;
                        if (interactRoom != -1) {
                            float lockDoorZ = -10.0f - (nextRoomIdx * 10.0f);
                            if (abs(camZ - lockDoorZ) < 1.8f) {
                                bool leftT = (camX < -1.4f), centerT = (camX >= -1.4f && camX <= 0.6f), rightT = (camX > 0.6f);
                                int correctPos = rooms[interactRoom].correctDupePos;
                                if ((leftT && correctPos == 0) || (centerT && correctPos == 1) || (rightT && correctPos == 2)) {
                                    if (!doorOpen[interactRoom]) { if (audio_ok && sndDoor.data_vaddr) { ndspChnWaveBufClear(1); sndDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sndDoor); } doorOpen[interactRoom] = true; needsVBOUpdate = true; }
                                } else if (leftT || centerT || rightT) {
                                    if (audio_ok && sndDupeAttack.data_vaddr) { ndspChnWaveBufClear(2); sndDupeAttack.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(2, &sndDupeAttack); }
                                    playerHealth -= 34; flashRedFrames = 25; camZ += 2.0f; if (playerHealth <= 0) isDead = true; 
                                }
                            }
                        }
                    }
                }
            }

            if ((kDown & KEY_B) && hideState == NOT_HIDING) { if (isCrouching) { if (!checkCollision(camX, 0.0f, camZ, 1.1f)) isCrouching = false; } else isCrouching = true; }

            int newChunk = 0; if (camZ < -10.0f) newChunk = (int)((abs(camZ) - 10.0f) / 10.0f) + 1;
            
            if (newChunk != currentChunk || needsVBOUpdate) { 
                if (newChunk != currentChunk && playerCurrentRoom > 1 && !rushActive && rushCooldown <= 0 && !inSeekEvent && rand() % 100 < 12) {
                    rushActive = true; rushState = 1; rushTimer = 300 + (rand() % 120); rushStartTimer = (float)rushTimer; 
                    if (audio_ok && sndRushScream.data_vaddr) { float mix[12] = {0}; ndspChnSetMix(3, mix); ndspChnWaveBufClear(3); sndRushScream.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(3, &sndRushScream); }
                }
                currentChunk = newChunk; needsVBOUpdate = true; 
            }

            int startRoom = currentChunk - 1, endRoom = currentChunk + 2;
            if (currentChunk >= seekStartRoom && currentChunk <= seekStartRoom + 2) { startRoom = seekStartRoom; endRoom = seekStartRoom + 3; }
            if (startRoom < 0) startRoom = 0; if (endRoom > TOTAL_ROOMS - 1) endRoom = TOTAL_ROOMS - 1;

            for(int i = startRoom; i <= endRoom; i++) {
                if (rooms[i].isDupeRoom || i == seekStartRoom + 1 || i == seekStartRoom + 2) continue; 
                float wallZ = -10.0f - (i * 10.0f), targetX = (rooms[i].doorPos == 0) ? -2.0f : ((rooms[i].doorPos == 1) ? 0.0f : 2.0f);
                bool shouldBeOpen = (abs(camZ - wallZ) < 1.5f && abs(camX - targetX) < 1.5f);
                if (rooms[i].isLocked || rooms[i].isJammed) shouldBeOpen = false; 
                if (doorOpen[i] != shouldBeOpen) {
                    if (shouldBeOpen && audio_ok && sndDoor.data_vaddr) { ndspChnWaveBufClear(1); sndDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sndDoor); }
                    doorOpen[i] = shouldBeOpen; needsVBOUpdate = true;
                }
            }

            if (needsVBOUpdate) { buildWorld(currentChunk, playerCurrentRoom); memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex)); GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex)); }

            float playerH = isCrouching ? 0.5f : 1.1f; 
            circlePosition cStick, cPad; irrstCstickRead(&cStick); hidCircleRead(&cPad); touchPosition touch; hidTouchRead(&touch);
            
            if ((hideState == NOT_HIDING || hideState == BEHIND_DOOR) && seekState != 1) {
                if (abs(cStick.dx) > 10) camYaw -= cStick.dx / 1560.0f * 0.8f;
                if (abs(cStick.dy) > 10) camPitch += cStick.dy / 1560.0f * 0.8f;
                if (kHeld & KEY_TOUCH) {
                    if (!wasTouching) { startTouchX = touch.px; startTouchY = touch.py; wasTouching = true; } 
                    else { float dx = (float)touch.px - startTouchX, dy = (float)touch.py - startTouchY; if (abs(dx) < 10.0f) dx = 0.0f; if (abs(dy) < 10.0f) dy = 0.0f; camYaw -= (dx / 160.0f) * 0.06f; camPitch -= (dy / 120.0f) * 0.06f; }
                } else wasTouching = false; 
                
                if (camPitch > 1.57f) camPitch = 1.57f; if (camPitch < -1.57f) camPitch = -1.57f;
                
                if (abs(cPad.dy) > 15 || abs(cPad.dx) > 15) {
                    float s = isCrouching ? 0.16f : 0.28f; if (seekState == 2) s = isCrouching ? 0.25f : 0.42f; 
                    float sy = cPad.dy/1560.0f, sx = cPad.dx/1560.0f, nextX = camX - (sinf(camYaw) * sy - cosf(camYaw) * sx) * s, nextZ = camZ - (cosf(camYaw) * sy + sinf(camYaw) * sx) * s;
                    if(!checkCollision(nextX, 0.0f, camZ, playerH)) camX = nextX; if(!checkCollision(camX, 0.0f, nextZ, playerH)) camZ = nextZ;
                }
            }

            if (hideState == NOT_HIDING || hideState == BEHIND_DOOR) {
                bool inDoorZone = false;
                for(auto& b : collisions) if (b.type == 4 && camX > b.minX && camX < b.maxX && camZ > b.minZ && camZ < b.maxZ) { inDoorZone = true; break; }
                if (inDoorZone && hideState == NOT_HIDING) hideState = BEHIND_DOOR; else if (!inDoorZone && hideState == BEHIND_DOOR) hideState = NOT_HIDING;
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW); C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); C3D_FrameDrawOn(target);

        if (flashRedFrames > 0 && !isDead) { C3D_TexEnvColor(env, 0xFF0000FF); C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT); flashRedFrames--; } 
        else if (!isDead) C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);

        float drawCamX = camX, drawCamZ = camZ, drawCamYaw = camYaw, drawCamPitch = camPitch; static float lockedCutsceneCamZ = 0.0f;
        if (seekState == 1) {
            if (seekTimer == 1) lockedCutsceneCamZ = seekZ - 4.0f; 
            if (seekTimer <= 90) { float t = seekTimer / 90.0f; t = t * t * (3.0f - 2.0f * t); drawCamZ = camZ + (lockedCutsceneCamZ - camZ) * t; drawCamYaw = camYaw + (3.14159f - camYaw) * t; } 
            else { drawCamZ = lockedCutsceneCamZ; drawCamYaw = 3.14159f; if (seekTimer > 180) drawCamPitch = sinf(seekTimer * 0.8f) * 0.03f; }
        }

        C3D_Mtx proj, view; Mtx_PerspTilt(&proj, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        Mtx_Identity(&view); Mtx_RotateX(&view, -drawCamPitch, true); Mtx_RotateY(&view, -drawCamYaw, true);
        Mtx_Translate(&view, -drawCamX, isDead ? -0.1f : (isCrouching ? -0.4f : (hideState==NOT_HIDING || hideState==BEHIND_DOOR ? -0.9f : (hideState==IN_CABINET?-0.7f:-0.15f))), -drawCamZ, true); 
        Mtx_Multiply(&view, &proj, &view);
        
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_proj, &view); C3D_DrawArrays(GPU_TRIANGLES, 0, world_mesh.size());
        C3D_FrameEnd(0);
    }
    
    if (audio_ok) {
        if (sndPsst.data_vaddr) linearFree((void*)sndPsst.data_vaddr); if (sndAttack.data_vaddr) linearFree((void*)sndAttack.data_vaddr); if (sndCaught.data_vaddr) linearFree((void*)sndCaught.data_vaddr);
        if (sndDoor.data_vaddr) linearFree((void*)sndDoor.data_vaddr); if (sndLockedDoor.data_vaddr) linearFree((void*)sndLockedDoor.data_vaddr); if (sndDupeAttack.data_vaddr) linearFree((void*)sndDupeAttack.data_vaddr);
        if (sndRushScream.data_vaddr) linearFree((void*)sndRushScream.data_vaddr); if (sndEyesAppear.data_vaddr) linearFree((void*)sndEyesAppear.data_vaddr); if (sndEyesGarble.data_vaddr) linearFree((void*)sndEyesGarble.data_vaddr); 
        if (sndEyesAttack.data_vaddr) linearFree((void*)sndEyesAttack.data_vaddr); if (sndEyesHit.data_vaddr) linearFree((void*)sndEyesHit.data_vaddr); if (sndSeekRise.data_vaddr) linearFree((void*)sndSeekRise.data_vaddr); 
        if (sndSeekChase.data_vaddr) linearFree((void*)sndSeekChase.data_vaddr); if (sndSeekEscaped.data_vaddr) linearFree((void*)sndSeekEscaped.data_vaddr); if (sndDeath.data_vaddr) linearFree((void*)sndDeath.data_vaddr); 
        if (sndElevatorJam.data_vaddr) linearFree((void*)sndElevatorJam.data_vaddr); if (sndElevatorJamEnd.data_vaddr) linearFree((void*)sndElevatorJamEnd.data_vaddr); ndspExit();
    }
    
    romfsExit(); C3D_Fini(); gfxExit(); return 0;
}
