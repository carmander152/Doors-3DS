#pragma once
#include <3ds.h>
#include <citro3d.h>
#include <vector>

#define DISPLAY_TRANSFER_FLAGS (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define MAX_VERTS 180000 
#define MAX_ENTITY_VERTS 5000
#define TOTAL_ROOMS 102 

typedef struct { float pos[4]; float texcoord[2]; float clr[4]; } vertex;
typedef struct { float minX, minY, minZ, maxX, maxY, maxZ; int type; } BBox;
typedef enum { NOT_HIDING, IN_CABINET, UNDER_BED, BEHIND_DOOR } HideState;

extern std::vector<vertex> world_mesh_colored;
extern std::vector<vertex> world_mesh_textured; 
extern std::vector<vertex> entity_mesh_colored;
extern std::vector<vertex> entity_mesh_textured;
extern std::vector<BBox> collisions;

extern float globalTintR, globalTintG, globalTintB;
extern C3D_Tex atlasTex;
extern bool hasAtlas; 
extern bool isBuildingEntities;
extern char texErrorMessage[100];

struct RoomSetup {
    int slotType[3], slotItem[3], doorPos, pCount, seekEyeCount;
    bool drawerOpen[3], isLocked, isJammed, hasLeftRoom, leftDoorOpen, hasRightRoom, rightDoorOpen, isDupeRoom, hasEyes, hasSeekEyes, isSeekHallway, isSeekChase, isSeekFinale;
    float animMain[3], animLL[3], animLR[3], animRL[3], animRL3[3], animRR[3]; 
    float lightLevel, leftDoorOffset, rightDoorOffset, eyesX, eyesY, eyesZ; 
    float pZ[10], pY[10], pW[10], pH[10], pR[10], pG[10], pB[10]; int pSide[10];   
    int leftRoomSlotTypeL[3], leftRoomSlotItemL[3], leftRoomSlotTypeR[3], leftRoomSlotItemR[3]; bool leftRoomDrawerOpenL[3], leftRoomDrawerOpenR[3];
    int rightRoomSlotTypeL[3], rightRoomSlotItemL[3], rightRoomSlotTypeR[3], rightRoomSlotItemR[3]; bool rightRoomDrawerOpenL[3], rightRoomDrawerOpenR[3];
    int correctDupePos, dupeNumbers[3]; 
};

extern RoomSetup rooms[TOTAL_ROOMS];

extern int playerHealth, flashRedFrames, seekStartRoom, playerCoins, elevatorTimer, messageTimer;
extern bool hasKey, lobbyKeyPickedUp, isCrouching, isDead, inElevator, elevatorDoorsOpen, elevatorClosing, elevatorJamFinished;
extern bool doorOpen[TOTAL_ROOMS]; 
extern HideState hideState; 
extern float camX, camZ, camYaw, camPitch; 
extern float elevatorDoorOffset; 
extern char uiMessage[50];

extern bool screechActive, rushActive, seekActive, inEyesRoom, isLookingAtEyes;
extern int screechState, screechTimer, screechCooldown, rushState, rushTimer, rushCooldown, seekState, seekTimer, eyesDamageTimer, eyesDamageAccumulator, eyesGraceTimer, eyesSoundCooldown;

extern float screechX, screechY, screechZ, screechOffsetX, screechOffsetY, screechOffsetZ, rushStartTimer, rushZ, rushTargetZ, seekZ, seekSpeed, seekMaxSpeed; 

extern bool figureActive, figureState; 
extern float figureX, figureY, figureZ, figureSpeed; 
extern int figureWaypoint;

// --- NEW FIGURE VARIABLES ---
extern int figureTargetWP;
extern float figureTargetX;
extern float figureTargetZ;
// ----------------------------

int getDisplayRoom(int idx);
int getNextDoorIndex(int currentIdx);
