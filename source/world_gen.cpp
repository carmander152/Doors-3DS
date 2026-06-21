#include "world_gen.h"
#include "render_utils.h"
#include "physics.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "atlas_uvs.h"

#define LIBRARY_ROOM 51

// --- ROTATIONAL MATRIX WRAPPERS ---
void rotateVertexRelative(float localX, float localZ, float centerX, float centerZ, Direction dir, float& outX, float& outZ) {
    if (dir == NORTH) { 
        outX = centerX + localX; 
        outZ = centerZ + localZ; 
    }
    else if (dir == EAST) { 
        outX = centerX - localZ; 
        outZ = centerZ + localX; 
    }
    else if (dir == SOUTH) { 
        outX = centerX - localX; 
        outZ = centerZ - localZ; 
    }
    else if (dir == WEST) { 
        outX = centerX + localZ; 
        outZ = centerZ - localX; 
    }
}

void addRBox(float lx, float ly, float lz, float lw, float lh, float ld, float r, float g, float b, bool col, int type, float L, float cx, float cz, Direction dir) {
    float rx1, rz1, rx2, rz2;
    rotateVertexRelative(lx, lz, cx, cz, dir, rx1, rz1);
    rotateVertexRelative(lx + lw, lz + ld, cx, cz, dir, rx2, rz2);
    
    float outX = fminf(rx1, rx2);
    float outW = fmaxf(rx1, rx2) - outX;
    float outZ = fminf(rz1, rz2);
    float outD = fmaxf(rz1, rz2) - outZ;

    // PREVENT CULLING BUG: Preserve original negative dimension signs based on rotation
    if ((dir == NORTH || dir == SOUTH) && lw < 0) { outX += outW; outW = -outW; }
    if ((dir == NORTH || dir == SOUTH) && ld < 0) { outZ += outD; outD = -outD; }
    if ((dir == EAST || dir == WEST) && ld < 0) { outX += outW; outW = -outW; }
    if ((dir == EAST || dir == WEST) && lw < 0) { outZ += outD; outD = -outD; }
    
    addBox(outX, ly, outZ, outW, lh, outD, r, g, b, false, 0, L);
}

void addRSurf(float lx, float ly, float lz, float lw, float lh, float ld, float u, float v, float uw, float vh, float tS, float r, float g, float b, float L, bool isWall, float cx, float cz, Direction dir) {
    float rx1, rz1, rx2, rz2;
    rotateVertexRelative(lx, lz, cx, cz, dir, rx1, rz1);
    rotateVertexRelative(lx + lw, lz + ld, cx, cz, dir, rx2, rz2);
    
    float outX = fminf(rx1, rx2);
    float outW = fmaxf(rx1, rx2) - outX;
    float outZ = fminf(rz1, rz2);
    float outD = fmaxf(rz1, rz2) - outZ;

    // PREVENT CULLING BUG: Preserve original negative dimension signs based on rotation
    if ((dir == NORTH || dir == SOUTH) && lw < 0) { outX += outW; outW = -outW; }
    if ((dir == NORTH || dir == SOUTH) && ld < 0) { outZ += outD; outD = -outD; }
    if ((dir == EAST || dir == WEST) && ld < 0) { outX += outW; outW = -outW; }
    if ((dir == EAST || dir == WEST) && lw < 0) { outZ += outD; outD = -outD; }
    
    addTiledSurface(outX, ly, outZ, outW, lh, outD, u, v, uw, vh, tS, r, g, b, L, isWall);
}

void addRCol(float lx, float ly, float lz, float lw, float lh, float ld, int type, float cx, float cz, Direction dir) {
    float rx1, rz1, rx2, rz2;
    rotateVertexRelative(lx, lz, cx, cz, dir, rx1, rz1);
    rotateVertexRelative(lx + lw, lz + ld, cx, cz, dir, rx2, rz2);
    
    float minX = fminf(rx1, rx2);
    float maxX = fmaxf(rx1, rx2);
    float minZ = fminf(rz1, rz2);
    float maxZ = fmaxf(rz1, rz2);
    
    collisions.push_back({minX, ly, minZ, maxX, ly + lh, maxZ, type});
}

// --- GENERATORS ---
void buildRLamp(float lx, float lz, float L, float cx, float cz, Direction dir) { 
    addRBox(lx-0.15f, 0.46f, lz-0.15f, 0.3f, 0.05f, 0.3f, 0.3f, 0.15f, 0.05f, false, 0, L, cx, cz, dir); 
    addRBox(lx-0.05f, 0.51f, lz-0.05f, 0.1f, 0.2f, 0.1f, 0.2f, 0.3f, 0.4f, false, 0, L, cx, cz, dir); 
    addRBox(lx-0.2f, 0.71f, lz-0.2f, 0.4f, 0.25f, 0.4f, 0.9f, 0.9f, 0.8f, false, 0, L, cx, cz, dir); 
}

