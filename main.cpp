#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h> 
#include <math.h> 
#include <vector>
#include <time.h>
#include "vshader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
#define MAX_VERTS 120000 
#define TOTAL_ROOMS 102 

typedef struct { float pos[4]; float clr[4]; } vertex;
typedef struct { float minX, minY, minZ, maxX, maxY, maxZ; int type; } BBox;
typedef enum { NOT_HIDING, IN_CABINET, UNDER_BED, BEHIND_DOOR } HideState;

std::vector<vertex> world_mesh; std::vector<BBox> collisions;
float globalTintR = 1.0f, globalTintG = 1.0f, globalTintB = 1.0f;

struct RoomSetup {
    int slotType[3], slotItem[3], doorPos, pCount, seekEyeCount;
    bool drawerOpen[3], isLocked, isJammed, hasLeftRoom, leftDoorOpen, hasRightRoom, rightDoorOpen, isDupeRoom, hasEyes, hasSeekEyes, isSeekHallway, isSeekChase, isSeekFinale;
    float animMain[3], animLL[3], animLR[3], animRL[3], animRR[3]; 
    float lightLevel, leftDoorOffset, rightDoorOffset, eyesX, eyesY, eyesZ; 
    float pZ[10], pY[10], pW[10], pH[10], pR[10], pG[10], pB[10]; int pSide[10];    
    int leftRoomSlotTypeL[3], leftRoomSlotItemL[3], leftRoomSlotTypeR[3], leftRoomSlotItemR[3]; bool leftRoomDrawerOpenL[3], leftRoomDrawerOpenR[3];
    int rightRoomSlotTypeL[3], rightRoomSlotItemL[3], rightRoomSlotTypeR[3], rightRoomSlotItemR[3]; bool rightRoomDrawerOpenL[3], rightRoomDrawerOpenR[3];
    int correctDupePos, dupeNumbers[3]; 
} rooms[TOTAL_ROOMS];

int playerHealth = 100, flashRedFrames = 0, seekStartRoom = 0, playerCoins = 0, elevatorTimer = 1593, messageTimer = 0;
bool hasKey = false, lobbyKeyPickedUp = false, isCrouching = false, isDead = false, inElevator = true, elevatorDoorsOpen = false, elevatorClosing = false, elevatorJamFinished = false;
bool doorOpen[TOTAL_ROOMS] = {false}; HideState hideState = NOT_HIDING; 
float elevatorDoorOffset = 0.0f; char uiMessage[50] = "";
bool screechActive = false, rushActive = false, seekActive = false, inEyesRoom = false, isLookingAtEyes = false;
int screechTimer = 0, screechCooldown = 0, rushState = 0, rushTimer = 0, rushCooldown = 0, seekState = 0, seekTimer = 0, eyesDamageTimer = 0, eyesDamageAccumulator = 0, eyesGraceTimer = 0, eyesSoundCooldown = 0;
float screechX = 0.0f, screechY = 0.0f, screechZ = 0.0f, screechOffsetX = 0.0f, screechOffsetY = 0.0f, screechOffsetZ = 0.0f, rushStartTimer = 1.0f, rushZ = 0.0f, rushTargetZ = 0.0f, seekZ = 0.0f, seekSpeed = 0.0f, seekMaxSpeed = 0.038f; 

bool figureActive = false;
int figureState = 0; 
float figureX = 0.0f, figureY = 0.0f, figureZ = 0.0f;
float figureSpeed = 0.03f;
int figureWaypoint = 0;
int figureListenTimer = 0;

int getDisplayRoom(int idx) { return (idx < 0) ? 0 : idx + 1; }
int getNextDoorIndex(int currentIdx) { return (currentIdx >= seekStartRoom && currentIdx <= seekStartRoom + 2) ? seekStartRoom + 3 : currentIdx + 1; }

ndspWaveBuf loadWav(const char* path) {
    ndspWaveBuf w={0}; FILE* f=fopen(path,"rb"); if(!f) return w; fseek(f,12,SEEK_SET);
    char id[4]; u32 sz; bool found=false;
    while(fread(id,1,4,f)==4){ fread(&sz,4,1,f); if(strncmp(id,"data",4)==0){found=true;break;} fseek(f,sz,SEEK_CUR); }
    if(!found){ fclose(f); return w; } s16* buf=(s16*)linearAlloc(sz); if(!buf){fclose(f); return w;}
    fread(buf,1,sz,f); fclose(f); DSP_FlushDataCache(buf,sz);
    w.data_vaddr=buf; w.nsamples=sz/2; w.looping=false; w.status=NDSP_WBUF_FREE; return w;
}

void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide, int colType = 0, float light = 1.0f) {
    r*=light*globalTintR; g*=light*globalTintG; b*=light*globalTintB; if(r>1.0f)r=1.0f; if(g>1.0f)g=1.0f; if(b>1.0f)b=1.0f;
    float x2=x+w, y2=y+h, z2=z+d; vertex v[] = {
        {{x,y,z,1},{r,g,b,1}},{{x2,y,z,1},{r,g,b,1}},{{x,y2,z,1},{r,g,b,1}}, {{x2,y,z,1},{r,g,b,1}},{{x2,y2,z,1},{r,g,b,1}},{{x,y2,z,1},{r,g,b,1}},
        {{x,y,z2,1},{r,g,b,1}},{{x2,y,z2,1},{r,g,b,1}},{{x,y2,z2,1},{r,g,b,1}}, {{x2,y,z2,1},{r,g,b,1}},{{x2,y2,z2,1},{r,g,b,1}},{{x,y2,z2,1},{r,g,b,1}},
        {{x,y,z,1},{r,g,b,1}},{{x,y2,z,1},{r,g,b,1}},{{x,y,z2,1},{r,g,b,1}}, {{x,y2,z,1},{r,g,b,1}},{{x,y2,z2,1},{r,g,b,1}},{{x,y,z2,1},{r,g,b,1}},
        {{x2,y,z,1},{r,g,b,1}},{{x2,y2,z,1},{r,g,b,1}},{{x2,y,z2,1},{r,g,b,1}}, {{x2,y2,z,1},{r,g,b,1}},{{x2,y2,z2,1},{r,g,b,1}},{{x2,y,z2,1},{r,g,b,1}},
        {{x,y2,z,1},{r,g,b,1}},{{x2,y2,z,1},{r,g,b,1}},{{x,y2,z2,1},{r,g,b,1}}, {{x2,y2,z,1},{r,g,b,1}},{{x2,y2,z2,1},{r,g,b,1}},{{x,y2,z2,1},{r,g,b,1}},
        {{x,y,z,1},{r,g,b,1}},{{x2,y,z,1},{r,g,b,1}},{{x,y,z2,1},{r,g,b,1}}, {{x2,y,z,1},{r,g,b,1}},{{x2,y,z2,1},{r,g,b,1}},{{x,y,z2,1},{r,g,b,1}}
    }; for(int i=0;i<36;i++) world_mesh.push_back(v[i]);
    if(collide) collisions.push_back({fmin(x,x2),fmin(y,y2),fmin(z,z2),fmax(x,x2),fmax(y,y2),fmax(z,z2),colType});
}

bool checkCollision(float x, float y, float z, float h) {
    if(isDead) return true; float r=0.2f; 
    for(auto& b:collisions) { if(b.type==4)continue; if(x+r>b.minX && x-r<b.maxX && z+r>b.minZ && z-r<b.maxZ && y+h>b.minY && y<b.maxY) return true; } return false;
}
bool checkLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2) {
    for(auto& b:collisions) { if(b.type==4)continue; 
        float tmin=0.0f,tmax=1.0f, dx=x2-x1, dy=y2-y1, dz=z2-z1;
        if(fabsf(dx)>0.0001f){ float tx1=(b.minX-x1)/dx, tx2=(b.maxX-x1)/dx; tmin=fmaxf(tmin,fminf(tx1,tx2)); tmax=fminf(tmax,fmaxf(tx1,tx2)); } else if(x1<=b.minX||x1>=b.maxX)continue;
        if(fabsf(dy)>0.0001f){ float ty1=(b.minY-y1)/dy, ty2=(b.maxY-y1)/dy; tmin=fmaxf(tmin,fminf(ty1,ty2)); tmax=fminf(tmax,fmaxf(ty1,ty2)); } else if(y1<=b.minY||y1>=b.maxY)continue;
        if(fabsf(dz)>0.0001f){ float tz1=(b.minZ-z1)/dz, tz2=(b.maxZ-z1)/dz; tmin=fmaxf(tmin,fminf(tz1,tz2)); tmax=fminf(tmax,fmaxf(tz1,tz2)); } else if(z1<=b.minZ||z1>=b.maxZ)continue;
        if(tmin<=tmax && tmax>0.0f && tmin<1.0f) return false; 
    } return true; 
}

