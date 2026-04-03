#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>
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

// Vertex structure
typedef struct { float pos[4]; float texcoord[2]; float clr[4]; } vertex;
typedef struct { float minX, minY, minZ, maxX, maxY, maxZ; int type; } BBox;
typedef enum { NOT_HIDING, IN_CABINET, UNDER_BED, BEHIND_DOOR } HideState;

// SPLIT MESH: This is REQUIRED to prevent the texture from tinting your solid-color objects.
std::vector<vertex> world_mesh_colored;
std::vector<vertex> world_mesh_textured; 
std::vector<BBox> collisions;

float globalTintR = 1.0f, globalTintG = 1.0f, globalTintB = 1.0f;
C3D_Tex atlasTex;
bool hasAtlas = false; 
char texErrorMessage[100] = "";

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

bool loadTextureFromFile(const char* path, C3D_Tex* tex) {
    FILE* f = fopen(path, "rb"); 
    if (!f) {
        sprintf(texErrorMessage, "Could not find %s", path);
        return false;
    }
    
    fseek(f, 0, SEEK_END); 
    size_t size = ftell(f); 
    fseek(f, 0, SEEK_SET);
    
    void* texData = linearAlloc(size); 
    if (!texData) { 
        sprintf(texErrorMessage, "linearAlloc failed (%zu bytes)", size);
        fclose(f); 
        return false; 
    }
    
    fread(texData, 1, size, f); 
    fclose(f);
    
    Tex3DS_Texture t3x = Tex3DS_TextureImport(texData, size, tex, NULL, false);
    linearFree(texData); 
    
    if (!t3x) {
        sprintf(texErrorMessage, "Tex3DS Import failed");
        return false; 
    }
    
    Tex3DS_TextureFree(t3x);
    C3D_TexSetFilter(tex, GPU_NEAREST, GPU_NEAREST); 
    C3D_TexSetWrap(tex, GPU_REPEAT, GPU_REPEAT);
    return true;
}

void addFaceTextured(vertex v1, vertex v2, vertex v3, vertex v4, vertex v5, vertex v6) {
    world_mesh_textured.push_back(v1); world_mesh_textured.push_back(v2); world_mesh_textured.push_back(v3);
    world_mesh_textured.push_back(v4); world_mesh_textured.push_back(v5); world_mesh_textured.push_back(v6);
}

void addFaceColored(vertex v1, vertex v2, vertex v3, vertex v4, vertex v5, vertex v6) {
    world_mesh_colored.push_back(v1); world_mesh_colored.push_back(v2); world_mesh_colored.push_back(v3);
    world_mesh_colored.push_back(v4); world_mesh_colored.push_back(v5); world_mesh_colored.push_back(v6);
}