void buildCabinet(float zC, bool isL, float L, float offX, int item, float cx, float cz, Direction dir) {
    float bX = (isL ? -4.95f : 4.85f) + offX;
    float tX = (isL ? -4.95f : 4.15f) + offX;
    float fX = (isL ? -4.25f : 4.15f) + offX;
    float hX = (isL ? fX + 0.1f : fX - 0.03f); 
    
    addRBox(bX, 0.0f, zC-0.5f, 0.1f, 1.5f, 1.0f, 0.3f, 0.18f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(tX, 1.5f, zC-0.5f, 0.8f, 0.1f, 1.0f, 0.3f, 0.18f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(tX, 0.0f, zC-0.5f, 0.8f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(tX, 0.0f, zC+0.4f, 0.8f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(fX, 0.0f, zC-0.4f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(fX, 0.0f, zC+0.05f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(hX, 0.6f, zC-0.15f, 0.03f, 0.15f, 0.04f, 0.9f, 0.75f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(hX, 0.6f, zC+0.11f, 0.03f, 0.15f, 0.04f, 0.9f, 0.75f, 0.1f, false, 0, L, cx, cz, dir); 
    
    if(hideState != IN_CABINET) {
        addRBox((isL ? -4.85f : 4.25f) + offX, 0.01f, zC-0.4f, 0.6f, 1.48f, 0.8f, 0, 0, 0, false, 0, L, cx, cz, dir); 
    }
    
    if (!isBuildingEntities) {
        addRCol((isL ? -4.9f : 4.1f) + offX, 0.0f, zC-0.5f, 0.8f, 1.5f, 1.0f, 1, cx, cz, dir);
    }
}

void buildBed(float zC, bool isL, int item, float L, float offX, float cx, float cz, Direction dir) {
    float x = (isL ? -4.95f : 3.55f) + offX;
    float sX = (isL ? -3.65f : 3.55f) + offX;
    float pX = (isL ? -4.65f : 4.15f) + offX; 
    
    addRBox(x, 0.4f, zC-1.25f, 1.4f, 0.1f, 2.5f, 0.4f, 0.1f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(x, 0.0f, zC+1.15f, 0.1f, 0.4f, 0.1f, 0.2f, 0.1f, 0.05f, false, 0, L, cx, cz, dir); 
    addRBox(x+1.3f, 0.0f, zC+1.15f, 0.1f, 0.4f, 0.1f, 0.2f, 0.1f, 0.05f, false, 0, L, cx, cz, dir);
    addRBox(x, 0.0f, zC-1.25f, 0.1f, 0.4f, 0.1f, 0.2f, 0.1f, 0.05f, false, 0, L, cx, cz, dir); 
    addRBox(x+1.3f, 0.0f, zC-1.25f, 0.1f, 0.4f, 0.1f, 0.2f, 0.1f, 0.05f, false, 0, L, cx, cz, dir); 
    addRBox(sX, 0.2f, zC-1.25f, 0.1f, 0.2f, 2.5f, 0.4f, 0.1f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(pX, 0.5f, zC+0.4f, 0.5f, 0.08f, 0.6f, 0.9f, 0.9f, 0.9f, false, 0, L, cx, cz, dir); 
    
    if (item == 1) { 
        float ix = (isL ? -4.2f : 4.1f) + offX; 
        addRBox(ix, 0.52f, zC-0.1f, 0.14f, 0.035f, 0.035f, 0.9f, 0.75f, 0.1f, false, 0, L, cx, cz, dir); 
        addRBox(ix+0.1f, 0.52f, zC-0.13f, 0.07f, 0.035f, 0.1f, 0.9f, 0.75f, 0.1f, false, 0, L, cx, cz, dir); 
        addRBox(ix+0.03f, 0.52f, zC-0.065f, 0.035f, 0.035f, 0.05f, 0.9f, 0.75f, 0.1f, false, 0, L, cx, cz, dir); 
    } else if (item == 3) {
        addRBox((isL ? -4.2f : 4.1f) + offX + 0.02f, 0.52f, zC-0.01f, 0.04f, 0.005f, 0.04f, 1.0f, 0.85f, 0.0f, false, 0, L, cx, cz, dir);
    }
    
    if(!isBuildingEntities) {
        addRCol(x, 0.0f, zC-1.25f, 1.4f, 0.6f, 2.5f, 2, cx, cz, dir);
    }
}

void buildDresser(float zC, bool isL, float openFactor, int item, float L, float offX, bool hasLamp, float cx, float cz, Direction dir) {
    float frX = (isL ? -4.95f : 4.45f) + offX;
    float oOff = openFactor * (isL ? 0.35f : -0.35f);
    float trX = (isL ? (-4.9f + oOff) : (4.4f + oOff)) + offX;
    float hX = (isL ? (-4.4f + oOff) : (4.35f + oOff)) + offX;
    float iX = (isL ? (-4.6f + oOff) : (4.5f + oOff)) + offX;
    
    addRBox(frX+0.05f, 0.0f, zC-0.35f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L, cx, cz, dir); 
    addRBox(frX+0.05f, 0.0f, zC+0.3f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L, cx, cz, dir);
    addRBox(frX+0.4f, 0.0f, zC-0.35f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L, cx, cz, dir); 
    addRBox(frX+0.4f, 0.0f, zC+0.3f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L, cx, cz, dir);
    addRBox(frX, 0.2f, zC-0.4f, 0.5f, 0.3f, 0.8f, 0.3f, 0.15f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(trX, 0.3f, zC-0.35f, 0.5f, 0.15f, 0.7f, 0.25f, 0.12f, 0.08f, false, 0, L, cx, cz, dir); 
    addRBox(hX, 0.35f, zC-0.1f, 0.05f, 0.05f, 0.2f, 0.8f, 0.8f, 0.8f, false, 0, L, cx, cz, dir);
    
    if(openFactor > 0.1f) { 
        if (item == 1) { 
            addRBox(iX, 0.46f, zC-0.1f, 0.14f, 0.035f, 0.035f, 0.9f, 0.75f, 0.1f, false, 0, L, cx, cz, dir); 
            addRBox(iX+0.1f, 0.46f, zC-0.13f, 0.07f, 0.035f, 0.1f, 0.9f, 0.75f, 0.1f, false, 0, L, cx, cz, dir); 
            addRBox(iX+0.03f, 0.46f, zC-0.065f, 0.035f, 0.035f, 0.05f, 0.9f, 0.75f, 0.1f, false, 0, L, cx, cz, dir); 
        } else if (item == 2) {
            addRBox(iX, 0.46f, zC-0.05f, 0.15f, 0.02f, 0.08f, 0.8f, 0.6f, 0.4f, false, 0, L, cx, cz, dir); 
        } else if (item == 3) {
            addRBox(iX+0.02f, 0.46f, zC-0.05f, 0.04f, 0.005f, 0.04f, 1.0f, 0.85f, 0.0f, false, 0, L, cx, cz, dir); 
        } 
    }
    
    if (hasLamp) {
        buildRLamp((isL ? -4.7f : 4.7f) + offX, zC, L, cx, cz, dir);
    }
    if (!isBuildingEntities) {
        addRCol(frX, 0.0f, zC-0.4f, 0.5f, 0.5f, 0.8f, 3, cx, cz, dir);
    }
}

void buildChest(float x, float z, float openFactor, float L, float cx, float cz, Direction dir) {
    bool isOpen = (openFactor > 0.5f);
    
    addRBox(x-0.4f, 0.0f, z-0.3f, 0.8f, 0.4f, 0.6f, 0.3f, 0.15f, 0.05f, false, 0, L, cx, cz, dir); 
    addRBox(x-0.42f, 0.0f, z-0.32f, 0.05f, 0.4f, 0.05f, 0.8f, 0.7f, 0.1f, false, 0, L, cx, cz, dir); 
    addRBox(x+0.37f, 0.0f, z-0.32f, 0.05f, 0.4f, 0.05f, 0.8f, 0.7f, 0.1f, false, 0, L, cx, cz, dir);
    
    if (!isOpen) { 
        addRBox(x-0.4f, 0.4f, z-0.3f, 0.8f, 0.2f, 0.6f, 0.35f, 0.18f, 0.08f, false, 0, L, cx, cz, dir); 
        addRBox(x-0.05f, 0.3f, z+0.3f, 0.1f, 0.15f, 0.05f, 0.8f, 0.8f, 0.8f, false, 0, L, cx, cz, dir); 
    } else { 
        addRBox(x-0.4f, 0.4f, z-0.4f, 0.8f, 0.6f, 0.1f, 0.35f, 0.18f, 0.08f, false, 0, L, cx, cz, dir); 
    }
}

void buildSideRoomInterior(float cx, float cz, Direction dir, float doorOffset, float L, 
                           int typeL[3], int itemL[3], float animL[3], 
                           int typeR[3], int itemR[3], float animR[3]) {
    
    float floorU = TEX_FLOOR.u, floorV = TEX_FLOOR.v, floorUW = TEX_FLOOR.uw, floorVH = TEX_FLOOR.vh;
    float wallU = TEX_WALL.u, wallV = TEX_WALL.v, wallUW = TEX_WALL.uw, wallVH = TEX_WALL.vh;
    float cR = 1.0f, cG = 1.0f, cB = 1.0f, tS = 2.4f;
    
    addRSurf(doorOffset - 3.0f, 0.0f, -11.0f, 6.0f, 0.01f, 6.0f, floorU, floorV, floorUW, floorVH, tS, cR, cG, cB, L, false, cx, cz, dir);
    addRSurf(doorOffset - 3.0f, 1.8f, -11.0f, 6.0f, 0.01f, 6.0f, floorU, floorV, floorUW, floorVH, tS, cR, cG, cB, L, false, cx, cz, dir);
    
    addRSurf(doorOffset - 3.0f, 0.4f, -11.0f, 0.1f, 1.4f, 6.0f, wallU, wallV, wallUW, wallVH, tS, cR, cG, cB, L, true, cx, cz, dir); 
    addRSurf(doorOffset - 3.0f, 0.0f, -11.0f, 0.12f, 0.4f, 6.0f, wallU, wallV, wallUW, wallVH, tS, cR, cG, cB, L, false, cx, cz, dir); 
    addRBox(doorOffset - 2.88f, 0.0f, -11.0f, 0.04f, 0.12f, 6.0f, 0.12f, 0.06f, 0.03f, false, 0, L, cx, cz, dir);
    
    addRSurf(doorOffset + 2.9f, 0.4f, -11.0f, 0.1f, 1.4f, 6.0f, wallU, wallV, wallUW, wallVH, tS, cR, cG, cB, L, true, cx, cz, dir); 
    addRSurf(doorOffset + 2.88f, 0.0f, -11.0f, 0.12f, 0.4f, 6.0f, wallU, wallV, wallUW, wallVH, tS, cR, cG, cB, L, false, cx, cz, dir); 
    addRBox(doorOffset + 2.84f, 0.0f, -11.0f, 0.04f, 0.12f, 6.0f, 0.12f, 0.06f, 0.03f, false, 0, L, cx, cz, dir);

    addRSurf(doorOffset - 3.0f, 0.4f, -11.1f, 6.0f, 1.4f, 0.1f, wallU, wallV, wallUW, wallVH, tS, cR, cG, cB, L, true, cx, cz, dir); 
    addRSurf(doorOffset - 3.0f, 0.0f, -11.12f, 6.0f, 0.4f, 0.12f, wallU, wallV, wallUW, wallVH, tS, cR, cG, cB, L, false, cx, cz, dir); 
    addRBox(doorOffset - 3.0f, 0.0f, -11.04f, 6.0f, 0.12f, 0.04f, 0.12f, 0.06f, 0.03f, false, 0, L, cx, cz, dir);
    
    addRSurf(doorOffset - 3.0f, 0.4f, -5.0f, 2.4f, 1.4f, 0.1f, wallU, wallV, wallUW, wallVH, tS, cR, cG, cB, L, true, cx, cz, dir);
    addRSurf(doorOffset - 3.0f, 0.0f, -5.0f, 2.4f, 0.4f, 0.12f, wallU, wallV, wallUW, wallVH, tS, cR, cG, cB, L, false, cx, cz, dir);
    addRBox(doorOffset - 3.0f, 0.0f, -5.04f, 2.4f, 0.12f, 0.04f, 0.12f, 0.06f, 0.03f, false, 0, L, cx, cz, dir);
    
    addRSurf(doorOffset + 0.6f, 0.4f, -5.0f, 2.4f, 1.4f, 0.1f, wallU, wallV, wallUW, wallVH, tS, cR, cG, cB, L, true, cx, cz, dir);
    addRSurf(doorOffset + 0.6f, 0.0f, -5.0f, 2.4f, 0.4f, 0.12f, wallU, wallV, wallUW, wallVH, tS, cR, cG, cB, L, false, cx, cz, dir);
    addRBox(doorOffset + 0.6f, 0.0f, -5.04f, 2.4f, 0.12f, 0.04f, 0.12f, 0.06f, 0.03f, false, 0, L, cx, cz, dir);

    if (!isBuildingEntities) {
        addRCol(doorOffset - 3.0f, 0.0f, -11.1f, 6.0f, 1.8f, 0.1f, 0, cx, cz, dir); 
        addRCol(doorOffset - 3.0f, 0.0f, -11.0f, 0.1f, 1.8f, 6.0f, 0, cx, cz, dir); 
        addRCol(doorOffset + 2.9f, 0.0f, -11.0f, 0.1f, 1.8f, 6.0f, 0, cx, cz, dir); 
    }

    for (int s = 0; s < 3; s++) {
        float lz = -5.9f - (s * 1.6f);
        
        int tL = typeL[s]; int iL = itemL[s]; float aL = animL[s];
        
        if (tL == 1) buildCabinet(lz, true, L, doorOffset + 2.0f, iL, cx, cz, dir); 
        else if (tL == 3) buildBed(lz, true, iL, L, doorOffset + 2.0f, cx, cz, dir);
        else if (tL == 5) buildDresser(lz, true, aL, iL, L, doorOffset + 2.0f, true, cx, cz, dir);
        else if (tL == 7 || tL == 8) buildChest(doorOffset - 2.4f, lz, aL, L, cx, cz, dir);
        
        int tR = typeR[s]; int iR = itemR[s]; float aR = animR[s];
        
        if (tR == 2) buildCabinet(lz, false, L, doorOffset - 2.0f, iR, cx, cz, dir);
        else if (tR == 4) buildBed(lz, false, iR, L, doorOffset - 2.0f, cx, cz, dir);
        else if (tR == 6) buildDresser(lz, false, aR, iR, L, doorOffset - 2.0f, true, cx, cz, dir);
        else if (tR == 7 || tR == 8) buildChest(doorOffset + 2.4f, lz, aR, L, cx, cz, dir);
    }
}

void addRWall(bool isExit, bool isDoorOpen, bool isLocked, float lz, float cx, float cz, Direction dir, float L) {
    float wallU = TEX_WALL.u, wallV = TEX_WALL.v, wallUW = TEX_WALL.uw, wallVH = TEX_WALL.vh; 
    float tS = 2.4f, r = 1.0f, g = 1.0f, b = 1.0f; 
    
    if (!isExit) {
        addRSurf(-5.0f, 0.4f, lz, 10.0f, 1.4f, -0.2f, wallU, wallV, wallUW, wallVH, tS, r,g,b, L, true, cx, cz, dir);
        addRSurf(-5.0f, 0.0f, lz, 10.0f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, tS, r,g,b, L, false, cx, cz, dir);
        addRBox(-5.0f, 0.0f, lz, 10.0f, 0.12f, 0.04f, 0.12f, 0.06f, 0.03f, false, 0, L, cx, cz, dir); 
        
        if (!isBuildingEntities) {
            addRCol(-5.0f, 0.0f, lz-0.2f, 10.0f, 2.0f, 0.4f, 0, cx, cz, dir); 
        }
    } else {
        // Left section
        addRSurf(-5.0f, 0.4f, lz, 4.4f, 1.4f, -0.2f, wallU, wallV, wallUW, wallVH, tS, r,g,b, L, true, cx, cz, dir);
        addRSurf(-5.0f, 0.0f, lz, 4.4f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, tS, r,g,b, L, false, cx, cz, dir);
        addRBox(-5.0f, 0.0f, lz, 4.4f, 0.12f, 0.04f, 0.12f, 0.06f, 0.03f, false, 0, L, cx, cz, dir);
        if (!isBuildingEntities) {
            addRCol(-5.0f, 0.0f, lz-0.2f, 4.4f, 2.0f, 0.4f, 0, cx, cz, dir); 
        }
        
        // Right section
        addRSurf(0.6f, 0.4f, lz, 4.4f, 1.4f, -0.2f, wallU, wallV, wallUW, wallVH, tS, r,g,b, L, true, cx, cz, dir);
        addRSurf(0.6f, 0.0f, lz, 4.4f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, tS, r,g,b, L, false, cx, cz, dir);
        addRBox(0.6f, 0.0f, lz, 4.4f, 0.12f, 0.04f, 0.12f, 0.06f, 0.03f, false, 0, L, cx, cz, dir);
        if (!isBuildingEntities) {
            addRCol(0.6f, 0.0f, lz-0.2f, 4.4f, 2.0f, 0.4f, 0, cx, cz, dir); 
        }
        
        // Top section
        addRSurf(-0.6f, 1.4f, lz, 1.2f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, tS, r,g,b, L, false, cx, cz, dir);
        
        // Door frame
        addRBox(-0.6f, 0.0f, lz, 0.05f, 1.4f, -0.2f, 0.15f, 0.08f, 0.04f, false, 0, L, cx, cz, dir); 
        addRBox(0.55f, 0.0f, lz, 0.05f, 1.4f, -0.2f, 0.15f, 0.08f, 0.04f, false, 0, L, cx, cz, dir); 
        addRBox(-0.55f, 1.35f, lz, 1.1f, 0.05f, -0.2f, 0.15f, 0.08f, 0.04f, false, 0, L, cx, cz, dir); 
        
        if (!isDoorOpen) { 
            // Closed door
            addRBox(-0.6f, 0.0f, lz, 1.2f, 1.4f, -0.1f, 0.15f, 0.08f, 0.05f, false, 0, L, cx, cz, dir); 
            addRBox(-0.2f, 1.1f, lz+0.02f, 0.4f, 0.12f, 0.02f, 0.8f, 0.7f, 0.2f, false, 0, L, cx, cz, dir); 
            addRBox(0.45f, 0.7f, lz+0.02f, 0.05f, 0.15f, 0.03f, 0.6f, 0.6f, 0.6f, false, 0, L, cx, cz, dir); 
            
            if (isLocked) {
                addRBox(0.3f, 0.6f, lz+0.05f, 0.2f, 0.2f, 0.05f, 0.8f, 0.8f, 0.8f, false, 0, L, cx, cz, dir);
            }
            if (!isBuildingEntities) {
                addRCol(-0.6f, 0.0f, lz-0.2f, 1.2f, 2.0f, 0.4f, 0, cx, cz, dir); 
            }
        } else { 
            // Open door
            addRBox(-0.6f, 0.0f, lz, 0.1f, 1.4f, -1.2f, 0.3f, 0.15f, 0.08f, false, 0, L, cx, cz, dir); 
            addRBox(-0.5f, 1.1f, lz-0.8f, 0.02f, 0.12f, 0.4f, 0.8f, 0.7f, 0.2f, false, 0, L, cx, cz, dir); 
            addRBox(-0.5f, 0.7f, lz-1.05f, 0.03f, 0.15f, 0.05f, 0.6f, 0.6f, 0.6f, false, 0, L, cx, cz, dir); 
            
            if (!isBuildingEntities) { 
                addRCol(-0.6f, 0.0f, lz-1.15f, 1.1f, 1.8f, 0.1f, 4, cx, cz, dir); 
                addRCol(-0.6f, 0.0f, lz-1.2f, 0.1f, 2.0f, 1.2f, 0, cx, cz, dir); 
            }
        }
    }
}

// --- WORLD GENERATION COMPILATION ---
void buildWorld(int cChunk, int pRm) {
    world_mesh_colored.clear(); 
    world_mesh_textured.clear(); 
    collisions.clear();
    
    float floorU = TEX_FLOOR.u, floorV = TEX_FLOOR.v, floorUW = TEX_FLOOR.uw, floorVH = TEX_FLOOR.vh;
    float wallU = TEX_WALL.u, wallV = TEX_WALL.v, wallUW = TEX_WALL.uw, wallVH = TEX_WALL.vh;
    float cR = 1.0f, cG = 1.0f, cB = 1.0f, floorScale = 2.4f, wallScale = 2.4f;  
    
    int st = pRm - 1; 
    int en = pRm + 1; 
    
    if (pRm == -1) { 
        st = -1; 
        en = doorOpen[0] ? 0 : -1; 
    } else { 
        if (st < -1) st = -1;
        if (pRm + 1 < TOTAL_ROOMS && doorOpen[pRm + 1]) en = pRm + 2; 
    }
    
    if (pRm >= seekStartRoom - 1 && pRm <= seekStartRoom + 3) { 
        st = seekStartRoom - 1; 
        en = seekStartRoom + 4; 
    }
    
    if (st < -1) st = -1; 
    if (en > TOTAL_ROOMS - 1) en = TOTAL_ROOMS - 1;

    // Hardcoded Lobby Construction - Shifted +10 units down the Z-axis
    if (st <= -1) {
        globalTintR = 1.0f; globalTintG = 1.0f; globalTintB = 1.0f;
        
        addTiledSurface(-2.0f, 0.0f, 15.0f, 4.0f, 0.01f, 4.0f, floorU, floorV, floorUW, floorVH, floorScale, cR, cG, cB, 1.0f, false); 
        addTiledSurface(-2.0f, 2.0f, 15.0f, 4.0f, 0.01f, 4.0f, floorU, floorV, floorUW, floorVH, floorScale, cR, cG, cB, 1.0f, false); 
        
        addBox(-2.0f, 0.0f, 19.0f, 4.0f, 2.0f, 0.1f, 0.4f, 0.3f, 0.2f, true); 
        addBox(-2.0f, 0.0f, 15.0f, 0.1f, 2.0f, 4.0f, 0.4f, 0.3f, 0.2f, true); 
        addBox(1.9f, 0.0f, 15.0f, 0.1f, 2.0f, 4.0f, 0.4f, 0.3f, 0.2f, true);   
        
        addBox(1.8f, 0.6f, 16.5f, 0.15f, 0.3f, 0.2f, 0.1f, 0.1f, 0.1f, false); 
        addBox(1.75f, 0.7f, 16.55f, 0.05f, 0.1f, 0.1f, 0, 0.8f, 0, false, 0, 1.5f); 
        
        addBox(-2.0f - elevatorDoorOffset, 0.0f, 15.05f, 2.0f, 2.0f, 0.1f, 0.6f, 0.6f, 0.6f, true); 
        addBox(0.0f + elevatorDoorOffset, 0.0f, 15.05f, 2.0f, 2.0f, 0.1f, 0.6f, 0.6f, 0.6f, true);  
        
        if (elevatorDoorOffset < 0.05f) {
            addBox(-0.02f, 0.0f, 15.04f, 0.04f, 2.0f, 0.12f, 0, 0, 0, false); 
        }
        
        if (elevatorClosing) {
            collisions.push_back({-2.0f, 0.0f, 14.8f, 2.0f, 2.0f, 15.1f, 0});
        }
        
        addTiledSurface(-6.0f, 0.0f, 15.0f, 12.0f, 0.01f, -15.0f, floorU, floorV, floorUW, floorVH, floorScale, cR, cG, cB, 1.0f, false); 
        addTiledSurface(-6.0f, 1.8f, 15.0f, 12.0f, 0.01f, -15.0f, floorU, floorV, floorUW, floorVH, floorScale, cR, cG, cB, 1.0f, false); 
        
        addBox(-6.0f, 0.0f, 15.0f, 0.1f, 1.8f, -15.0f, 0.3f, 0.3f, 0.3f, true); 
        addBox(6.0f, 0.0f, 15.0f, 0.1f, 1.8f, -15.0f, 0.3f, 0.3f, 0.3f, true);  
        
        addBox(-6.0f, 0.0f, 0.0f, 3.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true); 
        addBox(3.0f, 0.0f, 0.0f, 3.0f, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true); 
        
        addBox(-6.0f, 0.0f, 14.9f, 4.0f, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true); 
        addBox(2.0f, 0.0f, 14.9f, 4.0f, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true);  
        
        addBox(-6.0f, 0.0f, 3.0f, 3.5f, 0.8f, -0.8f, 0.3f, 0.15f, 0.1f, true); 
        addBox(-3.3f, 0.0f, 2.2f, 0.8f, 0.8f, -1.0f, 0.3f, 0.15f, 0.1f, true); 
        
        addBox(-2.5f, 0.1f, 1.4f, 1.0f, 0.05f, -1.4f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.15f, 1.4f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-1.55f, 0.15f, 1.4f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.15f, 0.05f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-1.55f, 0.15f, 0.05f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.6f, 1.4f, 1.0f, 0.05f, -1.4f, 0.8f, 0.7f, 0.2f, true); 
        
        if (!lobbyKeyPickedUp) { 
            addBox(-4.8f, 0.9f, 0.1f, 0.2f, 0.2f, 0.05f, 0.3f, 0.2f, 0.1f, false); 
            addBox(-4.72f, 0.75f, 0.14f, 0.035f, 0.1f, 0.035f, 1.0f, 0.84f, 0.0f, false); 
        }

        // Attach exit door to perfectly link up with Room 0 at Z=0!
        addRWall(true, doorOpen[0], rooms[0].isLocked, 0.0f, 0.0f, 0.0f, NORTH, 1.0f);
    }

    if (st < 0) st = 0; 
    
    // Iterative spatial rendering loop for standard rooms
    for(int i = st; i <= en; i++) {
        float cx = rooms[i].centerX;
        float cz = rooms[i].centerZ;
        Direction dir = rooms[i].orientation;
        
        float L = rooms[i].lightLevel;
        float wL = (i > 0) ? rooms[i-1].lightLevel : 1.0f; 

        if (seekState == 1) { 
            globalTintR=1.0f; globalTintG=0.2f; globalTintB=0.2f; 
        } else if (rooms[i].hasEyes) { 
            globalTintR=0.8f; globalTintG=0.3f; globalTintB=1.0f; 
        } else { 
            globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f; 
        }

        // --- CUSTOM LIBRARY RENDERING ---
        if (i == LIBRARY_ROOM) {
            float z = cz + 5.0f; 
            float cL = rooms[i].lightLevel; 
            globalTintR = 0.9f; globalTintG = 0.8f; globalTintB = 0.6f; 
            
            addTiledSurface(-3.0f, 0.0f, z, 2.0f, 1.8f, -0.2f, wallU, wallV, wallUW, wallVH, wallScale, cR, cG, cB, cL, false); 
            addTiledSurface(1.0f, 0.0f, z, 2.0f, 1.8f, -0.2f, wallU, wallV, wallUW, wallVH, wallScale, cR, cG, cB, cL, false); 
            addTiledSurface(-3.0f, 1.8f, z, 6.0f, 1.8f, -0.2f, wallU, wallV, wallUW, wallVH, wallScale, cR, cG, cB, cL, false); 
            
            if (!doorOpen[i]) {
                addBox(-1.0f, 0.0f, z, 1.0f, 1.8f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, cL); 
                addBox(0.0f, 0.0f, z, 1.0f, 1.8f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, cL); 
            } else {
                addBox(-1.0f, 0.0f, z-0.9f, 0.1f, 1.8f, 1.0f, 0.2f, 0.1f, 0.05f, true, 0, cL); 
                addBox(0.9f, 0.0f, z-0.9f, 0.1f, 1.8f, 1.0f, 0.2f, 0.1f, 0.05f, true, 0, cL); 
            }
            
            addTiledSurface(-6.0f, 0.0f, z, 12.0f, 0.01f, -20.0f, floorU, floorV, floorUW, floorVH, floorScale, cR, cG, cB, cL, false); 
            addTiledSurface(-6.0f, 3.6f, z, 12.0f, 0.01f, -20.0f, floorU, floorV, floorUW, floorVH, floorScale, cR, cG, cB, cL, false); 
            
            addTiledSurface(-6.1f, 0.0f, z, 0.1f, 3.6f, -20.0f, wallU, wallV, wallUW, wallVH, wallScale, cR, cG, cB, cL, true); 
            addTiledSurface(6.0f, 0.0f, z, 0.1f, 3.6f, -20.0f, wallU, wallV, wallUW, wallVH, wallScale, cR, cG, cB, cL, true);  
            
            addTiledSurface(-6.0f, 0.0f, z-20.0f, 12.0f, 3.6f, -0.1f, wallU, wallV, wallUW, wallVH, wallScale, cR, cG, cB, cL, true);
            
            addBox(-0.6f, 0.0f, z-19.9f, 1.2f, 1.6f, 0.1f, 0.15f, 0.08f, 0.04f, true, 0, cL); 
            addBox(-0.1f, 0.7f, z-19.8f, 0.2f, 0.3f, 0.1f, 0.8f, 0.8f, 0.8f, false, 0, cL); 
            
            addBox(-6.0f, 1.8f, z-2.0f, 3.0f, 0.1f, -16.0f, 0.25f, 0.15f, 0.1f, true, 0, cL); 
            addBox(3.0f, 1.8f, z-2.0f, 3.0f, 0.1f, -16.0f, 0.25f, 0.15f, 0.1f, true, 0, cL);  
            addBox(-3.0f, 1.8f, z-15.0f, 6.0f, 0.1f, -3.0f, 0.25f, 0.15f, 0.1f, true, 0, cL); 

            addBox(-3.1f, 1.9f, z-2.0f, 0.1f, 0.6f, -13.0f, 0.15f, 0.08f, 0.05f, true, 0, cL); 
            addBox(3.0f, 1.9f, z-2.0f, 0.1f, 0.6f, -13.0f, 0.15f, 0.08f, 0.05f, true, 0, cL);
            addBox(-3.0f, 1.9f, z-14.9f, 6.0f, 0.6f, -0.1f, 0.15f, 0.08f, 0.05f, true, 0, cL);
            
            for(int stt = 0; stt < 12; stt++) { 
                float sY = stt * 0.15f; 
                float sZ = z - 2.0f - (stt * 0.25f);
                addBox(-5.5f, 0.0f, sZ, 2.0f, sY+0.15f, -0.25f, 0.2f, 0.12f, 0.08f, true, 0, cL); 
                addBox(3.5f, 0.0f, sZ, 2.0f, sY+0.15f, -0.25f, 0.2f, 0.12f, 0.08f, true, 0, cL); 
            }
            
            addBox(-1.5f, 0.0f, z-7.0f, 3.0f, 0.6f, -4.0f, 0.3f, 0.18f, 0.1f, true, 0, cL); 
            addBox(-1.6f, 0.6f, z-6.9f, 3.2f, 0.1f, -4.2f, 0.2f, 0.1f, 0.05f, true, 0, cL); 
            
            buildRLamp(-1.2f, -7.5f, cL, cx, cz, dir); 
            buildRLamp(1.2f, -7.5f, cL, cx, cz, dir);
            
            globalTintR = 1.0f; globalTintG = 1.0f; globalTintB = 1.0f; 
            continue; 
        }

        if (i == LIBRARY_ROOM + 1) continue;

        // Render basic floor and ceiling layout for Standard Room
        addRSurf(-5.0f, 0.0f, -5.0f, 10.0f, 0.01f, 10.0f, floorU, floorV, floorUW, floorVH, floorScale, cR, cG, cB, L, false, cx, cz, dir);
        addRSurf(-5.0f, 1.8f, -5.0f, 10.0f, 0.01f, 10.0f, floorU, floorV, floorUW, floorVH, floorScale, cR, cG, cB, L, false, cx, cz, dir);
        
        bool nextOpen = (i + 1 < TOTAL_ROOMS) ? doorOpen[i+1] : false;
        bool nextLock = (i + 1 < TOTAL_ROOMS) ? rooms[i+1].isLocked : true;

        // --- WEAVING WALL RENDERING ---
        if (rooms[i].isDupeRoom) {
            // Dupe rooms generate doors on all 3 remaining walls
            addRWall(true, nextOpen && rooms[i].correctDupePos == 0, false, -5.0f, cx, cz, (Direction)((dir + 3) % 4), wL); 
            addRWall(true, nextOpen && rooms[i].correctDupePos == 1, false, -5.0f, cx, cz, dir, wL); 
            addRWall(true, nextOpen && rooms[i].correctDupePos == 2, false, -5.0f, cx, cz, (Direction)((dir + 1) % 4), wL); 
        } else {
            bool exitLeft = (rooms[i].chosenExitSide == 0);
            bool exitStraight = (rooms[i].chosenExitSide == 1);
            bool exitRight = (rooms[i].chosenExitSide == 2);
            
            // Construct Left Wall or Side-Room
            if (rooms[i].hasLeftRoom) { 
                buildSideRoomInterior(cx, cz, (Direction)((dir + 3) % 4), rooms[i].leftDoorOffset, L, rooms[i].leftRoomSlotTypeL, rooms[i].leftRoomSlotItemL, rooms[i].animLL, rooms[i].leftRoomSlotTypeR, rooms[i].leftRoomSlotItemR, rooms[i].animLR); 
                addRWall(true, rooms[i].leftDoorOpen, false, -5.0f, cx, cz, (Direction)((dir + 3) % 4), wL); 
            } else {
                addRWall(exitLeft, nextOpen, nextLock && exitLeft, -5.0f, cx, cz, (Direction)((dir + 3) % 4), wL);
            }
            
            // Construct Straight Wall or Side-Room
            if (rooms[i].hasFarRoom) { 
                buildSideRoomInterior(cx, cz, dir, rooms[i].farDoorOffset, L, rooms[i].farRoomSlotTypeL, rooms[i].farRoomSlotItemL, rooms[i].animFL, rooms[i].farRoomSlotTypeR, rooms[i].farRoomSlotItemR, rooms[i].animFR); 
                addRWall(true, rooms[i].farDoorOpen, false, -5.0f, cx, cz, dir, wL); 
            } else {
                addRWall(exitStraight, nextOpen, nextLock && exitStraight, -5.0f, cx, cz, dir, wL);
            }
            
            // Construct Right Wall or Side-Room
            if (rooms[i].hasRightRoom) { 
                buildSideRoomInterior(cx, cz, (Direction)((dir + 1) % 4), rooms[i].rightDoorOffset, L, rooms[i].rightRoomSlotTypeL, rooms[i].rightRoomSlotItemL, rooms[i].animRL, rooms[i].rightRoomSlotTypeR, rooms[i].rightRoomSlotItemR, rooms[i].animRR); 
                addRWall(true, rooms[i].rightDoorOpen, false, -5.0f, cx, cz, (Direction)((dir + 1) % 4), wL); 
            } else {
                addRWall(exitRight, nextOpen, nextLock && exitRight, -5.0f, cx, cz, (Direction)((dir + 1) % 4), wL);
            }
        }
        
        // Render internal furniture
        if (!rooms[i].isDupeRoom && !rooms[i].isSeekChase && !rooms[i].isSeekHallway && !rooms[i].isSeekFinale) {
            for (int s = 0; s < 3; s++) { 
                float lz = -2.5f - (s * 2.5f); 
                int t = rooms[i].slotType[s]; 
                
                if (t == 1) buildCabinet(lz, true, L, 0, rooms[i].slotItem[s], cx, cz, dir); 
                else if (t == 2) buildCabinet(lz, false, L, 0, rooms[i].slotItem[s], cx, cz, dir); 
                else if (t == 5) buildDresser(lz, true, rooms[i].animMain[s], rooms[i].slotItem[s], L, 0, true, cx, cz, dir); 
                else if (t == 6) buildDresser(lz, false, rooms[i].animMain[s], rooms[i].slotItem[s], L, 0, true, cx, cz, dir); 
            }
        }
        
        // --- SEEK RENDERING EXTRAS ---
        if(i == seekStartRoom + 9 && rooms[i].isLocked) {
            addRBox(-3.0f, 0.0f, -4.8f, 6.0f, 1.8f, 0.1f, 0.4f, 0.7f, 1.0f, true, 0, 1.5f, cx, cz, dir);
        }
        
        if (rooms[i].isSeekChase) { 
            srand(i * 777); 
            int oT = rand() % 3; 
            float oZ = -5.0f; 
            
            if (oT == 0) {
                addRBox(-3, 0.7f, oZ, 6, 1.1f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L, cx, cz, dir); 
            } else if (oT == 1) {
                addRBox(-3, 0.0f, oZ, 3, 1.8f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L, cx, cz, dir); 
                addRBox(0, 0.7f, oZ, 3, 1.1f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L, cx, cz, dir);
            } else {
                addRBox(0, 0.0f, oZ, 3, 1.8f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L, cx, cz, dir); 
                addRBox(-3, 0.7f, oZ, 3, 1.1f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L, cx, cz, dir);
            } 
            srand(time(NULL)); 
        } 
        else if (rooms[i].isSeekFinale) { 
            addRBox(-2.95f, 0.4f, -8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L, cx, cz, dir); 
            addRBox(2.85f, 0.4f, -8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L, cx, cz, dir); 
            addRBox(-3, 0.0f, -2.0f, 3.5f, 1.8f, 0.4f, 0.05f, 0.05f, 0.05f, true, 0, L, cx, cz, dir); 
            
            rooms[i].pW[0] = 2.6f; rooms[i].pZ[0] = cz - 2.0f; rooms[i].pSide[0] = 1; 
            addRBox(2, 0.8f, -2.2f, 1.0f, 0.2f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L, cx, cz, dir); 
            
            rooms[i].pW[1] = 1.8f; rooms[i].pZ[1] = cz - 3.5f; rooms[i].pSide[1] = 0; 
            addRBox(1.4f, 0.0f, -3.9f, 0.8f, 0.3f, 0.8f, 1.0f, 0.4f, 0.0f, false, 0, L, cx, cz, dir); 
            addRBox(1.6f, 0.3f, -3.7f, 0.4f, 0.4f, 0.4f, 1.0f, 0.8f, 0.0f, false, 0, L, cx, cz, dir); 
            addRBox(-0.5f, 0.0f, -5.0f, 3.5f, 1.8f, 0.4f, 0.05f, 0.05f, 0.05f, true, 0, L, cx, cz, dir); 
            
            rooms[i].pW[2] = -2.6f; rooms[i].pZ[2] = cz - 5.0f; rooms[i].pSide[2] = 1; 
            addRBox(-3, 0.8f, -5.2f, 1.0f, 0.2f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L, cx, cz, dir); 
            
            rooms[i].pW[3] = -1.8f; rooms[i].pZ[3] = cz - 6.5f; rooms[i].pSide[3] = 0; 
            addRBox(-2.2f, 0.0f, -6.9f, 0.8f, 0.3f, 0.8f, 1.0f, 0.4f, 0.0f, false, 0, L, cx, cz, dir); 
            addRBox(-2.0f, 0.3f, -6.7f, 0.4f, 0.4f, 0.4f, 1.0f, 0.8f, 0.0f, false, 0, L, cx, cz, dir); 
            addRBox(-3, 0.0f, -8.0f, 3.5f, 1.8f, 0.4f, 0.05f, 0.05f, 0.05f, true, 0, L, cx, cz, dir); 
            
            rooms[i].pW[4] = 2.6f; rooms[i].pZ[4] = cz - 8.0f; rooms[i].pSide[4] = 1; 
            addRBox(2, 0.8f, -8.2f, 1.0f, 0.2f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L, cx, cz, dir); 
            
            rooms[i].pW[5] = 0.8f; rooms[i].pZ[5] = cz - 9.0f; rooms[i].pSide[5] = 0; 
            addRBox(0.4f, 0.0f, -9.4f, 0.8f, 0.3f, 0.8f, 1.0f, 0.4f, 0.0f, false, 0, L, cx, cz, dir); 
            addRBox(0.6f, 0.3f, -9.2f, 0.4f, 0.4f, 0.4f, 1.0f, 0.8f, 0.0f, false, 0, L, cx, cz, dir); 
        } 
        else if (rooms[i].isSeekHallway) { 
            addRBox(-2.95f, 0.4f, -8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L, cx, cz, dir); 
            addRBox(2.85f, 0.4f, -8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L, cx, cz, dir); 
        } 
        
        globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f; 
    } 
}

// --- LOGICAL ROOM POPULATOR ---
void generateRooms() {
    seekStartRoom = 30 + (rand() % 9);
    
    // PERFECT ROOM 0 COORDINATES
    rooms[0].centerX = 0.0f; 
    rooms[0].centerZ = -5.0f; 
    rooms[0].orientation = NORTH; 
    rooms[0].chosenExitSide = 1;
    rooms[0].minX = -5.0f; 
    rooms[0].maxX = 5.0f; 
    rooms[0].minZ = -10.0f; 
    rooms[0].maxZ = 0.0f;
    rooms[0].isLocked = true; 
    rooms[0].isDupeRoom = false; 
    rooms[0].doorPos = 1;
    
    for (int i = 1; i < TOTAL_ROOMS; i++) {
        RoomSetup& prev = rooms[i - 1];
        
        rooms[i].isLocked = false; 
        rooms[i].isJammed = false; 
        rooms[i].hasSeekEyes = false; 
        rooms[i].isSeekHallway = false; 
        rooms[i].isSeekChase = false; 
        rooms[i].isSeekFinale = false; 
        rooms[i].seekEyeCount = 0;
        
        bool iSE = (i >= seekStartRoom - 5 && i <= seekStartRoom + 9);
        bool iSCE = (i >= seekStartRoom && i <= seekStartRoom + 9); 
        
        rooms[i].lightLevel = (i > 0 && rand() % 100 < 8 && !iSE) ? 0.3f : 1.0f; 
        
        for (int s = 0; s < 3; s++) { 
            rooms[i].drawerOpen[s] = false; 
            rooms[i].slotItem[s] = 0; 
            rooms[i].animMain[s] = 0.0f; 
            rooms[i].slotType[s] = 0; 
        }
        
        rooms[i].isDupeRoom = (!iSCE && i > 1 && (rand() % 100 < 15)); 
        
        if (rooms[i].isDupeRoom) {
            rooms[i].hasLeftRoom = false; 
            rooms[i].hasFarRoom = false; 
            rooms[i].hasRightRoom = false;
            rooms[i].correctDupePos = rand() % 3; 
            
            if (rooms[i].correctDupePos == 0) prev.chosenExitSide = 0; 
            else if (rooms[i].correctDupePos == 1) prev.chosenExitSide = 1; 
            else prev.chosenExitSide = 2; 
            
            int dN = getDisplayRoom(i);
            int f1 = dN + (rand() % 3 + 1);
            int f2 = dN - (rand() % 3 + 1); 
            
            if (f2 <= 0) f2 = dN + 4; 
            
            rooms[i].dupeNumbers[0] = f1; 
            rooms[i].dupeNumbers[1] = f2; 
            rooms[i].dupeNumbers[2] = dN + 5; 
            rooms[i].dupeNumbers[rooms[i].correctDupePos] = dN;
        } else if (i == LIBRARY_ROOM || iSCE) {
            prev.chosenExitSide = 1; 
        } else {
            prev.chosenExitSide = rand() % 3; 
        }
        
        int prevDir = (int)prev.orientation;
        if (prev.chosenExitSide == 0) {
            rooms[i].orientation = (Direction)((prevDir + 3) % 4); 
        } else if (prev.chosenExitSide == 1) {
            rooms[i].orientation = prev.orientation; 
        } else {
            rooms[i].orientation = (Direction)((prevDir + 1) % 4);                          
        }
        
        float stepX = 0;
        float stepZ = 0; 
        Direction exitGlobalDir = prev.orientation;
        
        if (prev.chosenExitSide == 0) {
            exitGlobalDir = (Direction)((prevDir + 3) % 4);
        } else if (prev.chosenExitSide == 2) {
            exitGlobalDir = (Direction)((prevDir + 1) % 4);
        }
        
        if (exitGlobalDir == NORTH) stepZ = -10.0f; 
        else if (exitGlobalDir == EAST) stepX = 10.0f; 
        else if (exitGlobalDir == SOUTH) stepZ = 10.0f; 
        else if (exitGlobalDir == WEST) stepX = -10.0f;
        
        rooms[i].centerX = prev.centerX + stepX; 
        rooms[i].centerZ = prev.centerZ + stepZ;
        
        rooms[i].minX = rooms[i].centerX - 5.0f; 
        rooms[i].maxX = rooms[i].centerX + 5.0f; 
        rooms[i].minZ = rooms[i].centerZ - 5.0f; 
        rooms[i].maxZ = rooms[i].centerZ + 5.0f;

        rooms[i].hasEyes = (!iSE && i > 2 && !rooms[i].isDupeRoom && rand() % 100 < 8); 
        
        if (rooms[i].hasEyes) { 
            rotateVertexRelative(0.0f, -5.0f, rooms[i].centerX, rooms[i].centerZ, rooms[i].orientation, rooms[i].eyesX, rooms[i].eyesZ); 
            rooms[i].eyesY = 0.9f; 
        }
        
        rooms[i].hasLeftRoom = false; 
        rooms[i].hasFarRoom = false; 
        rooms[i].hasRightRoom = false;
        
        if (!rooms[i].isDupeRoom && !iSCE) {
            bool bS = false; 
            for (int s = 0; s < 3; s++) { 
                int r = rand() % 100; 
                if (r < 25) {
                    rooms[i].slotType[s] = 1; 
                } else if (r < 50) {
                    rooms[i].slotType[s] = 2; 
                } else if (r < 75) { 
                    rooms[i].slotType[s] = 5; 
                    if (!bS && rand() % 100 < 15) { rooms[i].slotItem[s] = 2; bS = true; } 
                    else if (rand() % 100 < 30) rooms[i].slotItem[s] = 3; 
                } else { 
                    rooms[i].slotType[s] = 6; 
                    if (!bS && rand() % 100 < 15) { rooms[i].slotItem[s] = 2; bS = true; } 
                    else if (rand() % 100 < 30) rooms[i].slotItem[s] = 3; 
                } 
            }
            
            rooms[i].hasLeftRoom = (rooms[i].chosenExitSide != 0) && (rand() % 100 < 60); 
            rooms[i].hasFarRoom  = (rooms[i].chosenExitSide != 1) && (rand() % 100 < 60); 
            rooms[i].hasRightRoom = (rooms[i].chosenExitSide != 2) && (rand() % 100 < 60);
            
            auto popSideRoom = [&](int* typeL, int* itemL, int* typeR, int* itemR) {
                bool cL = false, cR = false;
                for(int s = 0; s < 3; s++) {
                    if(s == 1) { 
                        typeL[s] = 99; itemL[s] = 0; 
                        typeR[s] = 99; itemR[s] = 0; 
                        continue; 
                    } 
                    if(rand() % 100 < 45) {
                        int r = rand() % 100; 
                        typeL[s] = (r < 25) ? 1 : ((r < 50) ? 3 : ((r < 75) ? 5 : 7)); 
                        
                        if(typeL[s] == 7) { 
                            if(cL) typeL[s] = 5; 
                            else cL = true; 
                        } 
                        itemL[s] = (typeL[s] >= 3 && typeL[s] <= 6 && rand() % 100 < 30) ? 3 : 0; 
                    } else { 
                        typeL[s] = 0; itemL[s] = 0; 
                    }
                    
                    if(rand() % 100 < 45) {
                        int r = rand() % 100; 
                        typeR[s] = (r < 33) ? 2 : ((r < 66) ? 6 : 8); 
                        
                        if(typeR[s] == 8) { 
                            if(cR) typeR[s] = 6; 
                            else cR = true; 
                        } 
                        itemR[s] = (typeR[s] >= 3 && typeR[s] <= 6 && rand() % 100 < 30) ? 3 : 0; 
                    } else { 
                        typeR[s] = 0; itemR[s] = 0; 
                    }
                }
            };

            if (rooms[i].hasLeftRoom) { 
                rooms[i].leftDoorOffset = -3.0f - (rand() % 40) / 10.0f; 
                popSideRoom(rooms[i].leftRoomSlotTypeL, rooms[i].leftRoomSlotItemL, rooms[i].leftRoomSlotTypeR, rooms[i].leftRoomSlotItemR); 
            }
            if (rooms[i].hasRightRoom) { 
                rooms[i].rightDoorOffset = -3.0f - (rand() % 40) / 10.0f; 
                popSideRoom(rooms[i].rightRoomSlotTypeL, rooms[i].rightRoomSlotItemL, rooms[i].rightRoomSlotTypeR, rooms[i].rightRoomSlotItemR); 
            }
            if (rooms[i].hasFarRoom) { 
                rooms[i].farDoorOffset = -3.0f - (rand() % 40) / 10.0f; 
                popSideRoom(rooms[i].farRoomSlotTypeL, rooms[i].farRoomSlotItemL, rooms[i].farRoomSlotTypeR, rooms[i].farRoomSlotItemR); 
            }
        }
    }
}