void buildLamp(float x, float z, float L=1.0f) { addBox(x-0.15f,0.46f,z-0.15f,0.3f,0.05f,0.3f,0.3f,0.15f,0.05f,false,0,L); addBox(x-0.05f,0.51f,z-0.05f,0.1f,0.2f,0.1f,0.2f,0.3f,0.4f,false,0,L); addBox(x-0.2f,0.71f,z-0.2f,0.4f,0.25f,0.4f,0.9f,0.9f,0.8f,false,0,L); }
void buildCabinet(float zC, bool isL, float L=1.0f, float offX=0.0f, int item=0) {
    float bX=(isL?-2.95f:2.85f)+offX, tX=(isL?-2.95f:2.15f)+offX, fX=(isL?-2.25f:2.15f)+offX, hX=(isL?fX+0.1f:fX-0.03f); 
    addBox(bX,0,zC-0.5f,0.1f,1.5f,1.0f,0.3f,0.18f,0.1f,false,0,L); addBox(tX,1.5f,zC-0.5f,0.8f,0.1f,1.0f,0.3f,0.18f,0.1f,false,0,L); addBox(tX,0,zC-0.5f,0.8f,1.5f,0.1f,0.3f,0.18f,0.1f,false,0,L); 
    addBox(tX,0,zC+0.4f,0.8f,1.5f,0.1f,0.3f,0.18f,0.1f,false,0,L); addBox(fX,0,zC-0.4f,0.1f,1.5f,0.35f,0.3f,0.18f,0.1f,false,0,L); addBox(fX,0,zC+0.05f,0.1f,1.5f,0.35f,0.3f,0.18f,0.1f,false,0,L); 
    addBox(hX,0.6f,zC-0.15f,0.03f,0.15f,0.04f,0.9f,0.75f,0.1f,false,0,L); addBox(hX,0.6f,zC+0.11f,0.03f,0.15f,0.04f,0.9f,0.75f,0.1f,false,0,L); 
    if(hideState!=IN_CABINET) addBox((isL?-2.85f:2.25f)+offX,0.01f,zC-0.4f,0.6f,1.48f,0.8f,0,0,0,false,0,L); collisions.push_back({(isL?-2.9f:2.1f)+offX,0.0f,zC-0.5f,(isL?-2.1f:2.9f)+offX,1.5f,zC+0.5f,1});
}
void buildBed(float zC, bool isL, int item, float L=1.0f, float offX=0.0f) {
    float x=(isL?-2.95f:1.55f)+offX, sX=(isL?-1.65f:1.55f)+offX, pX=(isL?-2.65f:2.15f)+offX; 
    addBox(x,0.4f,zC+1.25f,1.4f,0.1f,-2.5f,0.4f,0.1f,0.1f,true,0,L); addBox(x,0,zC+1.25f,0.1f,0.4f,-0.1f,0.2f,0.1f,0.05f,true,0,L); addBox(x+1.3f,0,zC+1.25f,0.1f,0.4f,-0.1f,0.2f,0.1f,0.05f,true,0,L);
    addBox(x,0,zC-1.15f,0.1f,0.4f,-0.1f,0.2f,0.1f,0.05f,true,0,L); addBox(x+1.3f,0,zC-1.15f,0.1f,0.4f,-0.1f,0.2f,0.1f,0.05f,true,0,L); addBox(sX,0.2f,zC+1.25f,0.1f,0.2f,-2.5f,0.4f,0.1f,0.1f,true,0,L); 
    addBox(pX,0.5f,zC+1.0f,0.5f,0.08f,-0.6f,0.9f,0.9f,0.9f,false,0,L); 
    if(item==1){ float ix=(isL?-2.2f:2.1f)+offX; addBox(ix,0.52f,zC-0.1f,0.14f,0.035f,0.035f,0.9f,0.75f,0.1f,false,0,L); addBox(ix+0.1f,0.52f,zC-0.13f,0.07f,0.035f,0.1f,0.9f,0.75f,0.1f,false,0,L); addBox(ix+0.03f,0.52f,zC-0.065f,0.035f,0.035f,0.05f,0.9f,0.75f,0.1f,false,0,L); } 
    else if(item==3) addBox((isL?-2.2f:2.1f)+offX+0.02f,0.52f,zC-0.01f,0.04f,0.005f,0.04f,1.0f,0.85f,0.0f,false,0,L);
    collisions.push_back({x,0.0f,zC-1.25f,x+1.4f,0.6f,zC+1.25f,2});
}
void buildDresser(float zC, bool isL, float openFactor, int item, float L=1.0f, float offX=0.0f, bool hasLamp=false) {
    float frX=(isL?-2.95f:2.45f)+offX, oOff=openFactor*(isL?0.35f:-0.35f), trX=(isL?(-2.9f+oOff):(2.4f+oOff))+offX, hX=(isL?(-2.4f+oOff):(2.35f+oOff))+offX, iX=(isL?(-2.6f+oOff):(2.5f+oOff))+offX;
    addBox(frX+0.05f,0,zC-0.35f,0.05f,0.2f,0.05f,0.2f,0.1f,0.05f,false,0,L); addBox(frX+0.05f,0,zC+0.3f,0.05f,0.2f,0.05f,0.2f,0.1f,0.05f,false,0,L);
    addBox(frX+0.4f,0,zC-0.35f,0.05f,0.2f,0.05f,0.2f,0.1f,0.05f,false,0,L); addBox(frX+0.4f,0,zC+0.3f,0.05f,0.2f,0.05f,0.2f,0.1f,0.05f,false,0,L);
    addBox(frX,0.2f,zC-0.4f,0.5f,0.3f,0.8f,0.3f,0.15f,0.1f,true,3,L); addBox(trX,0.3f,zC-0.35f,0.5f,0.15f,0.7f,0.25f,0.12f,0.08f,false,0,L); addBox(hX,0.35f,zC-0.1f,0.05f,0.05f,0.2f,0.8f,0.8f,0.8f,false,0,L);
    if(openFactor > 0.1f){ if(item==1){ addBox(iX,0.46f,zC-0.1f,0.14f,0.035f,0.035f,0.9f,0.75f,0.1f,false,0,L); addBox(iX+0.1f,0.46f,zC-0.13f,0.07f,0.035f,0.1f,0.9f,0.75f,0.1f,false,0,L); addBox(iX+0.03f,0.46f,zC-0.065f,0.035f,0.035f,0.05f,0.9f,0.75f,0.1f,false,0,L); } else if(item==2) addBox(iX,0.46f,zC-0.05f,0.15f,0.02f,0.08f,0.8f,0.6f,0.4f,false,0,L); else if(item==3) addBox(iX+0.02f,0.46f,zC-0.05f,0.04f,0.005f,0.04f,1.0f,0.85f,0.0f,false,0,L); }
    if(hasLamp) buildLamp((isL?-2.7f:2.7f)+offX,zC,L);
}
void buildChest(float x, float z, float openFactor, float L=1.0f) {
    bool isOpen = (openFactor > 0.5f);
    addBox(x-0.4f,0,z-0.3f,0.8f,0.4f,0.6f,0.3f,0.15f,0.05f,true,0,L); addBox(x-0.42f,0,z-0.32f,0.05f,0.4f,0.05f,0.8f,0.7f,0.1f,false,0,L); addBox(x+0.37f,0,z-0.32f,0.05f,0.4f,0.05f,0.8f,0.7f,0.1f,false,0,L);
    if(!isOpen){ addBox(x-0.4f,0.4f,z-0.3f,0.8f,0.2f,0.6f,0.35f,0.18f,0.08f,false,0,L); addBox(x-0.05f,0.3f,z+0.3f,0.1f,0.15f,0.05f,0.8f,0.8f,0.8f,false,0,L); } else{ addBox(x-0.4f,0.4f,z-0.4f,0.8f,0.6f,0.1f,0.35f,0.18f,0.08f,false,0,L); addBox(x-0.2f,0.35f,z-0.1f,0.4f,0.05f,0.2f,1.0f,0.85f,0.0f,false,0,L); }
}
void addWallWithDoors(float z, bool lD, bool lO, bool cD, bool cO, bool rD, bool rO, int rm, float L=1.0f) {
    addBox(-3.0f,0,z,0.4f,1.8f,-0.2f,0.2f,0.15f,0.1f,true,0,L); if(!lD)addBox(-2.6f,0,z,1.2f,1.8f,-0.2f,0.2f,0.15f,0.1f,true,0,L);else addBox(-2.6f,1.4f,z,1.2f,0.4f,-0.2f,0.2f,0.15f,0.1f,false,0,L);
    addBox(-1.4f,0,z,0.8f,1.8f,-0.2f,0.2f,0.15f,0.1f,true,0,L); if(!cD)addBox(-0.6f,0,z,1.2f,1.8f,-0.2f,0.2f,0.15f,0.1f,true,0,L);else addBox(-0.6f,1.4f,z,1.2f,0.4f,-0.2f,0.2f,0.15f,0.1f,false,0,L);
    addBox(0.6f,0,z,0.8f,1.8f,-0.2f,0.2f,0.15f,0.1f,true,0,L); if(!rD)addBox(1.4f,0,z,1.2f,1.8f,-0.2f,0.2f,0.15f,0.1f,true,0,L);else addBox(1.4f,1.4f,z,1.2f,0.4f,-0.2f,0.2f,0.15f,0.1f,false,0,L); addBox(2.6f,0,z,0.4f,1.8f,-0.2f,0.2f,0.15f,0.1f,true,0,L);
    auto dr = [&](float dx, bool o) {
        if(!o){ addBox(dx,0,z,1.2f,1.4f,-0.1f,0.15f,0.08f,0.05f,true,0,L); addBox(dx+0.4f,1.1f,z+0.02f,0.4f,0.12f,0.02f,0.8f,0.7f,0.2f,false,0,L); addBox(dx+1.05f,0.7f,z+0.02f,0.05f,0.15f,0.03f,0.6f,0.6f,0.6f,false,0,L); if(rooms[rm].isLocked) addBox(dx+0.9f,0.6f,z+0.05f,0.2f,0.2f,0.05f,0.8f,0.8f,0.8f,false,0,L); } 
        else { addBox(dx,0,z,0.1f,1.4f,-1.2f,0.3f,0.15f,0.08f,true,0,L); addBox(dx+0.1f,1.1f,z-0.8f,0.02f,0.12f,0.4f,0.8f,0.7f,0.2f,false,0,L); addBox(dx+0.1f,0.7f,z-1.05f,0.03f,0.15f,0.05f,0.6f,0.6f,0.6f,false,0,L); }
    }; if(lD)dr(-2.6f,lO); if(cD)dr(-0.6f,cO); if(rD)dr(1.4f,rO);
}