void addBoxTextured(float x, float y, float z, float w, float h, float d, float u, float v, float uw, float vh, float repW, float repH, float r, float g, float b, float light = 1.0f) {
    float r_c = r * light * globalTintR;
    float g_c = g * light * globalTintG;
    float b_c = b * light * globalTintB;
    float x2=x+w, y2=y+h, z2=z+d; 
    
    float u1 = u;
    float v1 = v; 
    float u2 = u + (uw * repW);
    float v2 = v + (vh * repH);
    
    addFaceTextured({{x,y,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,y2,z2,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{u1,v1},{r_c,g_c,b_c,1}}); 
    addFaceTextured({{x,y,z,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}); 
    addFaceTextured({{x,y,z,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{u1,v1},{r_c,g_c,b_c,1}}); 
    addFaceTextured({{x2,y,z,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y2,z2,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}); 
    addFaceTextured({{x,y2,z,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x2,y2,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{u1,v2},{r_c,g_c,b_c,1}}); 
    addFaceTextured({{x,y,z,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{u1,v1},{r_c,g_c,b_c,1}}); 
}

void addBoxColored(float x, float y, float z, float w, float h, float d, float r, float g, float b, float light = 1.0f) {
    float r_c=r*light*globalTintR, g_c=g*light*globalTintG, b_c=b*light*globalTintB;
    if(r_c>1.0f) r_c=1.0f; if(g_c>1.0f) g_c=1.0f; if(b_c>1.0f) b_c=1.0f;
    float x2=x+w, y2=y+h, z2=z+d;
    addFaceColored({{x,y,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y2,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{0,0},{r_c,g_c,b_c,1}}); 
    addFaceColored({{x,y,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{0,0},{r_c,g_c,b_c,1}}); 
    addFaceColored({{x,y,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{0,0},{r_c,g_c,b_c,1}}); 
    addFaceColored({{x2,y,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y2,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{0,0},{r_c,g_c,b_c,1}}); 
    addFaceColored({{x,y2,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y2,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{0,0},{r_c,g_c,b_c,1}}); 
    addFaceColored({{x,y,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{0,0},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{0,0},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{0,0},{r_c,g_c,b_c,1}}); 
}

void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide, int colType = 0, float light = 1.0f) {
    addBoxColored(x, y, z, w, h, d, r, g, b, light);
    if(collide) collisions.push_back({fmin(x,x+w),fmin(y,y+h),fmin(z,z+d),fmax(x,x+w),fmax(y,y+h),fmax(z,z+d),colType});
}

void addTiledSurface(float x, float y, float z, float w, float h, float d, float u, float v, float uw, float vh, float chunkSize, float r, float g, float b, float light, bool collide) {
    if(collide) collisions.push_back({fmin(x,x+w),fmin(y,y+h),fmin(z,z+d),fmax(x,x+w),fmax(y,y+h),fmax(z,z+d),0});
    if(fabs(d) > fabs(w)) { float dir = (d>0)?1.0f:-1.0f; for(float curr = 0; curr < fabs(d); curr += chunkSize) { float segD = (curr+chunkSize > fabs(d)) ? (fabs(d)-curr) : chunkSize; addBoxTextured(x, y, z + (curr*dir), w, h, segD*dir, u, v, uw, vh, 1.0f, 1.0f, r, g, b, light); } } 
    else { for(float curr = 0; curr < fabs(w); curr += chunkSize) { float segW = (curr+chunkSize > fabs(w)) ? (fabs(w)-curr) : chunkSize; addBoxTextured(x + curr, y, z, segW, h, d, u, v, uw, vh, 1.0f, 1.0f, r, g, b, light); } }
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
    float wallU = 0.5f, wallV = 0.0f, wallUW = 0.5f, wallVH = 1.0f, chunkSize = 2.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f; 
    
    addTiledSurface(-3.0f,0.4f,z,0.4f,1.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, true); 
    addTiledSurface(-3.0f,0.0f,z,0.4f,0.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, false);
    if(!lD){ addTiledSurface(-2.6f,0.4f,z,1.2f,1.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, true); addTiledSurface(-2.6f,0.0f,z,1.2f,0.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, false); } else addTiledSurface(-2.6f,1.4f,z,1.2f,0.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, false);
    addTiledSurface(-1.4f,0.4f,z,0.8f,1.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, true); addTiledSurface(-1.4f,0.0f,z,0.8f,0.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, false);
    if(!cD){ addTiledSurface(-0.6f,0.4f,z,1.2f,1.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, true); addTiledSurface(-0.6f,0.0f,z,1.2f,0.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, false); } else addTiledSurface(-0.6f,1.4f,z,1.2f,0.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, false);
    addTiledSurface(0.6f,0.4f,z,0.8f,1.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, true); addTiledSurface(0.6f,0.0f,z,0.8f,0.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, false);
    if(!rD){ addTiledSurface(1.4f,0.4f,z,1.2f,1.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, true); addTiledSurface(1.4f,0.0f,z,1.2f,0.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, false); } else addTiledSurface(1.4f,1.4f,z,1.2f,0.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, false);
    addTiledSurface(2.6f,0.4f,z,0.4f,1.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, true); addTiledSurface(2.6f,0.0f,z,0.4f,0.4f,-0.2f, wallU, wallV, wallUW, wallVH, chunkSize, r,g,b, L, false);
    
    auto dr = [&](float dx, bool o) {
        if(!o){ addBox(dx,0,z,1.2f,1.4f,-0.1f,0.15f,0.08f,0.05f,true,0,L); addBox(dx+0.4f,1.1f,z+0.02f,0.4f,0.12f,0.02f,0.8f,0.7f,0.2f,false,0,L); addBox(dx+1.05f,0.7f,z+0.02f,0.05f,0.15f,0.03f,0.6f,0.6f,0.6f,false,0,L); if(rooms[rm].isLocked) addBox(dx+0.9f,0.6f,z+0.05f,0.2f,0.2f,0.05f,0.8f,0.8f,0.8f,false,0,L); } 
        else { addBox(dx,0,z,0.1f,1.4f,-1.2f,0.3f,0.15f,0.08f,true,0,L); addBox(dx+0.1f,1.1f,z-0.8f,0.02f,0.12f,0.4f,0.8f,0.7f,0.2f,false,0,L); addBox(dx+0.1f,0.7f,z-1.05f,0.03f,0.15f,0.05f,0.6f,0.6f,0.6f,false,0,L); }
    }; if(lD)dr(-2.6f,lO); if(cD)dr(-0.6f,cO); if(rD)dr(1.4f,rO);
}

void buildWorld(int cChunk, int pRm) {
    world_mesh_colored.clear(); world_mesh_textured.clear(); collisions.clear();
    world_mesh_colored.reserve(MAX_VERTS/2); world_mesh_textured.reserve(MAX_VERTS/2); collisions.reserve(2000);
    
    float floorU = 0.0f, floorV = 0.0f, floorUW = 0.5f, floorVH = 1.0f;
    float wallU = 0.5f, wallV = 0.0f, wallUW = 0.5f, wallVH = 1.0f;
    float cR = 1.0f, cG = 1.0f, cB = 1.0f; 
    
    if(screechActive){ addBox(screechX-0.2f,screechY,screechZ-0.2f,0.4f,0.4f,0.4f,0.05f,0.05f,0.05f,false); addBox(screechX-0.22f,screechY+0.1f,screechZ-0.22f,0.44f,0.05f,0.44f,0.9f,0.9f,0.9f,false); addBox(screechX-0.22f,screechY+0.25f,screechZ-0.22f,0.44f,0.05f,0.44f,0.9f,0.9f,0.9f,false); }
    if(rushActive && rushState==2){ addBox(-1.2f,0.2f,rushZ-0.5f,2.4f,2.0f,1.0f,0.05f,0.05f,0.05f,false); addBox(-0.8f,1.4f,rushZ-0.55f,0.4f,0.4f,0.1f,0.9f,0.9f,0.9f,false); addBox(0.4f,1.4f,rushZ-0.55f,0.4f,0.4f,0.1f,0.9f,0.9f,0.9f,false); addBox(-0.6f,0.5f,rushZ-0.55f,1.2f,0.6f,0.1f,0.8f,0.8f,0.8f,false); }
    if(seekActive){
        float sY=0.0f,sH=1.1f; if(seekState==1){ if(seekTimer<=130)sY=-1.1f+(seekTimer/130.0f)*1.1f; else{sY=0;sH=1.1f;} srand(seekTimer); for(int d=0;d<8;d++){ float dx=-1.5f+(rand()%30)/10.0f, dz=seekZ-0.5f-(rand()%30)/10.0f, dy=1.8f-fmod((seekTimer+d*20)*0.05f,1.8f); addBox(dx,dy,dz,0.08f,0.2f,0.08f,0.05f,0.05f,0.05f,false); } srand(time(NULL)); } else if(seekState==2){sY=0;sH=1.1f;}
        addBox(-0.3f,sY,seekZ-0.3f,0.6f,sH,0.6f,0.05f,0.05f,0.05f,false); addBox(-0.15f,sY+0.8f,seekZ-0.35f,0.3f,0.2f,0.06f,0.9f,0.9f,0.9f,false,0,1.5f); addBox(-0.05f,sY+0.8f,seekZ-0.38f,0.1f,0.2f,0.04f,0,0,0,false,0,1.5f); if(seekState==1) addBox(-1.0f,0.01f,seekZ-1.0f,2.0f,0.01f,2.0f,0.02f,0.02f,0.02f,false);
    }
    
    if(figureActive){
        addBox(figureX-0.2f, figureY, figureZ-0.1f, 0.15f, 0.9f, 0.2f, 0.3f, 0.05f, 0.05f, true); addBox(figureX+0.05f, figureY, figureZ-0.1f, 0.15f, 0.9f, 0.2f, 0.3f, 0.05f, 0.05f, true); addBox(figureX-0.2f, figureY+0.9f, figureZ-0.1f, 0.4f, 0.7f, 0.2f, 0.4f, 0.1f, 0.05f, true); addBox(figureX-0.35f, figureY+0.5f, figureZ-0.1f, 0.15f, 1.1f, 0.2f, 0.3f, 0.05f, 0.05f, false); addBox(figureX+0.2f, figureY+0.5f, figureZ-0.1f, 0.15f, 1.1f, 0.2f, 0.3f, 0.05f, 0.05f, false); addBox(figureX-0.15f, figureY+1.6f, figureZ-0.15f, 0.3f, 0.3f, 0.3f, 0.5f, 0.1f, 0.05f, false); addBox(figureX-0.1f, figureY+1.65f, figureZ-0.16f, 0.2f, 0.2f, 0.02f, 0.8f, 0.8f, 0.5f, false, 0, 1.5f);
    }
    
    if(cChunk<2){
        globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f;
        addTiledSurface(-2,0,5,4,0.01f,4, floorU,floorV,floorUW,floorVH, 2.0f, cR,cG,cB, 1.0f,false); 
        addTiledSurface(-2,2,5,4,0.1f,4, floorU,floorV,floorUW,floorVH, 2.0f, cR,cG,cB, 1.0f,false); 
        addBox(-2,0,9,4,2,0.1f,0.4f,0.3f,0.2f,true); addBox(-2,0,5,0.1f,2,4,0.4f,0.3f,0.2f,true); addBox(1.9f,0,5,0.1f,2,4,0.4f,0.3f,0.2f,true);   
        addBox(1.8f,0.6f,6.5f,0.15f,0.3f,0.2f,0.1f,0.1f,0.1f,false); addBox(1.75f,0.7f,6.55f,0.05f,0.1f,0.1f,0,0.8f,0,false,0,1.5f); addBox(-2.0f-elevatorDoorOffset,0,5.05f,2,2,0.1f,0.6f,0.6f,0.6f,true); addBox(0.0f+elevatorDoorOffset,0,5.05f,2,2,0.1f,0.6f,0.6f,0.6f,true);  
        if(elevatorDoorOffset<0.05f) addBox(-0.02f,0,5.04f,0.04f,2,0.12f,0,0,0,false); if(elevatorClosing) collisions.push_back({-2.0f,0.0f,4.8f,2.0f,2.0f,5.1f,0});
        addTiledSurface(-6,0,5,12,0.01f,-15, floorU,floorV,floorUW,floorVH, 2.0f, cR,cG,cB, 1.0f,false); 
        addTiledSurface(-6,1.8f,5,12,0.01f,-15, floorU,floorV,floorUW,floorVH, 2.0f, cR,cG,cB, 1.0f,false); 
        addBox(-6,0,5,0.1f,1.8f,-15,0.3f,0.3f,0.3f,true); addBox(6,0,5,0.1f,1.8f,-15,0.3f,0.3f,0.3f,true);  
        addBox(-6,0,-10,3,1.8f,0.1f,0.25f,0.2f,0.15f,true); addBox(3,0,-10,3,1.8f,0.1f,0.25f,0.2f,0.15f,true); addBox(-6,0,4.9f,4,1.8f,0.1f,0.25f,0.15f,0.1f,true); addBox(2,0,4.9f,4,1.8f,0.1f,0.25f,0.15f,0.1f,true);  
        addBox(-6,0,-7,3.5f,0.8f,-0.8f,0.3f,0.15f,0.1f,true); addBox(-3.3f,0,-7.8f,0.8f,0.8f,-1.0f,0.3f,0.15f,0.1f,true); addBox(-2.5f,0.1f,-8.6f,1,0.05f,-1.4f,0.8f,0.7f,0.2f,false); addBox(-2.5f,0.15f,-8.6f,0.05f,0.45f,-0.05f,0.8f,0.7f,0.2f,false); 
        addBox(-1.55f,0.15f,-8.6f,0.05f,0.45f,-0.05f,0.8f,0.7f,0.2f,false); addBox(-2.5f,0.15f,-9.95f,0.05f,0.45f,-0.05f,0.8f,0.7f,0.2f,false); addBox(-1.55f,0.15f,-9.95f,0.05f,0.45f,-0.05f,0.8f,0.7f,0.2f,false); addBox(-2.5f,0.6f,-8.6f,1,0.05f,-1.4f,0.8f,0.7f,0.2f,true); 
        if(!lobbyKeyPickedUp){ addBox(-4.8f,0.9f,-9.9f,0.2f,0.2f,0.05f,0.3f,0.2f,0.1f,false); addBox(-4.72f,0.75f,-9.86f,0.035f,0.1f,0.035f,1.0f,0.84f,0.0f,false); }
    }
    int st=pRm-1, en=pRm+2; if(pRm>=seekStartRoom && pRm<=seekStartRoom+2){st=seekStartRoom;en=seekStartRoom+3;} if(st<0)st=0; if(en>TOTAL_ROOMS-1)en=TOTAL_ROOMS-1;
    for(int i=st; i<=en; i++){
        float z=-10-(i*10), L=rooms[i].lightLevel, wL=(i>0)?rooms[i-1].lightLevel:1.0f; bool tAN=(!rooms[i].isSeekChase && !rooms[i].isSeekHallway && !rooms[i].isSeekFinale && i==pRm+2);
        
        bool isInteriorVisible = true;
        if (!seekActive) {
            if (i > pRm && i >= 0 && !doorOpen[i]) isInteriorVisible = false;
            if (i < pRm && i >= 0 && i+1 < TOTAL_ROOMS && !doorOpen[i+1]) isInteriorVisible = false;
        }

        if (i == 49) {
            float cL = rooms[i].lightLevel; globalTintR=0.9f; globalTintG=0.8f; globalTintB=0.6f; 
            addWallWithDoors(z - 20.0f, false, false, true, doorOpen[i], false, false, i, cL);
            if (isInteriorVisible) {
                addTiledSurface(-6.0f, 0.0f, z, 12.0f, 0.01f, -20.0f, floorU,floorV,floorUW,floorVH, 2.0f, cR,cG,cB, cL, false); 
                addTiledSurface(-6.0f, 3.6f, z, 12.0f, 0.01f, -20.0f, floorU,floorV,floorUW,floorVH, 2.0f, cR,cG,cB, cL, false); 
                addTiledSurface(-6.1f, 0.0f, z, 0.1f, 3.6f, -20.0f, wallU,wallV,wallUW,wallVH, 4.0f, cR,cG,cB, cL, true); 
                addTiledSurface(6.0f, 0.0f, z, 0.1f, 3.6f, -20.0f, wallU,wallV,wallUW,wallVH, 4.0f, cR,cG,cB, cL, true);  
                addTiledSurface(-6.0f, 0.0f, z, 4.6f, 3.6f, -0.1f, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, cL, true); 
                addTiledSurface(1.4f, 0.0f, z, 4.6f, 3.6f, -0.1f, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, cL, true);
                addTiledSurface(-1.4f, 1.8f, z, 2.8f, 1.8f, -0.1f, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, cL, true); 
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
                addTiledSurface(-2.5f, 0.0f, z-12.0f, 1.0f, 1.8f, -4.0f, wallU,wallV,wallUW,wallVH, 1.0f, cR,cG,cB, cL, true);
                addTiledSurface(1.5f, 0.0f, z-12.0f, 1.0f, 1.8f, -4.0f, wallU,wallV,wallUW,wallVH, 1.0f, cR,cG,cB, cL, true);
            }
            globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f; continue; 
        }

        if(seekState==1){globalTintR=1.0f;globalTintG=0.2f;globalTintB=0.2f;} else{globalTintR=1.0f;globalTintG=1.0f;globalTintB=1.0f;}
        if(i==seekStartRoom+9 && rooms[i].isLocked) addBox(-3.0f,0,z+0.2f,6.0f,1.8f,0.1f,0.4f,0.7f,1.0f,true,0,1.5f);
        if(!(i==seekStartRoom+1 || i==seekStartRoom+2)){ if(rooms[i].isDupeRoom){ if(pRm>=i) addWallWithDoors(z,(rooms[i].correctDupePos==0),((rooms[i].correctDupePos==0)&&doorOpen[i]),(rooms[i].correctDupePos==1),((rooms[i].correctDupePos==1)&&doorOpen[i]),(rooms[i].correctDupePos==2),((rooms[i].correctDupePos==2)&&doorOpen[i]),i,wL); else addWallWithDoors(z,true,(rooms[i].correctDupePos==0&&doorOpen[i]),true,(rooms[i].correctDupePos==1&&doorOpen[i]),true,(rooms[i].correctDupePos==2&&doorOpen[i]),i,wL); } else addWallWithDoors(z,(rooms[i].doorPos==0),((rooms[i].doorPos==0)&&doorOpen[i]),(rooms[i].doorPos==1),((rooms[i].doorPos==1)&&doorOpen[i]),(rooms[i].doorPos==2),((rooms[i].doorPos==2)&&doorOpen[i]),i,wL); }
        
        bool rE=!(rooms[i].isSeekChase||rooms[i].hasSeekEyes)||(i>=pRm && i<=pRm+1);

        if(seekState==1){globalTintR=1.0f;globalTintG=0.2f;globalTintB=0.2f;} else if(rooms[i].hasEyes){globalTintR=0.8f;globalTintG=0.3f;globalTintB=1.0f;} else{globalTintR=1.0f;globalTintG=1.0f;globalTintB=1.0f;}
        
        if (isInteriorVisible) {
            if(rE && (rooms[i].hasSeekEyes||rooms[i].isSeekChase)){ srand(i*12345); int wC=rooms[i].hasSeekEyes?rooms[i].seekEyeCount:15; for(int e=0;e<wC;e++){ bool iL=(rand()%2==0); float eZ=z-0.5f-(rand()%90)/10.0f, eY=0.2f+(rand()%160)/100.0f; if(iL){ addBox(-2.95f,eY,eZ,0.1f,0.3f,0.4f,0.05f,0.05f,0.05f,false,0,L); addBox(-2.88f,eY+0.05f,eZ+0.05f,0.06f,0.2f,0.3f,0.95f,0.95f,0.95f,false,0,L); addBox(-2.84f,eY+0.1f,eZ+0.12f,0.04f,0.1f,0.16f,0,0,0,false,0,1.5f); } else{ addBox(2.85f,eY,eZ,0.1f,0.3f,0.4f,0.05f,0.05f,0.05f,false,0,L); addBox(2.82f,eY+0.05f,eZ+0.05f,0.06f,0.2f,0.3f,0.95f,0.95f,0.95f,false,0,L); addBox(2.80f,eY+0.1f,eZ+0.12f,0.04f,0.1f,0.16f,0,0,0,false,0,1.5f); } } srand(time(NULL)); }
            if(rooms[i].hasEyes && rE){ float ex=rooms[i].eyesX, ey=rooms[i].eyesY, ez=rooms[i].eyesZ; addBox(ex-0.2f,ey-0.2f,ez-0.2f,0.4f,0.4f,0.4f,0.6f,0.1f,0.8f,false,0,L); addBox(ex-0.25f,ey-0.1f,ez-0.1f,0.5f,0.2f,0.2f,0.9f,0.9f,0.9f,false,0,L); addBox(ex-0.1f,ey-0.25f,ez-0.1f,0.2f,0.5f,0.2f,0.9f,0.9f,0.9f,false,0,L); addBox(ex-0.26f,ey-0.05f,ez-0.05f,0.02f,0.1f,0.1f,0,0,0,false,0,1.5f); addBox(ex+0.24f,ey-0.05f,ez-0.05f,0.02f,0.1f,0.1f,0,0,0,false,0,1.5f); } 
        }

        if(rooms[i].isSeekChase){ srand(i*777); int oT=rand()%3; float oZ=z-5.0f; if(oT==0) addBox(-3,0.7f,oZ,6,1.1f,0.4f,0.2f,0.15f,0.1f,true,0,L); else if(oT==1){addBox(-3,0,oZ,3,1.8f,0.4f,0.2f,0.15f,0.1f,true,0,L); addBox(0,0.7f,oZ,3,1.1f,0.4f,0.2f,0.15f,0.1f,true,0,L);} else{addBox(0,0,oZ,3,1.8f,0.4f,0.2f,0.15f,0.1f,true,0,L); addBox(-3,0.7f,oZ,3,1.1f,0.4f,0.2f,0.15f,0.1f,true,0,L);} srand(time(NULL)); } 
        else if(rooms[i].isSeekFinale){ addBox(-2.95f,0.4f,z-8.5f,0.1f,1.0f,7.0f,0.4f,0.7f,1.0f,false,0,L); addBox(2.85f,0.4f,z-8.5f,0.1f,1.0f,7.0f,0.4f,0.7f,1.0f,false,0,L); addBox(-3,0,z-2,3.5f,1.8f,0.4f,0.05f,0.05f,0.05f,true,0,L); addBox(-0.1f,0.5f,z-2.1f,0.6f,0.6f,0.6f,1,0,0,false,0,1.5f); rooms[i].pW[0]=2.6f; rooms[i].pZ[0]=z-2.0f; rooms[i].pSide[0]=1; addBox(2,0.8f,z-2.2f,1.0f,0.2f,0.4f,0.05f,0.05f,0.05f,false,0,L); rooms[i].pW[1]=1.8f; rooms[i].pZ[1]=z-3.5f; rooms[i].pSide[1]=0; addBox(1.4f,0,z-3.9f,0.8f,0.3f,0.8f,1,0.4f,0,false,0,L); addBox(1.6f,0.3f,z-3.7f,0.4f,0.4f,0.4f,1,0.8f,0,false,0,L); addBox(-0.5f,0,z-5,3.5f,1.8f,0.4f,0.05f,0.05f,0.05f,true,0,L); addBox(-0.5f,0.5f,z-5.1f,0.6f,0.6f,0.6f,1,0,0,false,0,1.5f); rooms[i].pW[2]=-2.6f; rooms[i].pZ[2]=z-5.0f; rooms[i].pSide[2]=1; addBox(-3,0.8f,z-5.2f,1.0f,0.2f,0.4f,0.05f,0.05f,0.05f,false,0,L); rooms[i].pW[3]=-1.8f; rooms[i].pZ[3]=z-6.5f; rooms[i].pSide[3]=0; addBox(-2.2f,0,z-6.9f,0.8f,0.3f,0.8f,1,0.4f,0,false,0,L); addBox(-2.0f,0.3f,z-6.7f,0.4f,0.4f,0.4f,1,0.8f,0,false,0,L); addBox(-3,0,z-8,3.5f,1.8f,0.4f,0.05f,0.05f,0.05f,true,0,L); addBox(-0.1f,0.5f,z-8.1f,0.6f,0.6f,0.6f,1,0,0,false,0,1.5f); rooms[i].pW[4]=2.6f; rooms[i].pZ[4]=z-8.0f; rooms[i].pSide[4]=1; addBox(2,0.8f,z-8.2f,1.0f,0.2f,0.4f,0.05f,0.05f,0.05f,false,0,L); rooms[i].pW[5]=0.8f; rooms[i].pZ[5]=z-9.0f; rooms[i].pSide[5]=0; addBox(0.4f,0,z-9.4f,0.8f,0.3f,0.8f,1,0.4f,0,false,0,L); addBox(0.6f,0.3f,z-9.2f,0.4f,0.4f,0.4f,1,0.8f,0,false,0,L); } 
        else if(rooms[i].isSeekHallway){ addBox(-2.95f,0.4f,z-8.5f,0.1f,1.0f,7.0f,0.4f,0.7f,1.0f,false,0,L); addBox(2.85f,0.4f,z-8.5f,0.1f,1.0f,7.0f,0.4f,0.7f,1.0f,false,0,L); } 
        
        addTiledSurface(-3, 0, z, 6, 0.01f, -10, floorU,floorV,floorUW,floorVH, 2.0f, cR,cG,cB, L, false); // Floor
        addTiledSurface(-3, 1.8f, z, 6, 0.01f, -10, floorU,floorV,floorUW,floorVH, 2.0f, cR,cG,cB, L, false); // Ceiling
        
        auto drawSide = [&](bool isL) {
            float dZ=z+(isL?rooms[i].leftDoorOffset:rooms[i].rightDoorOffset), bL=fabsf(dZ-z), aL=fabsf((z-10.0f)-(dZ-1.2f)), m=(isL?-1.0f:1.0f), bX=isL?-3.0f:2.9f, dO=isL?rooms[i].leftDoorOpen:rooms[i].rightDoorOpen;
            if(bL>0.05f) { addTiledSurface(bX, 0.4f, z, 0.1f, 1.4f, -bL, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L, true); addTiledSurface(isL?-3.02f:2.92f, 0.0f, z, 0.12f, 0.4f, -bL, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L, false); }
            if(aL>0.05f) { addTiledSurface(bX, 0.4f, dZ-1.2f, 0.1f, 1.4f, -aL, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L, true); addTiledSurface(isL?-3.02f:2.92f, 0.0f, dZ-1.2f, 0.12f, 0.4f, -aL, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L, false); }
            addTiledSurface(bX, 1.4f, dZ, 0.1f, 0.4f, -1.2f, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L, false);
            addBox(bX-(isL?-0.05f:0.0f),0,dZ,0.05f,1.4f,-0.05f,0.15f,0.1f,0.05f,false,0,L); addBox(bX-(isL?-0.05f:0.0f),0,dZ-1.15f,0.05f,1.4f,-0.05f,0.15f,0.1f,0.05f,false,0,L); addBox(bX-(isL?-0.05f:0.0f),1.35f,dZ,0.05f,0.05f,-1.2f,0.15f,0.1f,0.05f,false,0,L); 
            
            if(dO){ addBox(isL?-4.1f:3.0f,0,dZ-1.15f,1.1f,1.4f,0.05f,0.12f,0.06f,0.03f,true,0,L); addBox(isL?-4.0f:3.9f,0.7f,dZ-1.10f,0.1f,0.05f,0.05f,0.8f,0.7f,0.2f,false,0,L); collisions.push_back({isL?-4.1f:3.0f,0.0f,dZ-1.15f,isL?-3.0f:4.1f,1.8f,dZ-1.10f,4}); } else{ addBox(isL?-3.0f:2.95f,0,dZ-0.05f,0.05f,1.4f,-1.1f,0.12f,0.06f,0.03f,true,0,L); addBox(isL?-2.95f:2.9f,0.7f,dZ-0.15f,0.05f,0.1f,-0.1f,0.8f,0.7f,0.2f,false,0,L); } 
            
            float srZ=dZ+2.5f, sW=isL?-9.0f:3.0f; 
            addTiledSurface(sW,0,srZ,6.0f,0.01f,-5.0f, floorU,floorV,floorUW,floorVH, 2.0f, cR,cG,cB, L, false); 
            addTiledSurface(sW,1.8f,srZ,6.0f,0.01f,-5.0f, floorU,floorV,floorUW,floorVH, 2.0f, cR,cG,cB, L, false); 
            addTiledSurface(isL?-9.0f:8.9f,0.4f,srZ,0.1f,1.4f,-5.0f,wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L,true);
            addTiledSurface(isL?-9.02f:8.88f,0.0f,srZ,0.12f,0.4f,-5.0f,wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L,false);
            addTiledSurface(sW,0.4f,srZ,6.0f,1.4f,0.1f,wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L,true);
            addTiledSurface(sW,0.0f,srZ,6.0f,0.4f,0.12f,wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L,false);
            addTiledSurface(sW,0.4f,srZ-5.0f,6.0f,1.4f,-0.1f,wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L,true);
            addTiledSurface(sW,0.0f,srZ-5.0f,6.0f,0.4f,-0.12f,wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L,false);
            
            if (i == pRm && dO) {
                srand(i*(isL?123:321)); 
                if(rand()%2==0){ 
                    float pY=0.6f+(rand()%50)/100.0f, pZ=srZ-1.5f-(rand()%20)/10.0f, pW=0.5f+(rand()%40)/100.0f, pH=0.5f+(rand()%40)/100.0f; 
                    addBox(isL?-8.95f:8.89f, pY-0.05f, pZ+0.05f, 0.06f, pH+0.1f, -pW-0.1f, 0.15f, 0.1f, 0.05f, false, 0, L); 
                    addBox(isL?-8.9f:8.83f, pY, pZ, 0.07f, pH, -pW, 0.8f, 0.8f, 0.8f, false, 0, L); 
                } 
                srand(time(NULL));
                
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
        
        if(rooms[i].hasLeftRoom) drawSide(true); else { addTiledSurface(-3.0f, 0.4f, z, 0.1f, 1.4f, -10.0f, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L, true); addTiledSurface(-3.02f, 0.0f, z, 0.12f, 0.4f, -10.0f, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L, false); }
        if(rooms[i].hasRightRoom) drawSide(false); else { addTiledSurface(2.9f, 0.4f, z, 0.1f, 1.4f, -10.0f, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L, true); addTiledSurface(2.88f, 0.0f, z, 0.12f, 0.4f, -10.0f, wallU,wallV,wallUW,wallVH, 2.0f, cR,cG,cB, L, false); } 
        addBox(-0.4f,1.78f,z-5.4f,0.8f,0.02f,0.8f,(L>0.5f?0.9f:0.2f),(L>0.5f?0.9f:0.2f),(L>0.5f?0.8f:0.2f),false);
        
        if(isInteriorVisible && !tAN){
            for(int s=0;s<3;s++){ float zC=z-2.5f-(s*2.5f); int t=rooms[i].slotType[s]; if(t==1)buildCabinet(zC,true,L); else if(t==2)buildCabinet(zC,false,L); else if(t==5)buildDresser(zC,true,rooms[i].animMain[s],rooms[i].slotItem[s],L); else if(t==6)buildDresser(zC,false,rooms[i].animMain[s],rooms[i].slotItem[s],L); }
            for(int p=0;p<rooms[i].pCount;p++){ 
                float pZ=rooms[i].pZ[p],pH=rooms[i].pH[p],pW=rooms[i].pW[p],pY=rooms[i].pY[p]; 
                float wX=rooms[i].pSide[p]==0?-2.95f:2.89f, cX=rooms[i].pSide[p]==0?-2.88f:2.82f; 
                float pR=rooms[i].pR[p], pG=rooms[i].pG[p], pB=rooms[i].pB[p];
                addBox(wX, pY-0.05f, z-pZ+0.05f, 0.06f, pH+0.1f, -pW-0.1f, 0.15f, 0.1f, 0.05f, false, 0, L); 
                addBox(cX, pY, z-pZ, 0.07f, pH, -pW, pR, pG, pB, false, 0, L); 
            }
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
            rooms[i].hasLeftRoom = (rand()%100 < 30);
            rooms[i].hasRightRoom = (rand()%100 < 30);
            
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
    rooms[0].doorPos=1; rooms[0].isDupeRoom=false; rooms[0].isLocked=true; rooms[0].isJammed=false; rooms[0].hasEyes=false; rooms[48].doorPos=1; rooms[49].doorPos=1; 
    for(int i=2; i<TOTAL_ROOMS-1; i++){
        if(!rooms[i].isDupeRoom&&!rooms[i-1].isDupeRoom&&!(i>=seekStartRoom&&i<=seekStartRoom+9)&&!((i-1)>=seekStartRoom&&(i-1)<=seekStartRoom+9)&&(rand()%3==0)){ rooms[i].isLocked=true; struct SlotLoc{int type;int index;}; std::vector<SlotLoc> vS; for(int s=0;s<3;s++)if(rooms[i-1].slotType[s]>2)vS.push_back({0,s}); if(rooms[i-1].hasLeftRoom)for(int s=0;s<3;s++){if(rooms[i-1].leftRoomSlotTypeL[s]>=3&&rooms[i-1].leftRoomSlotTypeL[s]<=6)vS.push_back({1,s}); if(rooms[i-1].leftRoomSlotTypeR[s]>=3&&rooms[i-1].leftRoomSlotTypeR[s]<=6)vS.push_back({2,s});} if(rooms[i-1].hasRightRoom)for(int s=0;s<3;s++){if(rooms[i-1].rightRoomSlotTypeL[s]>=3&&rooms[i-1].rightRoomSlotTypeL[s]<=6)vS.push_back({3,s}); if(rooms[i-1].rightRoomSlotTypeR[s]>=3&&rooms[i-1].rightRoomSlotTypeR[s]<=6)vS.push_back({4,s});} if(!vS.empty()){ SlotLoc c=vS[rand()%vS.size()]; if(c.type==0)rooms[i-1].slotItem[c.index]=1; else if(c.type==1)rooms[i-1].leftRoomSlotItemL[c.index]=1; else if(c.type==2)rooms[i-1].leftRoomSlotItemR[c.index]=1; else if(c.type==3)rooms[i-1].rightRoomSlotItemL[c.index]=1; else if(c.type==4)rooms[i-1].rightRoomSlotItemR[c.index]=1; } else{rooms[i-1].slotType[1]=5;rooms[i-1].slotItem[1]=1;} }
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
    
    hasAtlas = loadTextureFromFile("romfs:/atlas.t3x", &atlasTex);
    
    generateRooms(); int currentChunk=0, playerCurrentRoom=-1; buildWorld(currentChunk, playerCurrentRoom);
    DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size); shaderProgram_s program; shaderProgramInit(&program); shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]); C3D_BindProgram(&program);
    
    int uLoc_proj = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx"); C3D_AttrInfo* attr = C3D_GetAttrInfo(); AttrInfo_Init(attr); 
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 4); 
    AttrInfo_AddLoader(attr, 1, GPU_FLOAT, 2); 
    AttrInfo_AddLoader(attr, 2, GPU_FLOAT, 4); 
    
    void* vbo_main = linearAlloc(MAX_VERTS * sizeof(vertex)); 
    
    int colored_size = world_mesh_colored.size();
    int textured_size = world_mesh_textured.size();
    memcpy(vbo_main, world_mesh_colored.data(), colored_size * sizeof(vertex)); 
    memcpy((vertex*)vbo_main + colored_size, world_mesh_textured.data(), textured_size * sizeof(vertex)); 
    GSPGPU_FlushDataCache(vbo_main, (colored_size + textured_size) * sizeof(vertex));
    
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL); C3D_CullFace(GPU_CULL_NONE); 

    float camX=0, camZ=7.5f, camYaw=0, camPitch=0; const char symbols[] = "@!$#&*%?"; static float startTouchX=0, startTouchY=0; static bool wasTouching=false;
    static int lastRoomForDarkCheck = -1;

    u64 lastTime = osGetTime();
    int frames = 0;
    float currentFps = 0.0f;

    while (aptMainLoop()) {
        u64 currTime = osGetTime();
        frames++;
        if (currTime - lastTime >= 1000) {
            currentFps = frames / ((currTime - lastTime) / 1000.0f);
            frames = 0;
            lastTime = currTime;
        }

        hidScanInput(); irrstScanInput(); u32 kDown=hidKeysDown(), kHeld=hidKeysHeld(); if(kDown&KEY_SELECT)break;
        bool needsVBOUpdate=false; static bool deathSoundPlayed=false; static u64 totalFrames = 0; totalFrames++;

        if(isDead){ if(!deathSoundPlayed){ if(audio_ok){ ndspChnWaveBufClear(3); ndspChnWaveBufClear(5); ndspChnWaveBufClear(7); ndspChnWaveBufClear(9); bool wAA=(sAttack.status==NDSP_WBUF_PLAYING||sAttack.status==NDSP_WBUF_QUEUED||sDupeAttack.status==NDSP_WBUF_PLAYING||sDupeAttack.status==NDSP_WBUF_QUEUED); if(!wAA){ ndspChnWaveBufClear(8); if(sDeath.data_vaddr){sDeath.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(8,&sDeath);} deathSoundPlayed=true; } } else deathSoundPlayed=true; } } else deathSoundPlayed=false;
        if(kDown&KEY_START){ if(isDead){ isDead=false;hasKey=false;lobbyKeyPickedUp=false;isCrouching=false;hideState=NOT_HIDING;playerHealth=100;screechActive=false;flashRedFrames=0;playerCoins=0;screechCooldown=1800;rushActive=false;rushState=0;rushCooldown=0;messageTimer=0;inElevator=true;elevatorTimer=1593;elevatorDoorsOpen=false;elevatorClosing=false;elevatorDoorOffset=0;elevatorJamFinished=false;camX=0;camZ=7.5f;camYaw=0;camPitch=0;currentChunk=0;playerCurrentRoom=-1;lastRoomForDarkCheck=-1;for(int i=0;i<TOTAL_ROOMS;i++)doorOpen[i]=false;seekActive=false;seekState=0;seekTimer=0;eyesSoundCooldown=0;generateRooms();buildWorld(currentChunk,playerCurrentRoom); colored_size=world_mesh_colored.size(); textured_size=world_mesh_textured.size(); memcpy(vbo_main,world_mesh_colored.data(),colored_size*sizeof(vertex)); memcpy((vertex*)vbo_main+colored_size,world_mesh_textured.data(),textured_size*sizeof(vertex)); GSPGPU_FlushDataCache(vbo_main,(colored_size+textured_size)*sizeof(vertex)); if(audio_ok)for(int i=3;i<=12;i++)ndspChnWaveBufClear(i);inEyesRoom=false;isLookingAtEyes=false;eyesDamageTimer=0;eyesDamageAccumulator=0;eyesGraceTimer=0;consoleClear();continue; } }
        
        if(!isDead && (kHeld & KEY_R) && (kHeld & KEY_Y)) {
            if(kDown & KEY_DDOWN) { camZ = -10.0f - ((seekStartRoom - 1) * 10.0f) + 5.0f; camX = 0.0f; camYaw = 0.0f; camPitch = 0.0f; needsVBOUpdate = true; sprintf(uiMessage, "Teleported to Seek!"); messageTimer = 90; }
            if(kDown & KEY_DLEFT) { camZ = -10.0f - (48 * 10.0f) + 5.0f; camX = 0.0f; camYaw = 0.0f; camPitch = 0.0f; needsVBOUpdate = true; sprintf(uiMessage, "Teleported near Library!"); messageTimer = 90; }
        }
        
        playerCurrentRoom=(camZ>=-10.0f)?-1:(int)((-camZ-10.0f)/10.0f); if(playerCurrentRoom<-1)playerCurrentRoom=-1; if(playerCurrentRoom>TOTAL_ROOMS-2)playerCurrentRoom=TOTAL_ROOMS-2;
        bool isGlitch=false; int tDR=-1; for(int k=1;k<TOTAL_ROOMS;k++){if(rooms[k].isDupeRoom&&playerCurrentRoom==(k-1)){isGlitch=true;tDR=k;break;}}
        if(messageTimer>0)messageTimer--; if(screechCooldown>0)screechCooldown--; if(rushCooldown>0)rushCooldown--; if(eyesSoundCooldown>0)eyesSoundCooldown--;

        if(playerCurrentRoom != lastRoomForDarkCheck && playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) { lastRoomForDarkCheck = playerCurrentRoom; if(rooms[playerCurrentRoom].lightLevel < 0.5f && audio_ok && sDarkRoomEnter.data_vaddr){ float m[12]={0}; m[0]=0.3f; m[1]=0.3f; ndspChnSetMix(11,m); ndspChnWaveBufClear(11); sDarkRoomEnter.status=NDSP_WBUF_FREE; ndspChnWaveBufAdd(11,&sDarkRoomEnter); } }

        if(inElevator&&!elevatorDoorsOpen){ if(elevatorTimer==1593&&audio_ok&&sElevatorJam.data_vaddr){ndspChnWaveBufClear(9);sElevatorJam.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(9,&sElevatorJam);} if(elevatorTimer>0)elevatorTimer--; bool tTO=false; if(audio_ok&&sElevatorJam.data_vaddr){if(elevatorTimer<1590&&sElevatorJam.status==NDSP_WBUF_DONE&&!elevatorJamFinished)tTO=true;}else if(elevatorTimer<=0&&!elevatorJamFinished)tTO=true; if(tTO){elevatorJamFinished=true;elevatorDoorsOpen=true;needsVBOUpdate=true;if(audio_ok){ndspChnWaveBufClear(9);if(sElevatorJamEnd.data_vaddr){sElevatorJamEnd.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(9,&sElevatorJamEnd);}}} }
        if(elevatorDoorsOpen&&!elevatorClosing&&elevatorDoorOffset<2.0f){elevatorDoorOffset+=0.03f;needsVBOUpdate=true;} if(elevatorDoorsOpen&&!elevatorClosing&&camZ<2.0f){inElevator=false;elevatorClosing=true;needsVBOUpdate=true;} if(elevatorClosing&&elevatorDoorOffset>0.0f){elevatorDoorOffset-=0.04f;if(elevatorDoorOffset<=0.0f)elevatorDoorOffset=0.0f;needsVBOUpdate=true;}

        bool animActive = false;
        if(totalFrames % 2 == 0) {
            if(playerCurrentRoom>=0 && playerCurrentRoom<TOTAL_ROOMS) {
                auto stepAnim = [&](bool target, float& val) {
                    if(target && val < 1.0f) { val += 0.10f; if(val>1.0f) val=1.0f; return true; }
                    if(!target && val > 0.0f) { val -= 0.10f; if(val<0.0f) val=0.0f; return true; }
                    return false;
                };
                for(int s=0; s<3; s++) {
                    if(stepAnim(rooms[playerCurrentRoom].drawerOpen[s], rooms[playerCurrentRoom].animMain[s])) animActive = true;
                    if(rooms[playerCurrentRoom].hasLeftRoom) {
                        if(stepAnim(rooms[playerCurrentRoom].leftRoomDrawerOpenL[s], rooms[playerCurrentRoom].animLL[s])) animActive = true;
                        if(stepAnim(rooms[playerCurrentRoom].leftRoomDrawerOpenR[s], rooms[playerCurrentRoom].animLR[s])) animActive = true;
                    }
                    if(rooms[playerCurrentRoom].hasRightRoom) {
                        if(stepAnim(rooms[playerCurrentRoom].rightRoomDrawerOpenL[s], rooms[playerCurrentRoom].animRL[s])) animActive = true;
                        if(stepAnim(rooms[playerCurrentRoom].rightRoomDrawerOpenR[s], rooms[playerCurrentRoom].animRR[s])) animActive = true;
                    }
                }
            }
        }
        if(animActive) needsVBOUpdate = true;

        static bool prevScreech=false; if(screechActive!=prevScreech){consoleClear();prevScreech=screechActive;}
        printf("\x1b[1;1H==============================\n");
        if(isDead){ printf("         YOU DIED!            \n==============================\n\n                              \n\n\n    [PRESS START TO RESTART]  \n\x1b[0J"); } 
        else if(screechActive){ printf("  >> SCREECH ATTACK!! <<      \n\n     (PSST!)                  \n    LOOK AROUND QUICKLY!      \n\n\x1b[0J"); } 
        else {
            printf("        PLAYER STATUS         \n==============================\n\n"); int dC=getDisplayRoom(playerCurrentRoom), nD=getNextDoorIndex(playerCurrentRoom), dN=getDisplayRoom(nD);
            if(playerCurrentRoom==-1){ printf(" Current Room : 000 (Lobby) \x1b[K\n"); if(isGlitch){char g2[4];for(int i=0;i<3;i++)g2[i]=symbols[rand()%8];g2[3]='\0';printf(" Next Door     : %s         \x1b[K\n",g2);}else printf(" Next Door     : 001         \x1b[K\n"); printf("                            \x1b[K\n\n"); } else if(isGlitch){ char g1[4],g2[4];for(int i=0;i<3;i++){g1[i]=symbols[rand()%8];g2[i]=symbols[rand()%8];}g1[3]='\0';g2[3]='\0'; printf(" Current Room : %s         \x1b[K\n Next Door     : %s         \x1b[K\n                            \x1b[K\n\n",g1,g2); } else printf(" Current Room : %03d         \x1b[K\n Next Door     : %03d         \x1b[K\n                            \x1b[K\n\n",dC,dN);
            if(nD>=0&&nD<TOTAL_ROOMS&&fabsf(camZ-(-10.0f-(nD*10.0f)))<4.0f&&fabsf(camX)<2.0f){ if(isGlitch&&tDR==nD){ if(camX<-1.4f)printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n",rooms[tDR].dupeNumbers[0]); else if(camX>=-1.4f&&camX<=0.6f)printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n",rooms[tDR].dupeNumbers[1]); else printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n",rooms[tDR].dupeNumbers[2]); } else printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n",dN); } else printf("                           \x1b[K\n\n");
            
            printf(" Health       : %d / 100   \x1b[K\n Golden Key   : %s         \x1b[K\n Coins        : %04d       \x1b[K\n FPS          : %.2f       \x1b[K\n\n        --- CONTROLS ---      \x1b[K\n [A] Interact  [B] Crouch    \x1b[K\n [X] Hide(Cab/Bed) [CPAD] Move \x1b[K\n [TOUCH/CSTICK] Look Around  \x1b[K\n", playerHealth, hasKey?"EQUIPPED":"None    ", playerCoins, currentFps);
            
            if(texErrorMessage[0] != '\0') printf("\n \x1b[31m[TEX ERROR] %s\x1b[0m \x1b[K\n", texErrorMessage);

            if(messageTimer>0)printf("\n ** %s ** \x1b[K\n",uiMessage);else if(rushActive&&rushState==1)printf("\n ** The lights are flickering... ** \x1b[K\n");else if(hideState==BEHIND_DOOR)printf("\n ** Hiding behind door... ** \x1b[K\n");else printf("\n                                    \x1b[K\n");
            if(!audio_ok)printf("\x1b[31m WARNING: dspfirm.cdc MISSING!\x1b[0m\x1b[K\n\x1b[31m Sound chip could not turn on.\x1b[0m\x1b[K\n"); else printf("                                    \x1b[K\n"); printf("\x1b[0J");
        }

        if(!isDead){
            if(playerCurrentRoom == 49 && !figureActive) { figureActive = true; figureZ = -10.0f - (49 * 10.0f) - 5.0f; needsVBOUpdate = true; sprintf(uiMessage, "Shh... He can hear you."); messageTimer = 150; }
            if(figureActive) {
                float distToPlayer = sqrt(pow(camX - figureX, 2) + pow(camZ - figureZ, 2));
                if(figureState == 0) {
                    if(!isCrouching && distToPlayer < 4.0f) figureState = 2;
                } else if(figureState == 2) {
                    if(camX > figureX) figureX += figureSpeed; else figureX -= figureSpeed;
                    if(camZ > figureZ) figureZ += figureSpeed; else figureZ -= figureSpeed;
                    if(distToPlayer < 0.8f) { playerHealth = 0; isDead = true; flashRedFrames = 50; sprintf(uiMessage, "The Figure found you..."); messageTimer = 120; }
                    needsVBOUpdate = true;
                }
            }
            
            if(playerCurrentRoom>=seekStartRoom&&playerCurrentRoom<=seekStartRoom+2){ if(camZ<-10.0f-((seekStartRoom+2)*10.0f)-8.0f&&seekState==0){seekState=1;seekActive=true;seekTimer=0;seekSpeed=0.0f;seekZ=-10.0f-(seekStartRoom*10.0f);needsVBOUpdate=true;if(audio_ok&&sSeekRise.data_vaddr){ndspChnWaveBufClear(7);sSeekRise.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(7,&sSeekRise);}} }
            if(seekState==1){ seekTimer++;needsVBOUpdate=true; if(seekTimer>=180&&seekTimer<230){if(seekSpeed<0.12f)seekSpeed+=0.005f;seekZ-=seekSpeed;} if(seekTimer>=230){seekState=2;seekSpeed=0;sprintf(uiMessage,"RUN!");messageTimer=90;flashRedFrames=10;if(audio_ok&&sSeekChase.data_vaddr){ndspChnWaveBufClear(7);sSeekChase.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(7,&sSeekChase);}} } 
            else if(seekState==2){ 
                seekSpeed=(seekZ>-10.0f-((seekStartRoom+2)*10.0f))?0.065f:seekMaxSpeed; seekZ-=seekSpeed;needsVBOUpdate=true; 
                if(audio_ok && sSeekChase.data_vaddr) { float mix[12] = {0}; mix[0] = 0.8f; mix[1] = 0.8f; ndspChnSetMix(7, mix); } 
                if(fabsf(seekZ-camZ)<1.2f){playerHealth=0;isDead=true;flashRedFrames=50;sprintf(uiMessage,"Seek caught you...");messageTimer=120;if(audio_ok)ndspChnWaveBufClear(7);} if(playerCurrentRoom>=0&&rooms[playerCurrentRoom].isSeekFinale){for(int h=0;h<6;h++){if(fabsf(camX-rooms[playerCurrentRoom].pW[h])<0.6f&&fabsf(camZ-rooms[playerCurrentRoom].pZ[h])<0.6f){if(rooms[playerCurrentRoom].pSide[h]==0&&!isDead&&messageTimer<=0){playerHealth-=40;flashRedFrames=25;camZ+=1.5f;sprintf(uiMessage,"Burned! (-40 HP)");messageTimer=60;if(playerHealth<=0){isDead=true;if(audio_ok)ndspChnWaveBufClear(7);}}else if(rooms[playerCurrentRoom].pSide[h]==1&&!isDead&&!isCrouching){playerHealth=0;isDead=true;flashRedFrames=50;sprintf(uiMessage,"The hands grabbed you!");messageTimer=120;if(audio_ok)ndspChnWaveBufClear(7);}}}} if(seekActive){ float fLZ=-10.0f-((seekStartRoom+8)*10.0f)-10.0f; int sR=seekStartRoom+9; bool pS=(camZ<fLZ-1.5f); if(pS||seekZ<fLZ+3.0f){ if(!rooms[sR].isJammed){doorOpen[sR]=false;rooms[sR].isLocked=false;rooms[sR].isJammed=true;needsVBOUpdate=true;if(audio_ok){if(sDoor.data_vaddr){ndspChnWaveBufClear(1);sDoor.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(1,&sDoor);}ndspChnWaveBufClear(7);if(sSeekEscaped.data_vaddr){sSeekEscaped.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(7,&sSeekEscaped);}}} if(pS){seekActive=false;seekState=0;sprintf(uiMessage,"You escaped!");messageTimer=150;} else{playerHealth=0;isDead=true;flashRedFrames=50;sprintf(uiMessage,"The door slammed shut!");messageTimer=120;seekActive=false;seekState=0;if(audio_ok)ndspChnWaveBufClear(7);} } } 
            }

            bool inE=(playerCurrentRoom>=0&&playerCurrentRoom<TOTAL_ROOMS&&rooms[playerCurrentRoom].hasEyes); if(inE&&!inEyesRoom){ inEyesRoom=true;eyesGraceTimer=60; if(audio_ok){ndspChnWaveBufClear(4);if(sEyesAppear.data_vaddr){sEyesAppear.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(4,&sEyesAppear);}ndspChnWaveBufClear(5);if(sEyesGarble.data_vaddr){float m[12]={0};m[0]=1.8f;m[1]=1.8f;ndspChnSetMix(5,m);sEyesGarble.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(5,&sEyesGarble);}} } else if(!inE&&inEyesRoom){inEyesRoom=false;if(audio_ok)ndspChnWaveBufClear(5);} if(inE)if(eyesGraceTimer>0)eyesGraceTimer--;
            if(inE&&hideState==NOT_HIDING){ float vx=rooms[playerCurrentRoom].eyesX-camX, vy=rooms[playerCurrentRoom].eyesY-(isCrouching?0.4f:0.9f), vz=rooms[playerCurrentRoom].eyesZ-camZ, dist=sqrt(vx*vx+vy*vy+vz*vz); if(dist>0){vx/=dist;vy/=dist;vz/=dist;} float fx=-sinf(camYaw)*cosf(camPitch), fy=sinf(camPitch), fz=-cosf(camYaw)*cosf(camPitch), dP=(fx*vx)+(fy*vy)+(fz*vz); if(dP>0.85f&&checkLineOfSight(camX,(isCrouching?0.4f:0.9f),camZ,rooms[playerCurrentRoom].eyesX,rooms[playerCurrentRoom].eyesY,rooms[playerCurrentRoom].eyesZ)){ if(eyesGraceTimer<=0){ if(!isLookingAtEyes){isLookingAtEyes=true;eyesDamageTimer=5;eyesDamageAccumulator=4;if(audio_ok&&sEyesAttack.data_vaddr&&eyesSoundCooldown<=0){ndspChnWaveBufClear(4);sEyesAttack.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(4,&sEyesAttack);eyesSoundCooldown=90;}} if(audio_ok&&sEyesAttack.data_vaddr&&(sEyesAttack.status==NDSP_WBUF_DONE||sEyesAttack.status==NDSP_WBUF_FREE)&&eyesSoundCooldown<=0){ndspChnWaveBufClear(4);sEyesAttack.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(4,&sEyesAttack);eyesSoundCooldown=90;} eyesDamageTimer++; if(eyesDamageTimer>=6){playerHealth-=1;eyesDamageTimer=0;flashRedFrames=2;eyesDamageAccumulator++;if(eyesDamageAccumulator>=5){eyesDamageAccumulator=0;if(audio_ok&&sEyesHit.data_vaddr){ndspChnWaveBufClear(6);sEyesHit.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(6,&sEyesHit);}}} if(playerHealth<=0){isDead=true;sprintf(uiMessage,"You stared at Eyes!");messageTimer=120;} } }else{isLookingAtEyes=false;eyesDamageTimer=0;eyesDamageAccumulator=0;} } else{isLookingAtEyes=false;eyesDamageTimer=0;eyesDamageAccumulator=0;}

            bool iSE=(playerCurrentRoom>=seekStartRoom-5&&playerCurrentRoom<=seekStartRoom+9); int sC=(playerCurrentRoom>0&&rooms[playerCurrentRoom].lightLevel<0.5f)?400:12000;
            if(!screechActive&&screechCooldown<=0&&hideState==NOT_HIDING&&playerCurrentRoom>0&&!iSE&&(rand()%sC==0)){ 
                screechActive=true; screechTimer=240;
                float aO=1.57f+((rand()%200)/100.0f)*1.57f, sY=camYaw+aO; 
                screechOffsetX=-sinf(sY)*2.0f; 
                screechOffsetZ=-cosf(sY)*2.0f; 
                screechOffsetY=(rand()%8)/10.0f; 
                if(audio_ok){ndspChnWaveBufClear(0);if(sPsst.data_vaddr){sPsst.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(0,&sPsst);}} 
            }
            
            if(screechActive){ 
                screechTimer--; 
                float cOX = screechOffsetX, cOZ = screechOffsetZ;
                float pY = isCrouching ? 0.4f : 0.9f;
                float nSX = camX + cOX, nSZ = camZ + cOZ, nSY = 0.8f + screechOffsetY;
                
                int att = 0; float currentDist = 2.0f;
                while( (!checkLineOfSight(camX, pY, camZ, nSX, nSY, nSZ) || checkCollision(nSX, nSY, nSZ, 0.4f)) && att < 10 && currentDist > 1.2f) { 
                    cOX *= 0.85f; cOZ *= 0.85f; currentDist *= 0.85f;
                    nSX = camX + cOX; nSZ = camZ + cOZ; att++; 
                }
                
                if(screechX!=nSX || screechZ!=nSZ){screechX=nSX; screechZ=nSZ; screechY=nSY; needsVBOUpdate=true;} 
                
                float vx=screechX-camX, vy=screechY-(isCrouching?0.4f:0.9f), vz=screechZ-camZ, dist=sqrt(vx*vx+vy*vy+vz*vz); 
                if(dist>0.0f){vx/=dist;vy/=dist;vz/=dist;} 
                float fx=-sinf(camYaw)*cosf(camPitch), fy=sinf(camPitch), fz=-cosf(camYaw)*cosf(camPitch), dP=(fx*vx)+(fy*vy)+(fz*vz); 
                
                if(dP>0.85f){
                    screechActive=false; screechCooldown=1800; needsVBOUpdate=true; sprintf(uiMessage,"Dodged Screech!"); messageTimer=90;
                    if(audio_ok){ndspChnWaveBufClear(0); if(sCaught.data_vaddr){sCaught.status=NDSP_WBUF_FREE; ndspChnWaveBufAdd(0,&sCaught);}}
                } else if(screechTimer<=0){
                    screechActive=false; screechCooldown=1800; needsVBOUpdate=true; playerHealth-=20; flashRedFrames=25; sprintf(uiMessage,"Screech bit you! (-20 HP)"); messageTimer=90;
                    if(playerHealth<=0)isDead=true;
                    if(audio_ok){ndspChnWaveBufClear(0); if(sAttack.data_vaddr){sAttack.status=NDSP_WBUF_FREE; ndspChnWaveBufAdd(0,&sAttack);}}
                } 
            }

            if(rushActive){ if(rushState==1){rushTimer--;if(rushTimer%10==0){if(playerCurrentRoom>=0&&playerCurrentRoom<TOTAL_ROOMS)rooms[playerCurrentRoom].lightLevel=(rand()%2==0)?0.3f:1.0f;needsVBOUpdate=true;}if(rushTimer<=0){rushState=2;rushZ=camZ+40.0f;rushTargetZ=camZ-60.0f;}} else if(rushState==2){rushZ-=0.8f;needsVBOUpdate=true;if(playerCurrentRoom>=0&&playerCurrentRoom<TOTAL_ROOMS){rooms[playerCurrentRoom].lightLevel=0.3f;if(playerCurrentRoom+1<TOTAL_ROOMS)rooms[playerCurrentRoom+1].lightLevel=0.3f;}if(fabsf(rushZ-camZ)<3.0f&&fabsf(camX)<3.0f&&hideState==NOT_HIDING){playerHealth=0;isDead=true;flashRedFrames=50;}if(rushZ<rushTargetZ){rushActive=false;rushState=0;needsVBOUpdate=true;rushCooldown=1800;if(audio_ok)ndspChnWaveBufClear(3);}} if(audio_ok&&sRushScream.data_vaddr){float dist=(rushState==1)?40.0f+(rushTimer/rushStartTimer)*110.0f:(fabsf(rushZ-camZ)*(rushZ<camZ?1.5f:1.0f));float vol=1.0f-(dist/150.0f);if(vol<0)vol=0;if(vol>1)vol=1;vol=vol*vol*vol;float mix[12]={0};mix[0]=vol*3.5f;mix[1]=vol*3.5f;ndspChnSetMix(3,mix);} }

            if(playerCurrentRoom>=0&&playerCurrentRoom<TOTAL_ROOMS){ if(rooms[playerCurrentRoom].hasLeftRoom&&!rooms[playerCurrentRoom].leftDoorOpen&&fabsf(camZ-((-10.0f-(playerCurrentRoom*10.0f))+rooms[playerCurrentRoom].leftDoorOffset-0.6f))<2.0f&&camX<-1.5f){rooms[playerCurrentRoom].leftDoorOpen=true;needsVBOUpdate=true;if(audio_ok&&sDoor.data_vaddr){ndspChnWaveBufClear(1);sDoor.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(1,&sDoor);}} if(rooms[playerCurrentRoom].hasRightRoom&&!rooms[playerCurrentRoom].rightDoorOpen&&fabsf(camZ-((-10.0f-(playerCurrentRoom*10.0f))+rooms[playerCurrentRoom].rightDoorOffset-0.6f))<2.0f&&camX>1.5f){rooms[playerCurrentRoom].rightDoorOpen=true;needsVBOUpdate=true;if(audio_ok&&sDoor.data_vaddr){ndspChnWaveBufClear(1);sDoor.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(1,&sDoor);}} }
            if(kDown&KEY_X&&hideState==NOT_HIDING){ float r=0.5f; for(auto& b:collisions){ if((b.type==1||b.type==2)&&camX+r>b.minX&&camX-r<b.maxX&&camZ+r>b.minZ&&camZ-r<b.maxZ){ float rCZ=(b.minZ+b.maxZ)/2.0f, cCX=((b.minX+b.maxX)/2.0f), tOX=(cCX<-4.0f)?-6.0f:((cCX>4.0f)?6.0f:0.0f); if(b.type==1){hideState=IN_CABINET;camZ=rCZ;camPitch=0;camX=(cCX-tOX<0)?-2.5f+tOX:2.5f+tOX;camYaw=(cCX-tOX<0)?-1.57f:1.57f;}else{hideState=UNDER_BED;camZ=rCZ;camPitch=0;camX=(cCX-tOX<0)?-2.2f+tOX:2.2f+tOX;camYaw=(cCX-tOX<0)?-1.57f:1.57f;} isCrouching=false;needsVBOUpdate=true;if(audio_ok&&sWardrobeEnter.data_vaddr){ndspChnWaveBufClear(10);sWardrobeEnter.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(10,&sWardrobeEnter);}break; } } } else if(kDown&KEY_X){ if(hideState==IN_CABINET||hideState==UNDER_BED){camX=(camX<-4.0f)?-6.0f:((camX>4.0f)?6.0f:0.0f);hideState=NOT_HIDING;camYaw=0;needsVBOUpdate=true;if(audio_ok&&sWardrobeExit.data_vaddr){ndspChnWaveBufClear(10);sWardrobeExit.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(10,&sWardrobeExit);}} }

            bool iA=false;
            if(!iA && (kDown&KEY_A)){ 
                int nI=getNextDoorIndex(playerCurrentRoom); 
                if(nI>=0&&nI<TOTAL_ROOMS&&rooms[nI].isDupeRoom){ 
                    float dZ=-10.0f-(nI*10.0f); 
                    if(fabsf(camZ-dZ)<2.5f){ 
                        float fx=-sinf(camYaw), fz=-cosf(camYaw); 
                        int tD=-1; float bD=0.85f; 
                        for(int d=0;d<3;d++){ 
                            float dx=(-2.0f+d*2.0f)-camX, dz=dZ-camZ, dist=sqrt(dx*dx+dz*dz); 
                            if(dist>0&&dist<3.0f){ 
                                float dot=(fx*(dx/dist))+(fz*(dz/dist)); 
                                if(dot>bD){bD=dot;tD=d;} 
                            } 
                        } 
                        if(tD!=-1){ 
                            if(tD==rooms[nI].correctDupePos){ 
                                if(!doorOpen[nI]){if(audio_ok&&sDoor.data_vaddr){ndspChnWaveBufClear(1);sDoor.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(1,&sDoor);}doorOpen[nI]=true;needsVBOUpdate=true;} 
                            } else { 
                                if(audio_ok&&sDupeAttack.data_vaddr){ndspChnWaveBufClear(2);sDupeAttack.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(2,&sDupeAttack);}playerHealth-=34;flashRedFrames=25;camZ+=2.0f;if(playerHealth<=0)isDead=true; 
                            } 
                            iA=true; 
                        } 
                    } 
                } 
            }

            if((kDown&KEY_A)||(kDown&KEY_X&&hideState==NOT_HIDING)){
                auto cI=[&](int ty,float zC,float sX,bool& iO,int& it){ 
                    if(ty!=0){
                        float dx=sX-camX, dz=zC-camZ, dist=sqrt(dx*dx+dz*dz);
                        if(dist<2.5f){
                            float fx=-sinf(camYaw), fz=-cosf(camYaw), dot=(fx*(dx/dist))+(fz*(dz/dist));
                            if(dot>0.7f){
                                float aX=sX-(dx/dist)*0.5f, aZ=zC-(dz/dist)*0.5f;
                                if(checkLineOfSight(camX,(isCrouching?0.4f:0.9f),camZ,aX,0.5f,aZ)){
                                    if(kDown&KEY_X&&(ty==5||ty==6)){iO=!iO;needsVBOUpdate=true;if(audio_ok){ndspChnWaveBufClear(10);if(iO&&sDrawerOpen.data_vaddr){sDrawerOpen.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(10,&sDrawerOpen);}else if(!iO&&sDrawerClose.data_vaddr){sDrawerClose.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(10,&sDrawerClose);}}return true;} 
                                    if(kDown&KEY_A){ 
                                        if((ty==5||ty==6)&&iO){if(it==1){hasKey=true;it=0;needsVBOUpdate=true;sprintf(uiMessage,"Grabbed the Golden Key!");messageTimer=90;return true;}else if(it==2){playerHealth+=10;if(playerHealth>100)playerHealth=100;it=0;needsVBOUpdate=true;sprintf(uiMessage,"Used a Bandaid! (+10 HP)");messageTimer=90;return true;}else if(it==3){playerCoins+=(rand()%15)+5;it=0;needsVBOUpdate=true;sprintf(uiMessage,"Looted some Coins!");messageTimer=90;if(audio_ok&&sCoinsCollect.data_vaddr){ndspChnWaveBufClear(10);sCoinsCollect.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(10,&sCoinsCollect);}return true;}else{sprintf(uiMessage,"Drawer is empty...");messageTimer=60;return true;}}else if((ty==3||ty==4)){if(it==1){hasKey=true;it=0;needsVBOUpdate=true;sprintf(uiMessage,"Grabbed Key off the bed!");messageTimer=90;return true;}else if(it==3){playerCoins+=(rand()%15)+5;it=0;needsVBOUpdate=true;sprintf(uiMessage,"Looted Coins off the bed!");messageTimer=90;if(audio_ok&&sCoinsCollect.data_vaddr){ndspChnWaveBufClear(10);sCoinsCollect.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(10,&sCoinsCollect);}return true;}} 
                                    }
                                }
                            }
                        }
                    } 
                    return false; 
                };
                auto cC=[&](float cX,float cZ,bool& iO){ 
                    if((kDown&KEY_X)&&!iO){
                        float dx=cX-camX, dz=cZ-camZ, dist=sqrt(dx*dx+dz*dz);
                        if(dist<2.5f){
                            float fx=-sinf(camYaw), fz=-cosf(camYaw), dot=(fx*(dx/dist))+(fz*(dz/dist));
                            if(dot>0.7f){
                                float aX=cX-(dx/dist)*0.5f, aZ=cZ-(dz/dist)*0.5f;
                                if(checkLineOfSight(camX,(isCrouching?0.4f:0.9f),camZ,aX,0.3f,aZ)){
                                    iO=true;needsVBOUpdate=true;playerCoins+=20;sprintf(uiMessage,"Opened Chest!");messageTimer=90;if(audio_ok&&sDrawerOpen.data_vaddr){ndspChnWaveBufClear(10);sDrawerOpen.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(10,&sDrawerOpen);}return true;
                                }
                            }
                        }
                    } 
                    return false; 
                };
                
                if(inElevator&&!elevatorDoorsOpen&&camX>0&&camZ>5.0f&&camZ<8.0f){ elevatorJamFinished=true;elevatorDoorsOpen=true;iA=true;needsVBOUpdate=true;if(audio_ok&&sElevatorJamEnd.data_vaddr){ndspChnWaveBufClear(9);sElevatorJamEnd.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(9,&sElevatorJamEnd);} }
                if(!iA&&playerCurrentRoom>=0&&playerCurrentRoom<TOTAL_ROOMS){ for(int s=0;s<3;s++)if(cI(rooms[playerCurrentRoom].slotType[s],(-10.0f-(playerCurrentRoom*10.0f))-2.5f-(s*2.5f),((rooms[playerCurrentRoom].slotType[s]%2!=0)?-2.4f:2.4f),rooms[playerCurrentRoom].drawerOpen[s],rooms[playerCurrentRoom].slotItem[s])){iA=true;break;} if(!iA&&rooms[playerCurrentRoom].hasLeftRoom){ float srZ=(-10.0f-(playerCurrentRoom*10.0f))+rooms[playerCurrentRoom].leftDoorOffset+2.5f; for(int s=0;s<3;s++){float fZ=srZ-0.9f-(s*1.6f);int tL=rooms[playerCurrentRoom].leftRoomSlotTypeL[s];if(tL>0&&tL!=99){if(tL==7||tL==8){if(cC(-8.4f,fZ,rooms[playerCurrentRoom].leftRoomDrawerOpenL[s])){iA=true;break;}}else if(cI(tL,fZ,-8.4f,rooms[playerCurrentRoom].leftRoomDrawerOpenL[s],rooms[playerCurrentRoom].leftRoomSlotItemL[s])){iA=true;break;}} int tR=rooms[playerCurrentRoom].leftRoomSlotTypeR[s];if(tR>0&&tR!=99){if(tR==7||tR==8){if(cC(-3.6f,fZ,rooms[playerCurrentRoom].leftRoomDrawerOpenR[s])){iA=true;break;}}else if(cI(tR,fZ,-3.6f,rooms[playerCurrentRoom].leftRoomDrawerOpenR[s],rooms[playerCurrentRoom].leftRoomSlotItemR[s])){iA=true;break;}} } } if(!iA&&rooms[playerCurrentRoom].hasRightRoom){ float srZ=(-10.0f-(playerCurrentRoom*10.0f))+rooms[playerCurrentRoom].rightDoorOffset+2.5f; for(int s=0;s<3;s++){float fZ=srZ-0.9f-(s*1.6f);int tL=rooms[playerCurrentRoom].rightRoomSlotTypeL[s];if(tL>0&&tL!=99){if(tL==7||tL==8){if(cC(3.6f,fZ,rooms[playerCurrentRoom].rightRoomDrawerOpenL[s])){iA=true;break;}}else if(cI(tL,fZ,3.6f,rooms[playerCurrentRoom].rightRoomDrawerOpenL[s],rooms[playerCurrentRoom].rightRoomSlotItemL[s])){iA=true;break;}} int tR=rooms[playerCurrentRoom].rightRoomSlotTypeR[s];if(tR>0&&tR!=99){if(tR==7||tR==8){if(cC(8.4f,fZ,rooms[playerCurrentRoom].rightRoomDrawerOpenR[s])){iA=true;break;}}else if(cI(tR,fZ,8.4f,rooms[playerCurrentRoom].rightRoomDrawerOpenR[s],rooms[playerCurrentRoom].rightRoomSlotItemR[s])){iA=true;break;}} } } }
                if(!iA&&!lobbyKeyPickedUp&&rooms[0].isLocked&&camX<-3.5f&&camZ<-8.5f){lobbyKeyPickedUp=true;hasKey=true;needsVBOUpdate=true;sprintf(uiMessage,"Found the Lobby Key!");messageTimer=90;iA=true;if(audio_ok&&sCoinsCollect.data_vaddr){ndspChnWaveBufClear(10);sCoinsCollect.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(10,&sCoinsCollect);}}
                if(!iA){ for(int i=0;i<TOTAL_ROOMS;i++){ if(i==seekStartRoom+1||i==seekStartRoom+2)continue; if(rooms[i].isLocked||rooms[i].isJammed){ float dZ=-10.0f-(i*10.0f), dX=(rooms[i].doorPos==0)?-2.0f:((rooms[i].doorPos==1)?0.0f:2.0f); if(fabsf(camZ-dZ)<2.5f&&fabsf(camX-dX)<2.0f){ if(rooms[i].isJammed){if(audio_ok&&sLockedDoor.data_vaddr){ndspChnWaveBufClear(1);sLockedDoor.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(1,&sLockedDoor);}sprintf(uiMessage,"The door is jammed shut!");messageTimer=60;} else if(hasKey){rooms[i].isLocked=false;hasKey=false;needsVBOUpdate=true;sprintf(uiMessage,"Door Unlocked!");messageTimer=60;} else{if(audio_ok&&sLockedDoor.data_vaddr){ndspChnWaveBufClear(1);sLockedDoor.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(1,&sLockedDoor);}sprintf(uiMessage,"It's locked...");messageTimer=60;} iA=true;break; } } } }
            } 
            if((kDown&KEY_B)&&hideState==NOT_HIDING){ if(isCrouching){if(!checkCollision(camX,0,camZ,1.1f))isCrouching=false;} else isCrouching=true; }
            int nC=(camZ<-10.0f)?(int)((fabsf(camZ)-10.0f)/10.0f)+1:0; if(nC!=currentChunk||needsVBOUpdate){ if(nC!=currentChunk&&playerCurrentRoom>1&&!rushActive&&rushCooldown<=0&&!iSE&&rand()%100<12){rushActive=true;rushState=1;rushTimer=300+(rand()%120);rushStartTimer=(float)rushTimer;if(audio_ok){if(sLightsFlicker.data_vaddr){float m[12]={0};m[0]=2.5f;m[1]=2.5f;ndspChnSetMix(12,m);ndspChnWaveBufClear(12);sLightsFlicker.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(12,&sLightsFlicker);}if(sRushScream.data_vaddr){float m[12]={0};ndspChnSetMix(3,m);ndspChnWaveBufClear(3);sRushScream.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(3,&sRushScream);}}} currentChunk=nC;needsVBOUpdate=true; }
            int st=currentChunk-1, en=currentChunk+2; if(currentChunk>=seekStartRoom&&currentChunk<=seekStartRoom+2){st=seekStartRoom;en=seekStartRoom+3;} if(st<0)st=0; if(en>TOTAL_ROOMS-1)en=TOTAL_ROOMS-1;
            for(int i=st;i<=en;i++){ if(rooms[i].isDupeRoom||i==seekStartRoom+1||i==seekStartRoom+2)continue; bool sBO=(fabsf(camZ-(-10.0f-(i*10.0f)))<1.5f&&fabsf(camX-((rooms[i].doorPos==0)?-2.0f:((rooms[i].doorPos==1)?0.0f:2.0f)))<1.5f); if(rooms[i].isLocked||rooms[i].isJammed)sBO=false; if(doorOpen[i]!=sBO){if(sBO&&audio_ok&&sDoor.data_vaddr){ndspChnWaveBufClear(1);sDoor.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(1,&sDoor);doorOpen[i]=sBO;needsVBOUpdate=true;}} }
            if(needsVBOUpdate){
                buildWorld(currentChunk,playerCurrentRoom);
                colored_size = world_mesh_colored.size();
                textured_size = world_mesh_textured.size();
                memcpy(vbo_main, world_mesh_colored.data(), colored_size * sizeof(vertex));
                memcpy((vertex*)vbo_main + colored_size, world_mesh_textured.data(), textured_size * sizeof(vertex));
                GSPGPU_FlushDataCache(vbo_main, (colored_size + textured_size) * sizeof(vertex));
            }
            float pH=isCrouching?0.5f:1.1f; circlePosition cS, cP; irrstCstickRead(&cS); hidCircleRead(&cP); touchPosition t; hidTouchRead(&t);
            if((hideState==NOT_HIDING||hideState==BEHIND_DOOR)&&seekState!=1){
                if(abs(cS.dx)>10)camYaw-=cS.dx/1560.0f*0.8f; if(abs(cS.dy)>10)camPitch+=cS.dy/1560.0f*0.8f;
                if(kHeld&KEY_TOUCH){ if(!wasTouching){startTouchX=t.px;startTouchY=t.py;wasTouching=true;} else{float dx=(float)t.px-startTouchX,dy=(float)t.py-startTouchY;if(fabsf(dx)<10.0f)dx=0;if(fabsf(dy)<10.0f)dy=0;camYaw-=(dx/160.0f)*0.06f;camPitch-=(dy/120.0f)*0.06f;} } else wasTouching=false; 
                if(camPitch>1.57f)camPitch=1.57f; if(camPitch<-1.57f)camPitch=-1.57f;
                if(abs(cP.dy)>15||abs(cP.dx)>15){ float s=(seekState==2)?(isCrouching?0.25f:0.42f):(isCrouching?0.16f:0.28f), sy=cP.dy/1560.0f, sx=cP.dx/1560.0f, nX=camX-(sinf(camYaw)*sy-cosf(camYaw)*sx)*s, nZ=camZ-(cosf(camYaw)*sy+sinf(camYaw)*sx)*s; if(!checkCollision(nX,0,camZ,pH))camX=nX; if(!checkCollision(camX,0,nZ,pH))camZ=nZ; }
            }
            if(hideState==NOT_HIDING||hideState==BEHIND_DOOR){ bool iDZ=false; for(auto& b:collisions)if(b.type==4&&camX>b.minX&&camX<b.maxX&&camZ>b.minZ&&camZ<b.maxZ){iDZ=true;break;} if(iDZ&&hideState==NOT_HIDING)hideState=BEHIND_DOOR; else if(!iDZ&&hideState==BEHIND_DOOR)hideState=NOT_HIDING; }
        }

        // --- DRAWING BLOCK FIX ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW); 
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); 
        C3D_FrameDrawOn(target);
        
        float dCX=camX, dCZ=camZ, dCY=camYaw, dCP=camPitch; static float lCCZ=0.0f;
        if(seekState==1){ if(seekTimer==1)lCCZ=seekZ-4.0f; if(seekTimer<=90){float t=seekTimer/90.0f;t=t*t*(3.0f-2.0f*t);dCZ=camZ+(lCCZ-camZ)*t;dCY=camYaw+(3.14159f-camYaw)*t;} else{dCZ=lCCZ;dCY=3.14159f;if(seekTimer>180)dCP=sinf(seekTimer*0.8f)*0.03f;} }
        C3D_Mtx proj, view; Mtx_PerspTilt(&proj, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false); Mtx_Identity(&view); Mtx_RotateX(&view, -dCP, true); Mtx_RotateY(&view, -dCY, true); Mtx_Translate(&view, -dCX, isDead?-0.1f:(isCrouching?-0.4f:(hideState==NOT_HIDING||hideState==BEHIND_DOOR?-0.9f:(hideState==IN_CABINET?-0.7f:-0.15f))), -dCZ, true); Mtx_Multiply(&view, &proj, &view);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_proj, &view); 
        
        C3D_BufInfo* buf = C3D_GetBufInfo();
        BufInfo_Init(buf);
        BufInfo_Add(buf, vbo_main, sizeof(vertex), 3, 0x210);

        // 1. DRAW COLORED OBJECTS (Untextured)
        if (colored_size > 0) {
            C3D_TexEnv* env = C3D_GetTexEnv(0);
            C3D_TexEnvInit(env); // Clean state for solid colors
            
            if (flashRedFrames > 0 && !isDead) {
                C3D_TexEnvColor(env, 0xFF0000FF);
                C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
                C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            } else {
                C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            }
            C3D_DrawArrays(GPU_TRIANGLES, 0, colored_size);
        }

        // 2. DRAW TEXTURED OBJECTS (Walls & Floors)
        if (textured_size > 0) {
            C3D_TexBind(0, &atlasTex);
            
            C3D_TexEnv* env = C3D_GetTexEnv(0);
            C3D_TexEnvInit(env); // CRITICAL FIX: Flush the solid color state out of the GPU!
            
            if (flashRedFrames > 0 && !isDead) {
                C3D_TexEnvColor(env, 0xFF0000FF);
                C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
                C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            } else if (hasAtlas) {
                // Mix the texture with pure white vertex colors
                C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
                C3D_TexEnvOpAlpha(env, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
                C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
            } else {
                // Failsafe in case texture can't load
                C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            }
            
            // Draw starting at the end of the colored mesh
            C3D_DrawArrays(GPU_TRIANGLES, colored_size, textured_size);
        }

        C3D_FrameEnd(0);
    }
    
    if(audio_ok){ 
        if(sPsst.data_vaddr)linearFree((void*)sPsst.data_vaddr); if(sAttack.data_vaddr)linearFree((void*)sAttack.data_vaddr); 
        if(sCaught.data_vaddr)linearFree((void*)sCaught.data_vaddr); if(sDoor.data_vaddr)linearFree((void*)sDoor.data_vaddr); 
        if(sLockedDoor.data_vaddr)linearFree((void*)sLockedDoor.data_vaddr); if(sDupeAttack.data_vaddr)linearFree((void*)sDupeAttack.data_vaddr); 
        if(sRushScream.data_vaddr)linearFree((void*)sRushScream.data_vaddr); if(sEyesAppear.data_vaddr)linearFree((void*)sEyesAppear.data_vaddr); 
        if(sEyesGarble.data_vaddr)linearFree((void*)sEyesGarble.data_vaddr); if(sEyesAttack.data_vaddr)linearFree((void*)sEyesAttack.data_vaddr); 
        if(sEyesHit.data_vaddr)linearFree((void*)sEyesHit.data_vaddr); if(sSeekRise.data_vaddr)linearFree((void*)sSeekRise.data_vaddr); 
        if(sSeekChase.data_vaddr)linearFree((void*)sSeekChase.data_vaddr); if(sSeekEscaped.data_vaddr)linearFree((void*)sSeekEscaped.data_vaddr); 
        if(sDeath.data_vaddr)linearFree((void*)sDeath.data_vaddr); if(sElevatorJam.data_vaddr)linearFree((void*)sElevatorJam.data_vaddr); 
        if(sElevatorJamEnd.data_vaddr)linearFree((void*)sElevatorJamEnd.data_vaddr); if(sCoinsCollect.data_vaddr)linearFree((void*)sCoinsCollect.data_vaddr); 
        if(sDarkRoomEnter.data_vaddr)linearFree((void*)sDarkRoomEnter.data_vaddr); if(sDrawerClose.data_vaddr)linearFree((void*)sDrawerClose.data_vaddr); 
        if(sDrawerOpen.data_vaddr)linearFree((void*)sDrawerOpen.data_vaddr); if(sLightsFlicker.data_vaddr)linearFree((void*)sLightsFlicker.data_vaddr); 
        if(sWardrobeEnter.data_vaddr)linearFree((void*)sWardrobeEnter.data_vaddr); 
        if(sWardrobeExit.data_vaddr)linearFree((void*)sWardrobeExit.data_vaddr); 
        ndspExit(); 
    }
    C3D_TexDelete(&atlasTex); 
    linearFree(vbo_main);
    romfsExit(); C3D_Fini(); gfxExit(); return 0;
}