void buildWorld(int cChunk, int pRm) {
    world_mesh.clear(); collisions.clear();
    if(screechActive){ addBox(screechX-0.2f,screechY,screechZ-0.2f,0.4f,0.4f,0.4f,0.05f,0.05f,0.05f,false); addBox(screechX-0.22f,screechY+0.1f,screechZ-0.22f,0.44f,0.05f,0.44f,0.9f,0.9f,0.9f,false); addBox(screechX-0.22f,screechY+0.25f,screechZ-0.22f,0.44f,0.05f,0.44f,0.9f,0.9f,0.9f,false); }
    if(rushActive && rushState==2){ addBox(-1.2f,0.2f,rushZ-0.5f,2.4f,2.0f,1.0f,0.05f,0.05f,0.05f,false); addBox(-0.8f,1.4f,rushZ-0.55f,0.4f,0.4f,0.1f,0.9f,0.9f,0.9f,false); addBox(0.4f,1.4f,rushZ-0.55f,0.4f,0.4f,0.1f,0.9f,0.9f,0.9f,false); addBox(-0.6f,0.5f,rushZ-0.55f,1.2f,0.6f,0.1f,0.8f,0.8f,0.8f,false); }
    if(seekActive){
        float sY=0.0f,sH=1.1f; if(seekState==1){ if(seekTimer<=130)sY=-1.1f+(seekTimer/130.0f)*1.1f; else{sY=0;sH=1.1f;} srand(seekTimer); for(int d=0;d<8;d++){ float dx=-1.5f+(rand()%30)/10.0f, dz=seekZ-0.5f-(rand()%30)/10.0f, dy=1.8f-fmod((seekTimer+d*20)*0.05f,1.8f); addBox(dx,dy,dz,0.08f,0.2f,0.08f,0.05f,0.05f,0.05f,false); } srand(time(NULL)); } else if(seekState==2){sY=0;sH=1.1f;}
        addBox(-0.3f,sY,seekZ-0.3f,0.6f,sH,0.6f,0.05f,0.05f,0.05f,false); addBox(-0.15f,sY+0.8f,seekZ-0.35f,0.3f,0.2f,0.06f,0.9f,0.9f,0.9f,false,0,1.5f); addBox(-0.05f,sY+0.8f,seekZ-0.38f,0.1f,0.2f,0.04f,0,0,0,false,0,1.5f); if(seekState==1) addBox(-1.0f,0.01f,seekZ-1.0f,2.0f,0.01f,2.0f,0.02f,0.02f,0.02f,false);
    }
    if(figureActive){
        addBox(figureX-0.2f, figureY, figureZ-0.1f, 0.15f, 0.9f, 0.2f, 0.3f, 0.05f, 0.05f, true); 
        addBox(figureX+0.05f, figureY, figureZ-0.1f, 0.15f, 0.9f, 0.2f, 0.3f, 0.05f, 0.05f, true);
        addBox(figureX-0.2f, figureY+0.9f, figureZ-0.1f, 0.4f, 0.7f, 0.2f, 0.4f, 0.1f, 0.05f, true);
        addBox(figureX-0.35f, figureY+0.5f, figureZ-0.1f, 0.15f, 1.1f, 0.2f, 0.3f, 0.05f, 0.05f, false);
        addBox(figureX+0.2f, figureY+0.5f, figureZ-0.1f, 0.15f, 1.1f, 0.2f, 0.3f, 0.05f, 0.05f, false);
        addBox(figureX-0.15f, figureY+1.6f, figureZ-0.15f, 0.3f, 0.3f, 0.3f, 0.5f, 0.1f, 0.05f, false);
        addBox(figureX-0.1f, figureY+1.65f, figureZ-0.16f, 0.2f, 0.2f, 0.02f, 0.8f, 0.8f, 0.5f, false, 0, 1.5f);
    }
    if(cChunk<2){
        globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f;
        addBox(-2,0,5,4,0.01f,4,0.2f,0.2f,0.2f,false); addBox(-2,2,5,4,0.1f,4,0.8f,0.8f,0.8f,false); addBox(-2,0,9,4,2,0.1f,0.4f,0.3f,0.2f,true); addBox(-2,0,5,0.1f,2,4,0.4f,0.3f,0.2f,true); addBox(1.9f,0,5,0.1f,2,4,0.4f,0.3f,0.2f,true);   
        addBox(1.8f,0.6f,6.5f,0.15f,0.3f,0.2f,0.1f,0.1f,0.1f,false); addBox(1.75f,0.7f,6.55f,0.05f,0.1f,0.1f,0,0.8f,0,false,0,1.5f); addBox(-2.0f-elevatorDoorOffset,0,5.05f,2,2,0.1f,0.6f,0.6f,0.6f,true); addBox(0.0f+elevatorDoorOffset,0,5.05f,2,2,0.1f,0.6f,0.6f,0.6f,true);  
        if(elevatorDoorOffset<0.05f) addBox(-0.02f,0,5.04f,0.04f,2,0.12f,0,0,0,false); if(elevatorClosing) collisions.push_back({-2.0f,0.0f,4.8f,2.0f,2.0f,5.1f,0});
        addBox(-6,0,5,12,0.01f,-15,0.22f,0.15f,0.1f,false); addBox(-6,1.8f,5,12,0.01f,-15,0.1f,0.1f,0.1f,false); addBox(-6,0,5,0.1f,1.8f,-15,0.3f,0.3f,0.3f,true); addBox(6,0,5,0.1f,1.8f,-15,0.3f,0.3f,0.3f,true);  
        addBox(-6,0,-10,3,1.8f,0.1f,0.25f,0.2f,0.15f,true); addBox(3,0,-10,3,1.8f,0.1f,0.25f,0.2f,0.15f,true); addBox(-6,0,4.9f,4,1.8f,0.1f,0.25f,0.15f,0.1f,true); addBox(2,0,4.9f,4,1.8f,0.1f,0.25f,0.15f,0.1f,true);  
        addBox(-6,0,-7,3.5f,0.8f,-0.8f,0.3f,0.15f,0.1f,true); addBox(-3.3f,0,-7.8f,0.8f,0.8f,-1.0f,0.3f,0.15f,0.1f,true); addBox(-2.5f,0.1f,-8.6f,1,0.05f,-1.4f,0.8f,0.7f,0.2f,false); addBox(-2.5f,0.15f,-8.6f,0.05f,0.45f,-0.05f,0.8f,0.7f,0.2f,false); 
        addBox(-1.55f,0.15f,-8.6f,0.05f,0.45f,-0.05f,0.8f,0.7f,0.2f,false); addBox(-2.5f,0.15f,-9.95f,0.05f,0.45f,-0.05f,0.8f,0.7f,0.2f,false); addBox(-1.55f,0.15f,-9.95f,0.05f,0.45f,-0.05f,0.8f,0.7f,0.2f,false); addBox(-2.5f,0.6f,-8.6f,1,0.05f,-1.4f,0.8f,0.7f,0.2f,true); 
        if(!lobbyKeyPickedUp){ addBox(-4.8f,0.9f,-9.9f,0.2f,0.2f,0.05f,0.3f,0.2f,0.1f,false); addBox(-4.72f,0.75f,-9.86f,0.035f,0.1f,0.035f,1.0f,0.84f,0.0f,false); }
    }
    int st=pRm-1, en=pRm+2; if(pRm>=seekStartRoom && pRm<=seekStartRoom+2){st=seekStartRoom;en=seekStartRoom+3;} if(st<0)st=0; if(en>TOTAL_ROOMS-1)en=TOTAL_ROOMS-1;
    for(int i=st; i<=en; i++){
        float z=-10-(i*10), L=rooms[i].lightLevel, wL=(i>0)?rooms[i-1].lightLevel:1.0f; bool tAN=(!rooms[i].isSeekChase && !rooms[i].isSeekHallway && !rooms[i].isSeekFinale && i==pRm+2);
        if(seekState==1){globalTintR=1.0f;globalTintG=0.2f;globalTintB=0.2f;} else{globalTintR=1.0f;globalTintG=1.0f;globalTintB=1.0f;}
        if(i==seekStartRoom+9 && rooms[i].isLocked) addBox(-3.0f,0,z+0.2f,6.0f,1.8f,0.1f,0.4f,0.7f,1.0f,true,0,1.5f);
        if(!(i==seekStartRoom+1 || i==seekStartRoom+2)){ if(rooms[i].isDupeRoom){ if(pRm>=i) addWallWithDoors(z,(rooms[i].correctDupePos==0),((rooms[i].correctDupePos==0)&&doorOpen[i]),(rooms[i].correctDupePos==1),((rooms[i].correctDupePos==1)&&doorOpen[i]),(rooms[i].correctDupePos==2),((rooms[i].correctDupePos==2)&&doorOpen[i]),i,wL); else addWallWithDoors(z,true,(rooms[i].correctDupePos==0&&doorOpen[i]),true,(rooms[i].correctDupePos==1&&doorOpen[i]),true,(rooms[i].correctDupePos==2&&doorOpen[i]),i,wL); } else addWallWithDoors(z,(rooms[i].doorPos==0),((rooms[i].doorPos==0)&&doorOpen[i]),(rooms[i].doorPos==1),((rooms[i].doorPos==1)&&doorOpen[i]),(rooms[i].doorPos==2),((rooms[i].doorPos==2)&&doorOpen[i]),i,wL); }
        
        bool isInteriorVisible = true;
        if (!seekActive) {
            if (i > pRm && i >= 0 && !doorOpen[i]) isInteriorVisible = false;
            if (i < pRm && i >= 0 && i+1 < TOTAL_ROOMS && !doorOpen[i+1]) isInteriorVisible = false;
        }

        bool rE=!(rooms[i].isSeekChase||rooms[i].hasSeekEyes)||(i>=pRm && i<=pRm+1);

        if (i == 49) {
            float cL = rooms[i].lightLevel;
            globalTintR=0.9f; globalTintG=0.8f; globalTintB=0.6f; 
            addWallWithDoors(z - 20.0f, false, false, true, doorOpen[i], false, false, i, cL);
            if (isInteriorVisible) {
                addBox(-6.0f, 0.0f, z, 12.0f, 0.01f, -20.0f, 0.2f, 0.15f, 0.1f, true, 0, cL); 
                addBox(-6.0f, 3.6f, z, 12.0f, 0.01f, -20.0f, 0.15f, 0.1f, 0.05f, false, 0, cL); 
                addBox(-6.1f, 0.0f, z, 0.1f, 3.6f, -20.0f, 0.25f, 0.15f, 0.1f, true, 0, cL); 
                addBox(6.0f, 0.0f, z, 0.1f, 3.6f, -20.0f, 0.25f, 0.15f, 0.1f, true, 0, cL);  
                addBox(-6.0f, 0.0f, z, 4.6f, 3.6f, -0.1f, 0.25f, 0.15f, 0.1f, true, 0, cL); 
                addBox(1.4f, 0.0f, z, 4.6f, 3.6f, -0.1f, 0.25f, 0.15f, 0.1f, true, 0, cL);
                addBox(-1.4f, 1.8f, z, 2.8f, 1.8f, -0.1f, 0.25f, 0.15f, 0.1f, true, 0, cL); 
                addBox(-6.0f, 1.8f, z-2.0f, 3.0f, 0.1f, -16.0f, 0.25f, 0.15f, 0.1f, true, 0, cL); 
                addBox(3.0f, 1.8f, z-2.0f, 3.0f, 0.1f, -16.0f, 0.25f, 0.15f, 0.1f, true, 0, cL);  
                addBox(-3.0f, 1.8f, z-15.0f, 6.0f, 0.1f, -3.0f, 0.25f, 0.15f, 0.1f, true, 0, cL); 
                addBox(-3.1f, 1.9f, z-2.0f, 0.1f, 0.6f, -13.0f, 0.15f, 0.08f, 0.05f, true, 0, cL);
                addBox(3.0f, 1.9f, z-2.0f, 0.1f, 0.6f, -13.0f, 0.15f, 0.08f, 0.05f, true, 0, cL);
                addBox(-3.0f, 1.9f, z-14.9f, 6.0f, 0.6f, -0.1f, 0.15f, 0.08f, 0.05f, true, 0, cL);
                for(int stt=0; stt<12; stt++) {
                    float sY = stt * 0.15f; float sZ = z - 2.0f - (stt * 0.25f);
                    addBox(-5.5f, 0.0f, sZ, 2.0f, sY+0.15f, -0.25f, 0.2f, 0.12f, 0.08f, true, 0, cL); 
                    addBox(3.5f, 0.0f, sZ, 2.0f, sY+0.15f, -0.25f, 0.2f, 0.12f, 0.08f, true, 0, cL); 
                }
                addBox(-1.5f, 0.0f, z-7.0f, 3.0f, 0.6f, -4.0f, 0.3f, 0.18f, 0.1f, true, 0, cL); 
                addBox(-1.6f, 0.6f, z-6.9f, 3.2f, 0.1f, -4.2f, 0.2f, 0.1f, 0.05f, true, 0, cL); 
                buildLamp(-1.2f, z-7.5f, cL); buildLamp(1.2f, z-7.5f, cL);
                addBox(-2.5f, 0.0f, z-12.0f, 1.0f, 1.8f, -4.0f, 0.2f, 0.1f, 0.05f, true, 0, cL);
                addBox(1.5f, 0.0f, z-12.0f, 1.0f, 1.8f, -4.0f, 0.2f, 0.1f, 0.05f, true, 0, cL);
            }
            globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f; continue; 
        }

        if(seekState==1){globalTintR=1.0f;globalTintG=0.2f;globalTintB=0.2f;} else if(rooms[i].hasEyes){globalTintR=0.8f;globalTintG=0.3f;globalTintB=1.0f;} else{globalTintR=1.0f;globalTintG=1.0f;globalTintB=1.0f;}
        
        if (isInteriorVisible) {
            if(rE && (rooms[i].hasSeekEyes||rooms[i].isSeekChase)){ srand(i*12345); int wC=rooms[i].hasSeekEyes?rooms[i].seekEyeCount:15; for(int e=0;e<wC;e++){ bool iL=(rand()%2==0); float eZ=z-0.5f-(rand()%90)/10.0f, eY=0.2f+(rand()%160)/100.0f; if(iL){ addBox(-2.95f,eY,eZ,0.1f,0.3f,0.4f,0.05f,0.05f,0.05f,false,0,L); addBox(-2.88f,eY+0.05f,eZ+0.05f,0.06f,0.2f,0.3f,0.9f,0.9f,0.9f,false,0,L); addBox(-2.84f,eY+0.1f,eZ+0.12f,0.04f,0.1f,0.16f,0,0,0,false,0,1.5f); } else{ addBox(2.85f,eY,eZ,0.1f,0.3f,0.4f,0.05f,0.05f,0.05f,false,0,L); addBox(2.82f,eY+0.05f,eZ+0.05f,0.06f,0.2f,0.3f,0.9f,0.9f,0.9f,false,0,L); addBox(2.80f,eY+0.1f,eZ+0.12f,0.04f,0.1f,0.16f,0,0,0,false,0,1.5f); } } srand(time(NULL)); }
            if(rooms[i].hasEyes && rE){ float ex=rooms[i].eyesX, ey=rooms[i].eyesY, ez=rooms[i].eyesZ; addBox(ex-0.2f,ey-0.2f,ez-0.2f,0.4f,0.4f,0.4f,0.6f,0.1f,0.8f,false,0,L); addBox(ex-0.25f,ey-0.1f,ez-0.1f,0.5f,0.2f,0.2f,0.9f,0.9f,0.9f,false,0,L); addBox(ex-0.1f,ey-0.25f,ez-0.1f,0.2f,0.5f,0.2f,0.9f,0.9f,0.9f,false,0,L); addBox(ex-0.26f,ey-0.05f,ez-0.05f,0.02f,0.1f,0.1f,0,0,0,false,0,1.5f); addBox(ex+0.24f,ey-0.05f,ez-0.05f,0.02f,0.1f,0.1f,0,0,0,false,0,1.5f); } 
        }

        if(rooms[i].isSeekChase){ srand(i*777); int oT=rand()%3; float oZ=z-5.0f; if(oT==0) addBox(-3,0.7f,oZ,6,1.1f,0.4f,0.2f,0.15f,0.1f,true,0,L); else if(oT==1){addBox(-3,0,oZ,3,1.8f,0.4f,0.2f,0.15f,0.1f,true,0,L); addBox(0,0.7f,oZ,3,1.1f,0.4f,0.2f,0.15f,0.1f,true,0,L);} else{addBox(0,0,oZ,3,1.8f,0.4f,0.2f,0.15f,0.1f,true,0,L); addBox(-3,0.7f,oZ,3,1.1f,0.4f,0.2f,0.15f,0.1f,true,0,L);} srand(time(NULL)); } 
        else if(rooms[i].isSeekFinale){ addBox(-2.95f,0.4f,z-8.5f,0.1f,1.0f,7.0f,0.4f,0.7f,1.0f,false,0,L); addBox(2.85f,0.4f,z-8.5f,0.1f,1.0f,7.0f,0.4f,0.7f,1.0f,false,0,L); addBox(-3,0,z-2,3.5f,1.8f,0.4f,0.05f,0.05f,0.05f,true,0,L); addBox(-0.1f,0.5f,z-2.1f,0.6f,0.6f,0.6f,1,0,0,false,0,1.5f); rooms[i].pW[0]=2.6f; rooms[i].pZ[0]=z-2.0f; rooms[i].pSide[0]=1; addBox(2,0.8f,z-2.2f,1.0f,0.2f,0.4f,0.05f,0.05f,0.05f,false,0,L); rooms[i].pW[1]=1.8f; rooms[i].pZ[1]=z-3.5f; rooms[i].pSide[1]=0; addBox(1.4f,0,z-3.9f,0.8f,0.3f,0.8f,1,0.4f,0,false,0,L); addBox(1.6f,0.3f,z-3.7f,0.4f,0.4f,0.4f,1,0.8f,0,false,0,L); addBox(-0.5f,0,z-5,3.5f,1.8f,0.4f,0.05f,0.05f,0.05f,true,0,L); addBox(-0.5f,0.5f,z-5.1f,0.6f,0.6f,0.6f,1,0,0,false,0,1.5f); rooms[i].pW[2]=-2.6f; rooms[i].pZ[2]=z-5.0f; rooms[i].pSide[2]=1; addBox(-3,0.8f,z-5.2f,1.0f,0.2f,0.4f,0.05f,0.05f,0.05f,false,0,L); rooms[i].pW[3]=-1.8f; rooms[i].pZ[3]=z-6.5f; rooms[i].pSide[3]=0; addBox(-2.2f,0,z-6.9f,0.8f,0.3f,0.8f,1,0.4f,0,false,0,L); addBox(-2.0f,0.3f,z-6.7f,0.4f,0.4f,0.4f,1,0.8f,0,false,0,L); addBox(-3,0,z-8,3.5f,1.8f,0.4f,0.05f,0.05f,0.05f,true,0,L); addBox(-0.1f,0.5f,z-8.1f,0.6f,0.6f,0.6f,1,0,0,false,0,1.5f); rooms[i].pW[4]=2.6f; rooms[i].pZ[4]=z-8.0f; rooms[i].pSide[4]=1; addBox(2,0.8f,z-8.2f,1.0f,0.2f,0.4f,0.05f,0.05f,0.05f,false,0,L); rooms[i].pW[5]=0.8f; rooms[i].pZ[5]=z-9.0f; rooms[i].pSide[5]=0; addBox(0.4f,0,z-9.4f,0.8f,0.3f,0.8f,1,0.4f,0,false,0,L); addBox(0.6f,0.3f,z-9.2f,0.4f,0.4f,0.4f,1,0.8f,0,false,0,L); } 
        else if(rooms[i].isSeekHallway){ addBox(-2.95f,0.4f,z-8.5f,0.1f,1.0f,7.0f,0.4f,0.7f,1.0f,false,0,L); addBox(2.85f,0.4f,z-8.5f,0.1f,1.0f,7.0f,0.4f,0.7f,1.0f,false,0,L); } 
        
        addBox(-3,0,z,6,0.01f,-10,0.2f,0.1f,0.05f,false,0,L); addBox(-3,1.8f,z,6,0.01f,-10,0.15f,0.15f,0.15f,false,0,L); 
        
        auto drawSide = [&](bool isL) {
            float dZ=z+(isL?rooms[i].leftDoorOffset:rooms[i].rightDoorOffset), bL=fabsf(dZ-z), aL=fabsf((z-10.0f)-(dZ-1.2f)), bX=isL?-3.0f:2.9f, dO=isL?rooms[i].leftDoorOpen:rooms[i].rightDoorOpen;
            if(bL>0.05f) addBox(bX,0,z,0.1f,1.8f,-bL,0.25f,0.2f,0.15f,true,0,L); if(aL>0.05f) addBox(bX,0,dZ-1.2f,0.1f,1.8f,-aL,0.25f,0.2f,0.15f,true,0,L); addBox(bX,1.4f,dZ,0.1f,0.4f,-1.2f,0.25f,0.2f,0.15f,false,0,L); addBox(bX-(isL?-0.05f:0.0f),0,dZ,0.05f,1.4f,-0.05f,0.15f,0.1f,0.05f,false,0,L); addBox(bX-(isL?-0.05f:0.0f),0,dZ-1.15f,0.05f,1.4f,-0.05f,0.15f,0.1f,0.05f,false,0,L); addBox(bX-(isL?-0.05f:0.0f),1.35f,dZ,0.05f,0.05f,-1.2f,0.15f,0.1f,0.05f,false,0,L); 
            
            if(dO){ addBox(isL?-4.1f:3.0f,0,dZ-1.15f,1.1f,1.4f,0.05f,0.12f,0.06f,0.03f,true,0,L); addBox(isL?-4.0f:3.9f,0.7f,dZ-1.10f,0.1f,0.05f,0.05f,0.8f,0.7f,0.2f,false,0,L); collisions.push_back({isL?-4.1f:3.0f,0.0f,dZ-1.15f,isL?-3.0f:4.1f,1.8f,dZ-1.10f,4}); } else{ addBox(isL?-3.0f:2.95f,0,dZ-0.05f,0.05f,1.4f,-1.1f,0.12f,0.06f,0.03f,true,0,L); addBox(isL?-2.95f:2.9f,0.7f,dZ-0.15f,0.05f,0.1f,-0.1f,0.8f,0.7f,0.2f,false,0,L); } 
            
            float srZ=dZ+2.5f, sW=isL?-9.0f:3.0f; addBox(sW,0,srZ,6.0f,0.01f,-5.0f,0.18f,0.1f,0.05f,false,0,L); addBox(sW,1.8f,srZ,6.0f,0.01f,-5.0f,0.12f,0.12f,0.12f,false,0,L); addBox(isL?-9.0f:8.9f,0,srZ,0.1f,1.8f,-5.0f,0.25f,0.2f,0.15f,true,0,L); addBox(sW,0,srZ,6.0f,1.8f,0.1f,0.25f,0.2f,0.15f,true,0,L); addBox(sW,0,srZ-5.0f,6.0f,1.8f,-0.1f,0.25f,0.2f,0.15f,true,0,L); 
            
            if (i == pRm) { // OPTIMIZATION: Only draw side-room items if player is in this specific room
                srand(i*(isL?123:321)); if(rand()%2==0){ float pY=0.6f+(rand()%50)/100.0f, pZ=srZ-1.5f-(rand()%20)/10.0f, pW=0.5f+(rand()%40)/100.0f, pH=0.5f+(rand()%40)/100.0f, pR=0.15f+(rand()%35)/100.0f, pG=0.15f+(rand()%35)/100.0f, pB=0.15f+(rand()%35)/100.0f; addBox(isL?-8.95f:8.89f,pY-0.05f,pZ+0.05f,0.06f,pH+0.1f,-pW-0.1f,0.1f,0.05f,0.02f,false,0,L); addBox(isL?-8.9f:8.83f,pY,pZ,0.07f,pH,-pW,pR,pG,pB,false,0,L); } srand(time(NULL));
                for(int s=0;s<3;s++){ float fZ=srZ-0.9f-(s*1.6f); 
                    int tL=isL?rooms[i].leftRoomSlotTypeL[s]:rooms[i].rightRoomSlotTypeL[s], iL=isL?rooms[i].leftRoomSlotItemL[s]:rooms[i].rightRoomSlotItemL[s]; 
                    float oL=isL?rooms[i].animLL[s]:rooms[i].animRL[s];
                    if(tL==1)buildCabinet(fZ,true,L,isL?-6.0f:6.0f,iL); else if(tL==3)buildBed(fZ,true,iL,L,isL?-6.0f:6.0f); else if(tL==5)buildDresser(fZ,true,oL,iL,L,isL?-6.0f:6.0f,true); else if(tL==7||tL==8)buildChest(isL?-8.4f:3.6f,fZ,oL,L); 
                    int tR=isL?rooms[i].leftRoomSlotTypeR[s]:rooms[i].rightRoomSlotTypeR[s], iR=isL?rooms[i].leftRoomSlotItemR[s]:rooms[i].rightRoomSlotItemR[s]; 
                    float oR=isL?rooms[i].animLR[s]:rooms[i].animRR[s];
                    if(tR==2)buildCabinet(fZ,false,L,isL?-6.0f:6.0f,iR); else if(tR==4)buildBed(fZ,false,iR,L,isL?-6.0f:6.0f); else if(tR==6)buildDresser(fZ,false,oR,iR,L,isL?-6.0f:6.0f,true); else if(tR==7||tR==8)buildChest(isL?-3.6f:8.4f,fZ,oR,L); 
                }
            }
        };
        
        if(rooms[i].hasLeftRoom) drawSide(true); else addBox(-3,0,z,0.1f,1.8f,-10,0.25f,0.2f,0.15f,true,0,L); if(rooms[i].hasRightRoom) drawSide(false); else addBox(2.9f,0,z,0.1f,1.8f,-10,0.25f,0.2f,0.15f,true,0,L); 
        addBox(-0.4f,1.78f,z-5.4f,0.8f,0.02f,0.8f,(L>0.5f?0.9f:0.2f),(L>0.5f?0.9f:0.2f),(L>0.5f?0.8f:0.2f),false);
        
        if(isInteriorVisible && !tAN){
            for(int s=0;s<3;s++){ float zC=z-2.5f-(s*2.5f); int t=rooms[i].slotType[s]; if(t==1)buildCabinet(zC,true,L); else if(t==2)buildCabinet(zC,false,L); else if(t==5)buildDresser(zC,true,rooms[i].animMain[s],rooms[i].slotItem[s],L); else if(t==6)buildDresser(zC,false,rooms[i].animMain[s],rooms[i].slotItem[s],L); }
            for(int p=0;p<rooms[i].pCount;p++){ float pZ=rooms[i].pZ[p],pH=rooms[i].pH[p],pW=rooms[i].pW[p],pY=rooms[i].pY[p],cR=rooms[i].hasSeekEyes?0.02f:rooms[i].pR[p],cG=rooms[i].hasSeekEyes?0.02f:rooms[i].pG[p],cB=rooms[i].hasSeekEyes?0.02f:rooms[i].pB[p]; float wX=rooms[i].pSide[p]==0?-2.95f:2.89f, cX=rooms[i].pSide[p]==0?-2.95f:2.88f; addBox(wX,pY-0.05f,z-pZ+0.05f,0.06f,pH+0.1f,-pW-0.1f,0.1f,0.05f,0.02f,false,0,L); addBox(cX,pY,z-pZ,0.07f,pH,-pW,cR,cG,cB,false,0,L); if(rooms[i].hasSeekEyes&&rE){ srand(i*100+p); int nE=2+(int)(pH*pW*30.0f)+(rand()%4); for(int e=0;e<nE;e++){ float eY=pY+0.02f+(rand()%(int)((pH-0.04f)*100))/100.0f, eZ=z-pZ-0.02f-(rand()%(int)((pW-0.04f)*100))/100.0f; addBox(rooms[i].pSide[p]==0?-2.88f:2.87f,eY,eZ,0.01f,0.04f,0.06f,0.9f,0.9f,0.9f,false,0,L); addBox(rooms[i].pSide[p]==0?-2.875f:2.865f,eY+0.01f,eZ-0.01f,0.01f,0.02f,0.04f,0,0,0,false,0,1.5f); } srand(time(NULL)); } }
        }
    } globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f; 
}

void generateRooms() {
    seekStartRoom=30+(rand()%9);
    for(int i=0; i<TOTAL_ROOMS; i++){
        rooms[i].doorPos=rand()%3; rooms[i].isLocked=false; rooms[i].isJammed=false; rooms[i].hasSeekEyes=false; rooms[i].isSeekHallway=false; rooms[i].isSeekChase=false; rooms[i].isSeekFinale=false; rooms[i].seekEyeCount=0;
        bool iSE=(i>=seekStartRoom-5 && i<=seekStartRoom+9), iSCE=(i>=seekStartRoom && i<=seekStartRoom+9); rooms[i].lightLevel=(i>0&&rand()%100<8&&!iSE)?0.3f:1.0f; 
        for(int s=0;s<3;s++){ 
            rooms[i].drawerOpen[s]=false; rooms[i].slotItem[s]=0; 
            rooms[i].animMain[s]=0.0f; rooms[i].animLL[s]=0.0f; rooms[i].animLR[s]=0.0f; rooms[i].animRL[s]=0.0f; rooms[i].animRR[s]=0.0f;
        }
        if(i>=seekStartRoom-5 && i<seekStartRoom){rooms[i].hasSeekEyes=true;rooms[i].seekEyeCount=((i-(seekStartRoom-5)+1)*2)+3;} if(i>=seekStartRoom && i<=seekStartRoom+2){rooms[i].isSeekHallway=true;rooms[i].doorPos=1;} if(i>=seekStartRoom+3 && i<=seekStartRoom+7){rooms[i].isSeekChase=true;rooms[i].doorPos=1;} if(i==seekStartRoom+8){rooms[i].isSeekFinale=true;rooms[i].doorPos=1;}
        rooms[i].isDupeRoom=(!iSCE&&i>1&&(rand()%100<15)); if(rooms[i].isDupeRoom){rooms[i].correctDupePos=rand()%3; int dN=getDisplayRoom(i), f1=dN+(rand()%3+1), f2=dN-(rand()%3+1); if(f2<=0)f2=dN+4; rooms[i].dupeNumbers[0]=f1; rooms[i].dupeNumbers[1]=f2; rooms[i].dupeNumbers[2]=dN+5; rooms[i].dupeNumbers[rooms[i].correctDupePos]=dN;}
        rooms[i].hasEyes=(!iSE&&i>2&&!rooms[i].isDupeRoom&&rand()%100<8); if(rooms[i].hasEyes){rooms[i].eyesX=0.0f;rooms[i].eyesY=0.9f;rooms[i].eyesZ=-10.0f-(i*10.0f)-5.0f;}
        
        if(!rooms[i].isSeekChase&&!rooms[i].isSeekHallway&&!rooms[i].isSeekFinale&&!rooms[i].isDupeRoom){
            rooms[i].hasLeftRoom = (rand()%100 < 30); rooms[i].hasRightRoom = (rand()%100 < 30);
            if(rooms[i].hasLeftRoom){ 
                rooms[i].leftDoorOffset=-3.0f-(rand()%40)/10.0f; bool cL=false; 
                for(int s=0;s<3;s++){ 
                    rooms[i].leftRoomDrawerOpenL[s]=false; 
                    if(rand()%100<45){
                        int r=rand()%100; rooms[i].leftRoomSlotTypeL[s]=(r<25)?1:((r<50)?3:((r<75)?5:7)); 
                        if(rooms[i].leftRoomSlotTypeL[s]==7){if(cL)rooms[i].leftRoomSlotTypeL[s]=5;else cL=true;}
                        rooms[i].leftRoomSlotItemL[s]=(rooms[i].leftRoomSlotTypeL[s]>=3&&rooms[i].leftRoomSlotTypeL[s]<=6&&rand()%100<30)?3:0;
                    }else{rooms[i].leftRoomSlotTypeL[s]=0;rooms[i].leftRoomSlotItemL[s]=0;} 
                    rooms[i].leftRoomDrawerOpenR[s]=false; 
                    if(s==1){rooms[i].leftRoomSlotTypeR[s]=99;rooms[i].leftRoomSlotItemR[s]=0;} 
                    else{
                        if(rand()%100<45){
                            int r=rand()%100; rooms[i].leftRoomSlotTypeR[s]=(r<33)?2:((r<66)?6:8);
                            if(rooms[i].leftRoomSlotTypeR[s]==8){if(cL)rooms[i].leftRoomSlotTypeR[s]=6;else cL=true;}
                            rooms[i].leftRoomSlotItemR[s]=(rooms[i].leftRoomSlotTypeR[s]>=3&&rooms[i].leftRoomSlotTypeR[s]<=6&&rand()%100<30)?3:0;
                        }else{rooms[i].leftRoomSlotTypeR[s]=0;rooms[i].leftRoomSlotItemR[s]=0;}
                    } 
                } 
            }
            if(rooms[i].hasRightRoom){ 
                rooms[i].rightDoorOffset=-3.0f-(rand()%40)/10.0f; bool cR=false; 
                for(int s=0;s<3;s++){ 
                    rooms[i].rightRoomDrawerOpenL[s]=false; 
                    if(s==1){rooms[i].rightRoomSlotTypeL[s]=99;rooms[i].rightRoomSlotItemL[s]=0;} 
                    else{
                        if(rand()%100<45){
                            int r=rand()%100; rooms[i].rightRoomSlotTypeL[s]=(r<33)?1:((r<66)?5:7);
                            if(rooms[i].rightRoomSlotTypeL[s]==7){if(cR)rooms[i].rightRoomSlotTypeL[s]=5;else cR=true;}
                            rooms[i].rightRoomSlotItemL[s]=(rooms[i].rightRoomSlotTypeL[s]>=3&&rooms[i].rightRoomSlotTypeL[s]<=6&&rand()%100<30)?3:0;
                        }else{rooms[i].rightRoomSlotTypeL[s]=0;rooms[i].rightRoomSlotItemL[s]=0;}
                    } 
                    rooms[i].rightRoomDrawerOpenR[s]=false; 
                    if(rand()%100<45){
                        int r=rand()%100; rooms[i].rightRoomSlotTypeR[s]=(r<25)?2:((r<50)?4:((r<75)?6:8));
                        if(rooms[i].rightRoomSlotTypeR[s]==8){if(cR)rooms[i].rightRoomSlotTypeR[s]=6;else cR=true;}
                        rooms[i].rightRoomSlotItemR[s]=(rooms[i].rightRoomSlotTypeR[s]>=3&&rooms[i].rightRoomSlotTypeR[s]<=6&&rand()%100<30)?3:0;
                    }else{rooms[i].rightRoomSlotTypeR[s]=0;rooms[i].rightRoomSlotItemR[s]=0;} 
                } 
            }
            bool bS=false; for(int s=0;s<3;s++){ float sZR=-2.5f-(s*2.5f); int r=rand()%100; if(r<25)rooms[i].slotType[s]=1; else if(r<50)rooms[i].slotType[s]=2; else if(r<75){rooms[i].slotType[s]=5;if(!bS&&rand()%100<15){rooms[i].slotItem[s]=2;bS=true;}else if(rand()%100<30)rooms[i].slotItem[s]=3;} else{rooms[i].slotType[s]=6;if(!bS&&rand()%100<15){rooms[i].slotItem[s]=2;bS=true;}else if(rand()%100<30)rooms[i].slotItem[s]=3;} if(rooms[i].hasLeftRoom&&fabsf(rooms[i].leftDoorOffset-sZR)<2.6f&&(rooms[i].slotType[s]==1||rooms[i].slotType[s]==3||rooms[i].slotType[s]==5)){rooms[i].slotType[s]=0;rooms[i].slotItem[s]=0;} if(rooms[i].hasRightRoom&&fabsf(rooms[i].rightDoorOffset-sZR)<2.6f&&(rooms[i].slotType[s]==2||rooms[i].slotType[s]==4||rooms[i].slotType[s]==6)){rooms[i].slotType[s]=0;rooms[i].slotItem[s]=0;} }
        } else {rooms[i].slotType[0]=0;rooms[i].slotType[1]=0;rooms[i].slotType[2]=0;rooms[i].hasLeftRoom=false;rooms[i].hasRightRoom=false;}
        if(rooms[i].isSeekHallway||rooms[i].isSeekFinale||rooms[i].isSeekChase){rooms[i].pCount=0;} else if(!iSE||rooms[i].hasSeekEyes){ rooms[i].pCount=rand()%5+3; for(int p=0;p<rooms[i].pCount;p++){ bool oL; int tr=0; do{ oL=false; rooms[i].pSide[p]=rand()%2; rooms[i].pZ[p]=1.0f+(rand()%70)/10.0f; rooms[i].pY[p]=0.5f+(rand()%70)/100.0f; rooms[i].pW[p]=0.3f+(rand()%60)/100.0f; rooms[i].pH[p]=0.3f+(rand()%60)/100.0f; if(rooms[i].pSide[p]==0&&rooms[i].hasLeftRoom&&fabsf(rooms[i].pZ[p]-fabsf(rooms[i].leftDoorOffset))<2.8f)oL=true; if(rooms[i].pSide[p]==1&&rooms[i].hasRightRoom&&fabsf(rooms[i].pZ[p]-fabsf(rooms[i].rightDoorOffset))<2.8f)oL=true; for(int op=0;op<p;op++){if(rooms[i].pSide[p]==rooms[i].pSide[op]){if(fabsf(rooms[i].pZ[p]-rooms[i].pZ[op])<(rooms[i].pW[p]+rooms[i].pW[op])*0.6f && fabsf(rooms[i].pY[p]-rooms[i].pY[op])<(rooms[i].pH[p]+rooms[i].pH[op])*0.6f){oL=true;break;}}} tr++; }while(oL&&tr<10); if(oL){rooms[i].pCount--;p--;} else{rooms[i].pR[p]=0.15f+(rand()%35)/100.0f;rooms[i].pG[p]=0.15f+(rand()%35)/100.0f;rooms[i].pB[p]=0.15f+(rand()%35)/100.0f;} } } else rooms[i].pCount=0; 
    }
    rooms[0].doorPos=1; rooms[0].isLocked=true; rooms[49].doorPos=1; rooms[49].isLocked = true; rooms[49].lightLevel = 0.5f; 
    for(int i=2; i<TOTAL_ROOMS-1; i++){
        if(!rooms[i].isDupeRoom&&!rooms[i-1].isDupeRoom&&!(i>=seekStartRoom&&i<=seekStartRoom+9)&&(rand()%3==0)){ rooms[i].isLocked=true; std::vector<int> vS; for(int s=0;s<3;s++)if(rooms[i-1].slotType[s]>2)vS.push_back(s); if(!vS.empty()){rooms[i-1].slotItem[vS[rand()%vS.size()]]=1;} else{rooms[i-1].slotType[1]=5;rooms[i-1].slotItem[1]=1;} }
    }
}

int main() {
    gfxInitDefault(); gfxSet3D(false); irrstInit(); srand(time(NULL)); consoleInit(GFX_BOTTOM, NULL); romfsInit();
    bool audio_ok = R_SUCCEEDED(ndspInit()); 
    ndspWaveBuf sPsst={0},sAttack={0},sCaught={0},sDoor={0},sLockedDoor={0},sDupeAttack={0},sRushScream={0},sEyesAppear={0},sEyesGarble={0},sEyesAttack={0},sEyesHit={0},sSeekRise={0},sSeekChase={0},sSeekEscaped={0},sDeath={0},sElevatorJam={0},sElevatorJamEnd={0};
    ndspWaveBuf sCoinsCollect={0},sDarkRoomEnter={0},sDrawerClose={0},sDrawerOpen={0},sLightsFlicker={0},sWardrobeEnter={0},sWardrobeExit={0};

    if(audio_ok){ 
        ndspSetOutputMode(NDSP_OUTPUT_STEREO); 
        for(int i=0;i<=12;i++){ndspChnSetInterp(i,NDSP_INTERP_LINEAR);ndspChnSetRate(i,44100);ndspChnSetFormat(i,NDSP_FORMAT_MONO_PCM16);} 
        sPsst=loadWav("romfs:/Screech_Psst.wav"); sAttack=loadWav("romfs:/Screech_Attack.wav"); sCaught=loadWav("romfs:/Screech_Caught.wav"); 
        sDoor=loadWav("romfs:/Door_Open.wav"); sLockedDoor=loadWav("romfs:/Locked_Door.wav"); sDupeAttack=loadWav("romfs:/Dupe_Attack.wav"); 
        sRushScream=loadWav("romfs:/Rush_Scream.wav"); sEyesAppear=loadWav("romfs:/Eyes_Appear.wav"); sEyesGarble=loadWav("romfs:/Eyes_Garble.wav"); 
        sEyesGarble.looping=true; sEyesAttack=loadWav("romfs:/Eyes_Attack.wav"); sEyesHit=loadWav("romfs:/Eyes_Hit.wav"); 
        sSeekRise=loadWav("romfs:/Seek_Rise.wav"); sSeekChase=loadWav("romfs:/Seek_Chase.wav"); sSeekChase.looping=true; sSeekEscaped=loadWav("romfs:/Seek_Escaped.wav"); 
        sDeath=loadWav("romfs:/Player_Death.wav"); sElevatorJam=loadWav("romfs:/Elevator_Jam.wav"); sElevatorJamEnd=loadWav("romfs:/Elevator_Jam_End.wav"); 
        sCoinsCollect=loadWav("romfs:/Coins_Collect.wav"); sDarkRoomEnter=loadWav("romfs:/Dark_Room_Enter.wav"); sDrawerClose=loadWav("romfs:/Drawer_Close.wav");
        sDrawerOpen=loadWav("romfs:/Drawer_Open.wav"); sLightsFlicker=loadWav("romfs:/Lights_Flicker.wav"); 
        sWardrobeEnter=loadWav("romfs:/Wardrobe_Enter.wav"); sWardrobeExit=loadWav("romfs:/Wardrobe_Exit.wav");
    }

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE); C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8); C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    generateRooms(); int currentChunk=0, playerCurrentRoom=-1; buildWorld(currentChunk, playerCurrentRoom);
    DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size); shaderProgram_s program; shaderProgramInit(&program); shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]); C3D_BindProgram(&program);
    int uLoc_proj = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx"); C3D_AttrInfo* attr = C3D_GetAttrInfo(); AttrInfo_Init(attr); AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 4); AttrInfo_AddLoader(attr, 1, GPU_FLOAT, 4);
    void* vbo_ptr = linearAlloc(MAX_VERTS * sizeof(vertex)); memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex)); GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
    C3D_BufInfo* buf = C3D_GetBufInfo(); BufInfo_Init(buf); BufInfo_Add(buf, vbo_ptr, sizeof(vertex), 2, 0x10); C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL); C3D_CullFace(GPU_CULL_NONE); 
    C3D_TexEnv* env = C3D_GetTexEnv(0); C3D_TexEnvInit(env); C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    float camX=0, camZ=7.5f, camYaw=0, camPitch=0; const char symbols[] = "@!$#&*%?"; static float startTouchX=0, startTouchY=0; static bool wasTouching=false;
    static int lastRoomForDarkCheck = -1; u64 totalFrames = 0; 

    while (aptMainLoop()) {
        hidScanInput(); irrstScanInput(); u32 kDown=hidKeysDown(), kHeld=hidKeysHeld(); if(kDown&KEY_SELECT)break;
        bool needsVBOUpdate=false; static bool deathSoundPlayed=false; totalFrames++;
        if(isDead){ if(!deathSoundPlayed){ if(audio_ok){ ndspChnWaveBufClear(8); if(sDeath.data_vaddr){sDeath.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(8,&sDeath);} deathSoundPlayed=true; } } }
        if(kDown&KEY_START && isDead){ isDead=false; playerHealth=100; playerCoins=0; inElevator=true; elevatorTimer=1593; camZ=7.5f; currentChunk=0; playerCurrentRoom=-1; generateRooms(); buildWorld(currentChunk,playerCurrentRoom); continue; }
        
        playerCurrentRoom=(camZ>=-10.0f)?-1:(int)((-camZ-10.0f)/10.0f); 
        if(messageTimer>0)messageTimer--; if(screechCooldown>0)screechCooldown--;

        if(totalFrames % 3 == 0) { 
            if(playerCurrentRoom>=0 && playerCurrentRoom<TOTAL_ROOMS) {
                auto stepAnim = [&](bool target, float& val) { if(target && val < 1.0f) { val += 0.15f; if(val>1.0f) val=1.0f; return true; } if(!target && val > 0.0f) { val -= 0.15f; if(val<0.0f) val=0.0f; return true; } return false; };
                for(int s=0; s<3; s++) { if(stepAnim(rooms[playerCurrentRoom].drawerOpen[s], rooms[playerCurrentRoom].animMain[s])) animActive = true; }
            }
        }
        if(animActive) needsVBOUpdate = true;

        if(!isDead){
            if(playerCurrentRoom == 49 && !figureActive) { figureActive = true; figureZ = -10.0f - (49 * 10.0f) - 5.0f; needsVBOUpdate = true; sprintf(uiMessage, "Shh... He can hear you."); messageTimer = 150; }
            if(seekActive && seekState==2){ seekZ-=seekMaxSpeed; needsVBOUpdate=true; if(fabsf(seekZ-camZ)<1.2f){playerHealth=0;isDead=true;} }
            
            if(kDown&KEY_X && hideState==NOT_HIDING){ 
                for(auto& b:collisions){ if((b.type==1||b.type==2)&&camX+0.5f>b.minX&&camX-0.5f<b.maxX&&camZ+0.5f>b.minZ&&camZ-0.5f<b.maxZ){ hideState=(b.type==1?IN_CABINET:UNDER_BED); needsVBOUpdate=true; break; } }
            } else if(kDown&KEY_X) { hideState=NOT_HIDING; needsVBOUpdate=true; }

            int nC=(camZ<-10.0f)?(int)((fabsf(camZ)-10.0f)/10.0f)+1:0; 
            if(nC!=currentChunk||needsVBOUpdate){ currentChunk=nC; needsVBOUpdate=true; buildWorld(currentChunk, playerCurrentRoom); }
            
            float pH=isCrouching?0.5f:1.1f; circlePosition cS, cP; irrstCstickRead(&cS); hidCircleRead(&cP); touchPosition t; hidTouchRead(&t);
            if(hideState==NOT_HIDING){
                if(abs(cS.dx)>10)camYaw-=cS.dx/1560.0f*0.8f; if(abs(cS.dy)>10)camPitch+=cS.dy/1560.0f*0.8f;
                if(abs(cP.dy)>15||abs(cP.dx)>15){ float s=isCrouching?0.16f:0.28f, sy=cP.dy/1560.0f, sx=cP.dx/1560.0f, nX=camX-(sinf(camYaw)*sy-cosf(camYaw)*sx)*s, nZ=camZ-(cosf(camYaw)*sy+sinf(camYaw)*sx)*s; if(!checkCollision(nX,0,camZ,pH))camX=nX; if(!checkCollision(camX,0,nZ,pH))camZ=nZ; }
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW); C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); C3D_FrameDrawOn(target);
        C3D_Mtx proj, view; Mtx_PerspTilt(&proj, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false); Mtx_Identity(&view); Mtx_RotateX(&view, -camPitch, true); Mtx_RotateY(&view, -camYaw, true); Mtx_Translate(&view, -camX, (isCrouching?-0.4f:-0.9f), -camZ, true); Mtx_Multiply(&view, &proj, &view);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_proj, &view); memcpy(vbo_ptr, world_mesh.data(), world_mesh.size() * sizeof(vertex)); GSPGPU_FlushDataCache(vbo_ptr, world_mesh.size() * sizeof(vertex));
        C3D_DrawArrays(GPU_TRIANGLES, 0, world_mesh.size()); C3D_FrameEnd(0);
    }

    if(audio_ok) ndspExit();
    romfsExit(); C3D_Fini(); gfxExit(); return 0;
}
