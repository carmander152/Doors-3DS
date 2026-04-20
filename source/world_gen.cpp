#include "world_gen.h"
#include "render_utils.h"
#include "physics.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

void buildLamp(float x, float z, float L) { 
    addBox(x-0.15f, 0.46f, z-0.15f, 0.3f, 0.05f, 0.3f, 0.3f, 0.15f, 0.05f, false, 0, L); 
    addBox(x-0.05f, 0.51f, z-0.05f, 0.1f, 0.2f, 0.1f, 0.2f, 0.3f, 0.4f, false, 0, L); 
    addBox(x-0.2f, 0.71f, z-0.2f, 0.4f, 0.25f, 0.4f, 0.9f, 0.9f, 0.8f, false, 0, L); 
}

void buildCabinet(float zC, bool isL, float L, float offX, int item) {
    float bX=(isL?-2.95f:2.85f)+offX, tX=(isL?-2.95f:2.15f)+offX, fX=(isL?-2.25f:2.15f)+offX, hX=(isL?fX+0.1f:fX-0.03f); 
    
    addBox(bX, 0, zC-0.5f, 0.1f, 1.5f, 1.0f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(tX, 1.5f, zC-0.5f, 0.8f, 0.1f, 1.0f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(tX, 0, zC-0.5f, 0.8f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(tX, 0, zC+0.4f, 0.8f, 1.5f, 0.1f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(fX, 0, zC-0.4f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(fX, 0, zC+0.05f, 0.1f, 1.5f, 0.35f, 0.3f, 0.18f, 0.1f, false, 0, L); 
    addBox(hX, 0.6f, zC-0.15f, 0.03f, 0.15f, 0.04f, 0.9f, 0.75f, 0.1f, false, 0, L); 
    addBox(hX, 0.6f, zC+0.11f, 0.03f, 0.15f, 0.04f, 0.9f, 0.75f, 0.1f, false, 0, L); 
    
    if(hideState != IN_CABINET) {
        addBox((isL?-2.85f:2.25f)+offX, 0.01f, zC-0.4f, 0.6f, 1.48f, 0.8f, 0, 0, 0, false, 0, L); 
    }
    
    collisions.push_back({(isL?-2.9f:2.1f)+offX, 0.0f, zC-0.5f, (isL?-2.1f:2.9f)+offX, 1.5f, zC+0.5f, 1});
}

void buildBed(float zC, bool isL, int item, float L, float offX) {
    float x=(isL?-2.95f:1.55f)+offX, sX=(isL?-1.65f:1.55f)+offX, pX=(isL?-2.65f:2.15f)+offX; 
    
    addBox(x, 0.4f, zC+1.25f, 1.4f, 0.1f, -2.5f, 0.4f, 0.1f, 0.1f, true, 0, L); 
    addBox(x, 0, zC+1.25f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L); 
    addBox(x+1.3f, 0, zC+1.25f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L);
    addBox(x, 0, zC-1.15f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L); 
    addBox(x+1.3f, 0, zC-1.15f, 0.1f, 0.4f, -0.1f, 0.2f, 0.1f, 0.05f, true, 0, L); 
    addBox(sX, 0.2f, zC+1.25f, 0.1f, 0.2f, -2.5f, 0.4f, 0.1f, 0.1f, true, 0, L); 
    addBox(pX, 0.5f, zC+1.0f, 0.5f, 0.08f, -0.6f, 0.9f, 0.9f, 0.9f, false, 0, L); 
    
    if (item == 1) { 
        float ix = (isL ? -2.2f : 2.1f) + offX; 
        addBox(ix, 0.52f, zC-0.1f, 0.14f, 0.035f, 0.035f, 0.9f, 0.75f, 0.1f, false, 0, L); 
        addBox(ix+0.1f, 0.52f, zC-0.13f, 0.07f, 0.035f, 0.1f, 0.9f, 0.75f, 0.1f, false, 0, L); 
        addBox(ix+0.03f, 0.52f, zC-0.065f, 0.035f, 0.035f, 0.05f, 0.9f, 0.75f, 0.1f, false, 0, L); 
    } 
    else if (item == 3) {
        addBox((isL?-2.2f:2.1f)+offX+0.02f, 0.52f, zC-0.01f, 0.04f, 0.005f, 0.04f, 1.0f, 0.85f, 0.0f, false, 0, L);
    }
    
    if(!isBuildingEntities) {
        collisions.push_back({x, 0.0f, zC-1.25f, x+1.4f, 0.6f, zC+1.25f, 2});
    }
}

void buildDresser(float zC, bool isL, float openFactor, int item, float L, float offX, bool hasLamp) {
    float frX = (isL ? -2.95f : 2.45f) + offX;
    float oOff = openFactor * (isL ? 0.35f : -0.35f);
    float trX = (isL ? (-2.9f + oOff) : (2.4f + oOff)) + offX;
    float hX = (isL ? (-2.4f + oOff) : (2.35f + oOff)) + offX;
    float iX = (isL ? (-2.6f + oOff) : (2.5f + oOff)) + offX;
    
    addBox(frX+0.05f, 0, zC-0.35f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L); 
    addBox(frX+0.05f, 0, zC+0.3f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L);
    addBox(frX+0.4f, 0, zC-0.35f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L); 
    addBox(frX+0.4f, 0, zC+0.3f, 0.05f, 0.2f, 0.05f, 0.2f, 0.1f, 0.05f, false, 0, L);
    addBox(frX, 0.2f, zC-0.4f, 0.5f, 0.3f, 0.8f, 0.3f, 0.15f, 0.1f, true, 3, L); 
    addBox(trX, 0.3f, zC-0.35f, 0.5f, 0.15f, 0.7f, 0.25f, 0.12f, 0.08f, false, 0, L); 
    addBox(hX, 0.35f, zC-0.1f, 0.05f, 0.05f, 0.2f, 0.8f, 0.8f, 0.8f, false, 0, L);
    
    if(openFactor > 0.1f) { 
        if (item == 1) { 
            addBox(iX, 0.46f, zC-0.1f, 0.14f, 0.035f, 0.035f, 0.9f, 0.75f, 0.1f, false, 0, L); 
            addBox(iX+0.1f, 0.46f, zC-0.13f, 0.07f, 0.035f, 0.1f, 0.9f, 0.75f, 0.1f, false, 0, L); 
            addBox(iX+0.03f, 0.46f, zC-0.065f, 0.035f, 0.035f, 0.05f, 0.9f, 0.75f, 0.1f, false, 0, L); 
        } 
        else if (item == 2) {
            addBox(iX, 0.46f, zC-0.05f, 0.15f, 0.02f, 0.08f, 0.8f, 0.6f, 0.4f, false, 0, L); 
        } 
        else if (item == 3) {
            addBox(iX+0.02f, 0.46f, zC-0.05f, 0.04f, 0.005f, 0.04f, 1.0f, 0.85f, 0.0f, false, 0, L); 
        } 
    }
    
    if(hasLamp) {
        buildLamp((isL?-2.7f:2.7f)+offX, zC, L);
    }
}

void buildChest(float x, float z, float openFactor, float L) {
    bool isOpen = (openFactor > 0.5f);
    
    addBox(x-0.4f, 0, z-0.3f, 0.8f, 0.4f, 0.6f, 0.3f, 0.15f, 0.05f, true, 0, L); 
    addBox(x-0.42f, 0, z-0.32f, 0.05f, 0.4f, 0.05f, 0.8f, 0.7f, 0.1f, false, 0, L); 
    addBox(x+0.37f, 0, z-0.32f, 0.05f, 0.4f, 0.05f, 0.8f, 0.7f, 0.1f, false, 0, L);
    
    if (!isOpen) { 
        addBox(x-0.4f, 0.4f, z-0.3f, 0.8f, 0.2f, 0.6f, 0.35f, 0.18f, 0.08f, false, 0, L); 
        addBox(x-0.05f, 0.3f, z+0.3f, 0.1f, 0.15f, 0.05f, 0.8f, 0.8f, 0.8f, false, 0, L); 
    } else { 
        addBox(x-0.4f, 0.4f, z-0.4f, 0.8f, 0.6f, 0.1f, 0.35f, 0.18f, 0.08f, false, 0, L); 
        addBox(x-0.2f, 0.35f, z-0.1f, 0.4f, 0.05f, 0.2f, 1.0f, 0.85f, 0.0f, false, 0, L); 
    }
}

void addWallWithDoors(float z, bool lD, bool lO, bool cD, bool cO, bool rD, bool rO, int rm, float L) {
    float wallU = 0.032f, wallV = 0.532f, wallUW = 0.436f, wallVH = 0.436f;
    float texScale = 2.4f, r = 1.0f, g = 1.0f, b = 1.0f; 
    
    addTiledSurface(-3.0f, 0.4f, z, 0.4f, 1.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, true); 
    addTiledSurface(-3.0f, 0.0f, z, 0.4f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, false);
    
    if (!lD) { 
        addTiledSurface(-2.6f, 0.4f, z, 1.2f, 1.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, true); 
        addTiledSurface(-2.6f, 0.0f, z, 1.2f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, false); 
    } else {
        addTiledSurface(-2.6f, 1.4f, z, 1.2f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, false);
    }
    
    addTiledSurface(-1.4f, 0.4f, z, 0.8f, 1.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, true); 
    addTiledSurface(-1.4f, 0.0f, z, 0.8f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, false);
    
    if (!cD) { 
        addTiledSurface(-0.6f, 0.4f, z, 1.2f, 1.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, true); 
        addTiledSurface(-0.6f, 0.0f, z, 1.2f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, false); 
    } else {
        addTiledSurface(-0.6f, 1.4f, z, 1.2f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, false);
    }
    
    addTiledSurface(0.6f, 0.4f, z, 0.8f, 1.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, true); 
    addTiledSurface(0.6f, 0.0f, z, 0.8f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, false);
    
    if (!rD) { 
        addTiledSurface(1.4f, 0.4f, z, 1.2f, 1.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, true); 
        addTiledSurface(1.4f, 0.0f, z, 1.2f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, false); 
    } else {
        addTiledSurface(1.4f, 1.4f, z, 1.2f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, false);
    }
    
    addTiledSurface(2.6f, 0.4f, z, 0.4f, 1.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, true); 
    addTiledSurface(2.6f, 0.0f, z, 0.4f, 0.4f, -0.2f, wallU, wallV, wallUW, wallVH, texScale, r,g,b, L, false);
    
    auto bb = [&](float bx, float bw) { 
        addBox(bx, 0.0f, z, bw, 0.12f, 0.04f, 0.12f, 0.06f, 0.03f, false, 0, L); 
        addBox(bx, 0.0f, z-0.2f, bw, 0.12f, -0.04f, 0.12f, 0.06f, 0.03f, false, 0, L); 
    };
    
    bb(-3.0f, 0.4f); 
    if(!lD) bb(-2.6f, 1.2f); 
    bb(-1.4f, 0.8f); 
    if(!cD) bb(-0.6f, 1.2f); 
    bb(0.6f, 0.8f); 
    if(!rD) bb(1.4f, 1.2f); 
    bb(2.6f, 0.4f);

    auto dr = [&](float dx, bool o) {
        addBox(dx, 0, z, 0.05f, 1.4f, -0.2f, 0.15f, 0.08f, 0.04f, false, 0, L); 
        addBox(dx + 1.15f, 0, z, 0.05f, 1.4f, -0.2f, 0.15f, 0.08f, 0.04f, false, 0, L); 
        addBox(dx + 0.05f, 1.35f, z, 1.1f, 0.05f, -0.2f, 0.15f, 0.08f, 0.04f, false, 0, L); 
        
        if (!o) { 
            addBox(dx, 0, z, 1.2f, 1.4f, -0.1f, 0.15f, 0.08f, 0.05f, true, 0, L); 
            addBox(dx+0.4f, 1.1f, z+0.02f, 0.4f, 0.12f, 0.02f, 0.8f, 0.7f, 0.2f, false, 0, L); 
            addBox(dx+1.05f, 0.7f, z+0.02f, 0.05f, 0.15f, 0.03f, 0.6f, 0.6f, 0.6f, false, 0, L); 
            if(rooms[rm].isLocked) {
                addBox(dx+0.9f, 0.6f, z+0.05f, 0.2f, 0.2f, 0.05f, 0.8f, 0.8f, 0.8f, false, 0, L);
            }
        } else { 
            addBox(dx, 0, z, 0.1f, 1.4f, -1.2f, 0.3f, 0.15f, 0.08f, false, 0, L); 
            addBox(dx+0.1f, 1.1f, z-0.8f, 0.02f, 0.12f, 0.4f, 0.8f, 0.7f, 0.2f, false, 0, L); 
            addBox(dx+0.1f, 0.7f, z-1.05f, 0.03f, 0.15f, 0.05f, 0.6f, 0.6f, 0.6f, false, 0, L); 
        }
    }; 
    
    if(lD) dr(-2.6f, lO); 
    if(cD) dr(-0.6f, cO); 
    if(rD) dr(1.4f, rO);
}

void buildWorld(int cChunk, int pRm) {
    world_mesh_colored.clear(); 
    world_mesh_textured.clear(); 
    collisions.clear();
    
    // Texture constants
    float floorU = 0.032f, floorV = 0.032f, floorUW = 0.436f, floorVH = 0.436f;
    float wallU = 0.032f, wallV = 0.532f, wallUW = 0.436f, wallVH = 0.436f;     
    float cR = 1.0f, cG = 1.0f, cB = 1.0f, floorScale = 2.4f, wallScale = 2.4f;  

    int st = pRm; 
    int en = pRm + 1; 
    
    if (pRm == -1) { 
        st = -1; 
        en = doorOpen[0] ? 1 : 0; 
    } else { 
        if (pRm > 0 && doorOpen[pRm]) st = pRm - 1; 
        if (pRm == 0 && doorOpen[0]) st = -1; 
        if (pRm + 1 < TOTAL_ROOMS && doorOpen[pRm + 1]) en = pRm + 2; 
    }
    
    if (pRm >= seekStartRoom - 1 && pRm <= seekStartRoom + 3) { 
        st = seekStartRoom - 1; 
        en = seekStartRoom + 4; 
    }
    
    if (st < -1) st = -1; 
    if (en > TOTAL_ROOMS - 1) en = TOTAL_ROOMS - 1;

    // Room 0 (Lobby)
    if(st <= -1){
        globalTintR = 1.0f; globalTintG = 1.0f; globalTintB = 1.0f;
        addTiledSurface(-2, 0, 5, 4, 0.01f, 4, floorU,floorV,floorUW,floorVH, floorScale, cR,cG,cB, 1.0f, false); 
        addTiledSurface(-2, 2, 5, 4, 0.01f, 4, floorU,floorV,floorUW,floorVH, floorScale, cR,cG,cB, 1.0f, false); 
        addBox(-2, 0, 9, 4, 2, 0.1f, 0.4f, 0.3f, 0.2f, true); 
        addBox(-2, 0, 5, 0.1f, 2, 4, 0.4f, 0.3f, 0.2f, true); 
        addBox(1.9f, 0, 5, 0.1f, 2, 4, 0.4f, 0.3f, 0.2f, true);   
        addBox(1.8f, 0.6f, 6.5f, 0.15f, 0.3f, 0.2f, 0.1f, 0.1f, 0.1f, false); 
        addBox(1.75f, 0.7f, 6.55f, 0.05f, 0.1f, 0.1f, 0, 0.8f, 0, false, 0, 1.5f); 
        addBox(-2.0f - elevatorDoorOffset, 0, 5.05f, 2, 2, 0.1f, 0.6f, 0.6f, 0.6f, true); 
        addBox(0.0f + elevatorDoorOffset, 0, 5.05f, 2, 2, 0.1f, 0.6f, 0.6f, 0.6f, true);  
        
        if (elevatorDoorOffset < 0.05f) {
            addBox(-0.02f, 0, 5.04f, 0.04f, 2, 0.12f, 0, 0, 0, false); 
        }
        if (elevatorClosing) {
            collisions.push_back({-2.0f, 0.0f, 4.8f, 2.0f, 2.0f, 5.1f, 0});
        }
        
        addTiledSurface(-6, 0, 5, 12, 0.01f, -15, floorU,floorV,floorUW,floorVH, floorScale, cR,cG,cB, 1.0f, false); 
        addTiledSurface(-6, 1.8f, 5, 12, 0.01f, -15, floorU,floorV,floorUW,floorVH, floorScale, cR,cG,cB, 1.0f, false); 
        addBox(-6, 0, 5, 0.1f, 1.8f, -15, 0.3f, 0.3f, 0.3f, true); 
        addBox(6, 0, 5, 0.1f, 1.8f, -15, 0.3f, 0.3f, 0.3f, true);  
        addBox(-6, 0, -10, 3, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true); 
        addBox(3, 0, -10, 3, 1.8f, 0.1f, 0.25f, 0.2f, 0.15f, true); 
        addBox(-6, 0, 4.9f, 4, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true); 
        addBox(2, 0, 4.9f, 4, 1.8f, 0.1f, 0.25f, 0.15f, 0.1f, true);  
        addBox(-6, 0, -7, 3.5f, 0.8f, -0.8f, 0.3f, 0.15f, 0.1f, true); 
        addBox(-3.3f, 0, -7.8f, 0.8f, 0.8f, -1.0f, 0.3f, 0.15f, 0.1f, true); 
        addBox(-2.5f, 0.1f, -8.6f, 1, 0.05f, -1.4f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.15f, -8.6f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-1.55f, 0.15f, -8.6f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.15f, -9.95f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-1.55f, 0.15f, -9.95f, 0.05f, 0.45f, -0.05f, 0.8f, 0.7f, 0.2f, false); 
        addBox(-2.5f, 0.6f, -8.6f, 1, 0.05f, -1.4f, 0.8f, 0.7f, 0.2f, true); 
        
        if (!lobbyKeyPickedUp) { 
            addBox(-4.8f, 0.9f, -9.9f, 0.2f, 0.2f, 0.05f, 0.3f, 0.2f, 0.1f, false); 
            addBox(-4.72f, 0.75f, -9.86f, 0.035f, 0.1f, 0.035f, 1.0f, 0.84f, 0.0f, false); 
        }
        
        addBox(-5.9f, 0.0f, 5.0f, 0.04f, 0.12f, -15.0f, 0.12f, 0.06f, 0.03f, false); 
        addBox(5.86f, 0.0f, 5.0f, 0.04f, 0.12f, -15.0f, 0.12f, 0.06f, 0.03f, false); 
        addBox(-6.0f, 0.0f, 5.0f, 4.0f, 0.12f, -0.04f, 0.12f, 0.06f, 0.03f, false); 
        addBox(2.0f, 0.0f, 5.0f, 4.0f, 0.12f, -0.04f, 0.12f, 0.06f, 0.03f, false); 
        addBox(-6.0f, 0.0f, -10.0f, 3.0f, 0.12f, 0.04f, 0.12f, 0.06f, 0.03f, false); 
        addBox(3.0f, 0.0f, -10.0f, 3.0f, 0.12f, 0.04f, 0.12f, 0.06f, 0.03f, false);
    }

    if (st < 0) st = 0; 
    
    // Main Room Loop
    for(int i = st; i <= en; i++) {
        float z = -10 - (i * 10);
        float L = rooms[i].lightLevel;
        float wL = (i > 0) ? rooms[i-1].lightLevel : 1.0f; 
        bool tAN = (!rooms[i].isSeekChase && !rooms[i].isSeekHallway && !rooms[i].isSeekFinale && i == pRm + 2);
        
        bool isInteriorVisible = true;
        if (!seekActive) {
            if (i > pRm && i >= 0 && !doorOpen[i] && i != seekStartRoom + 1 && i != seekStartRoom + 2) isInteriorVisible = false;
            if (i < pRm && i >= 0 && i + 1 < TOTAL_ROOMS && !doorOpen[i + 1] && (i + 1) != seekStartRoom + 1 && (i + 1) != seekStartRoom + 2) isInteriorVisible = false;
        }

        // Room 49 (Library / Figure Room)
        if (i == 49) {
            float cL = rooms[i].lightLevel; 
            globalTintR=0.9f; globalTintG=0.8f; globalTintB=0.6f; 
            
            addWallWithDoors(z - 20.0f, false, false, true, doorOpen[i], false, false, i, cL);
            
            if (isInteriorVisible) {
                addTiledSurface(-6.0f, 0.0f, z, 12.0f, 0.01f, -20.0f, floorU,floorV,floorUW,floorVH, floorScale, cR,cG,cB, cL, false); 
                addTiledSurface(-6.0f, 3.6f, z, 12.0f, 0.01f, -20.0f, floorU,floorV,floorUW,floorVH, floorScale, cR,cG,cB, cL, false); 
                addTiledSurface(-6.1f, 0.0f, z, 0.1f, 3.6f, -20.0f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, cL, true); 
                addTiledSurface(6.0f, 0.0f, z, 0.1f, 3.6f, -20.0f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, cL, true);  
                addTiledSurface(-6.0f, 0.0f, z, 4.6f, 3.6f, -0.1f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, cL, true); 
                addTiledSurface(1.4f, 0.0f, z, 4.6f, 3.6f, -0.1f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, cL, true);
                addTiledSurface(-1.4f, 1.8f, z, 2.8f, 1.8f, -0.1f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, cL, true); 
                
                addBox(-6.0f, 1.8f, z-2.0f, 3.0f, 0.1f, -16.0f, 0.25f, 0.15f, 0.1f, true, 0, cL); 
                addBox(3.0f, 1.8f, z-2.0f, 3.0f, 0.1f, -16.0f, 0.25f, 0.15f, 0.1f, true, 0, cL);  
                addBox(-3.0f, 1.8f, z-15.0f, 6.0f, 0.1f, -3.0f, 0.25f, 0.15f, 0.1f, true, 0, cL); 
                addBox(-3.1f, 1.9f, z-2.0f, 0.1f, 0.6f, -13.0f, 0.15f, 0.08f, 0.05f, true, 0, cL); 
                addBox(3.0f, 1.9f, z-2.0f, 0.1f, 0.6f, -13.0f, 0.15f, 0.08f, 0.05f, true, 0, cL);
                addBox(-3.0f, 1.9f, z-14.9f, 6.0f, 0.6f, -0.1f, 0.15f, 0.08f, 0.05f, true, 0, cL);
                
                // Bookshelves
                for(int stt = 0; stt < 12; stt++) { 
                    float sY = stt * 0.15f; 
                    float sZ = z - 2.0f - (stt * 0.25f);
                    addBox(-5.5f, 0.0f, sZ, 2.0f, sY+0.15f, -0.25f, 0.2f, 0.12f, 0.08f, true, 0, cL); 
                    addBox(3.5f, 0.0f, sZ, 2.0f, sY+0.15f, -0.25f, 0.2f, 0.12f, 0.08f, true, 0, cL); 
                }
                
                addBox(-1.5f, 0.0f, z-7.0f, 3.0f, 0.6f, -4.0f, 0.3f, 0.18f, 0.1f, true, 0, cL); 
                addBox(-1.6f, 0.6f, z-6.9f, 3.2f, 0.1f, -4.2f, 0.2f, 0.1f, 0.05f, true, 0, cL); 
                buildLamp(-1.2f, z-7.5f, cL); 
                buildLamp(1.2f, z-7.5f, cL);
                
                addTiledSurface(-2.5f, 0.0f, z-12.0f, 1.0f, 1.8f, -4.0f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, cL, true);
                addTiledSurface(1.5f, 0.0f, z-12.0f, 1.0f, 1.8f, -4.0f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, cL, true);
                addBox(-6.0f, 0.0f, z, 0.04f, 0.12f, -20.0f, 0.12f, 0.06f, 0.03f, false, 0, cL); 
                addBox(5.96f, 0.0f, z, 0.04f, 0.12f, -20.0f, 0.12f, 0.06f, 0.03f, false, 0, cL);
                addBox(-6.0f, 0.0f, z, 4.6f, 0.12f, -0.04f, 0.12f, 0.06f, 0.03f, false, 0, cL); 
                addBox(1.4f, 0.0f, z, 4.6f, 0.12f, -0.04f, 0.12f, 0.06f, 0.03f, false, 0, cL);
            }
            globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f; continue; 
        }

        // Global Tints based on state
        if (seekState == 1) {
            globalTintR=1.0f; globalTintG=0.2f; globalTintB=0.2f;
        } else {
            globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f;
        }
        
        // Seek blockages
        if(i == seekStartRoom + 9 && rooms[i].isLocked) {
            addBox(-3.0f, 0, z+0.2f, 6.0f, 1.8f, 0.1f, 0.4f, 0.7f, 1.0f, true, 0, 1.5f);
        }
        
        // Build generic walls
        if (!(i == seekStartRoom + 1 || i == seekStartRoom + 2)) { 
            if (rooms[i].isDupeRoom) { 
                if (pRm >= i) {
                    addWallWithDoors(z, (rooms[i].correctDupePos == 0), ((rooms[i].correctDupePos == 0) && doorOpen[i]), (rooms[i].correctDupePos == 1), ((rooms[i].correctDupePos == 1) && doorOpen[i]), (rooms[i].correctDupePos == 2), ((rooms[i].correctDupePos == 2) && doorOpen[i]), i, wL); 
                } else {
                    addWallWithDoors(z, true, (rooms[i].correctDupePos == 0 && doorOpen[i]), true, (rooms[i].correctDupePos == 1 && doorOpen[i]), true, (rooms[i].correctDupePos == 2 && doorOpen[i]), i, wL); 
                }
            } else { 
                addWallWithDoors(z, (rooms[i].doorPos == 0), ((rooms[i].doorPos == 0) && doorOpen[i]), (rooms[i].doorPos == 1), ((rooms[i].doorPos == 1) && doorOpen[i]), (rooms[i].doorPos == 2), ((rooms[i].doorPos == 2) && doorOpen[i]), i, wL); 
            } 
        }
        
        // Update tints for eyes
        if (seekState == 1) {
            globalTintR=1.0f; globalTintG=0.2f; globalTintB=0.2f;
        } else if (rooms[i].hasEyes) {
            globalTintR=0.8f; globalTintG=0.3f; globalTintB=1.0f;
        } else {
            globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f;
        }

        // Draw Seek Eyes (WITH DOOR AVOIDANCE MATH)
        bool rE = !(rooms[i].isSeekChase || rooms[i].hasSeekEyes) || (i >= pRm && i <= pRm + 1);
        if (isInteriorVisible) {
            if (rE && (rooms[i].hasSeekEyes || rooms[i].isSeekChase)) { 
                srand(i * 12345); 
                int wC = rooms[i].hasSeekEyes ? rooms[i].seekEyeCount : 15; 
                for(int e = 0; e < wC; e++) { 
                    bool iL = (rand() % 2 == 0); 
                    float randomZOffset = 0.5f + (rand() % 90) / 10.0f;
                    
                    // --- THE FIX: Check if the eye hits a side door! ---
                    if (iL && rooms[i].hasLeftRoom && fabsf(-randomZOffset - rooms[i].leftDoorOffset) < 1.8f) continue;
                    if (!iL && rooms[i].hasRightRoom && fabsf(-randomZOffset - rooms[i].rightDoorOffset) < 1.8f) continue;

                    float eZ = z - randomZOffset;
                    float eY = 0.2f + (rand() % 160) / 100.0f; 
                    
                    if (iL) { 
                        addBox(-2.95f, eY, eZ, 0.1f, 0.3f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L); 
                        addBox(-2.88f, eY+0.05f, eZ+0.05f, 0.06f, 0.2f, 0.3f, 0.95f, 0.95f, 0.95f, false, 0, L); 
                        addBox(-2.84f, eY+0.1f, eZ+0.12f, 0.04f, 0.1f, 0.16f, 0, 0, 0, false, 0, 1.5f); 
                    } else { 
                        addBox(2.85f, eY, eZ, 0.1f, 0.3f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L); 
                        addBox(2.82f, eY+0.05f, eZ+0.05f, 0.06f, 0.2f, 0.3f, 0.95f, 0.95f, 0.95f, false, 0, L); 
                        addBox(2.80f, eY+0.1f, eZ+0.12f, 0.04f, 0.1f, 0.16f, 0, 0, 0, false, 0, 1.5f); 
                    } 
                } 
                srand(time(NULL)); 
            }
        }

        // Special Seek Rooms
        if (rooms[i].isSeekChase) { 
            srand(i * 777); 
            int oT = rand() % 3; 
            float oZ = z - 5.0f; 
            
            if (oT == 0) {
                addBox(-3, 0.7f, oZ, 6, 1.1f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L); 
            } else if (oT == 1) {
                addBox(-3, 0, oZ, 3, 1.8f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L); 
                addBox(0, 0.7f, oZ, 3, 1.1f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L);
            } else {
                addBox(0, 0, oZ, 3, 1.8f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L); 
                addBox(-3, 0.7f, oZ, 3, 1.1f, 0.4f, 0.2f, 0.15f, 0.1f, true, 0, L);
            } 
            srand(time(NULL)); 
        } 
        else if (rooms[i].isSeekFinale) { 
            addBox(-2.95f, 0.4f, z-8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L); 
            addBox(2.85f, 0.4f, z-8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L); 
            addBox(-3, 0, z-2, 3.5f, 1.8f, 0.4f, 0.05f, 0.05f, 0.05f, true, 0, L); 
            
            rooms[i].pW[0] = 2.6f; rooms[i].pZ[0] = z - 2.0f; rooms[i].pSide[0] = 1; 
            addBox(2, 0.8f, z-2.2f, 1.0f, 0.2f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L); 
            
            rooms[i].pW[1] = 1.8f; rooms[i].pZ[1] = z - 3.5f; rooms[i].pSide[1] = 0; 
            addBox(1.4f, 0, z-3.9f, 0.8f, 0.3f, 0.8f, 1, 0.4f, 0, false, 0, L); 
            addBox(1.6f, 0.3f, z-3.7f, 0.4f, 0.4f, 0.4f, 1, 0.8f, 0, false, 0, L); 
            addBox(-0.5f, 0, z-5, 3.5f, 1.8f, 0.4f, 0.05f, 0.05f, 0.05f, true, 0, L); 
            
            rooms[i].pW[2] = -2.6f; rooms[i].pZ[2] = z - 5.0f; rooms[i].pSide[2] = 1; 
            addBox(-3, 0.8f, z-5.2f, 1.0f, 0.2f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L); 
            
            rooms[i].pW[3] = -1.8f; rooms[i].pZ[3] = z - 6.5f; rooms[i].pSide[3] = 0; 
            addBox(-2.2f, 0, z-6.9f, 0.8f, 0.3f, 0.8f, 1, 0.4f, 0, false, 0, L); 
            addBox(-2.0f, 0.3f, z-6.7f, 0.4f, 0.4f, 0.4f, 1, 0.8f, 0, false, 0, L); 
            addBox(-3, 0, z-8, 3.5f, 1.8f, 0.4f, 0.05f, 0.05f, 0.05f, true, 0, L); 
            
            rooms[i].pW[4] = 2.6f; rooms[i].pZ[4] = z - 8.0f; rooms[i].pSide[4] = 1; 
            addBox(2, 0.8f, z-8.2f, 1.0f, 0.2f, 0.4f, 0.05f, 0.05f, 0.05f, false, 0, L); 
            
            rooms[i].pW[5] = 0.8f; rooms[i].pZ[5] = z - 9.0f; rooms[i].pSide[5] = 0; 
            addBox(0.4f, 0, z-9.4f, 0.8f, 0.3f, 0.8f, 1, 0.4f, 0, false, 0, L); 
            addBox(0.6f, 0.3f, z-9.2f, 0.4f, 0.4f, 0.4f, 1, 0.8f, 0, false, 0, L); 
        } 
        else if (rooms[i].isSeekHallway) { 
            addBox(-2.95f, 0.4f, z-8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L); 
            addBox(2.85f, 0.4f, z-8.5f, 0.1f, 1.0f, 7.0f, 0.4f, 0.7f, 1.0f, false, 0, L); 
        } 
        
        // Lambda for generating side rooms
        auto drawSide = [&](bool isL) {
            float dZ = z + (isL ? rooms[i].leftDoorOffset : rooms[i].rightDoorOffset);
            float bL = fabsf(dZ - z);
            float aL = fabsf((z - 10.0f) - (dZ - 1.2f));
            float m = (isL ? -1.0f : 1.0f);
            float bX = isL ? -3.0f : 2.9f;
            bool dO = isL ? rooms[i].leftDoorOpen : rooms[i].rightDoorOpen;
            float bbX = isL ? -2.90f : 2.86f; 
            
            if (bL > 0.05f) { 
                addTiledSurface(bX, 0.4f, z, 0.1f, 1.4f, -bL, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, true); 
                addTiledSurface(isL?-3.02f:2.92f, 0.0f, z, 0.12f, 0.4f, -bL, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, false); 
                addBox(bbX, 0.0f, z, 0.04f, 0.12f, -bL, 0.12f, 0.06f, 0.03f, false, 0, L); 
            }
            if (aL > 0.05f) { 
                addTiledSurface(bX, 0.4f, dZ-1.2f, 0.1f, 1.4f, -aL, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, true); 
                addTiledSurface(isL?-3.02f:2.92f, 0.0f, dZ-1.2f, 0.12f, 0.4f, -aL, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, false); 
                addBox(bbX, 0.0f, dZ-1.2f, 0.04f, 0.12f, -aL, 0.12f, 0.06f, 0.03f, false, 0, L); 
            }
            
            addTiledSurface(bX, 1.4f, dZ, 0.1f, 0.4f, -1.2f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, false);
            addBox(bX, 0.0f, dZ, 0.1f, 1.4f, -0.05f, 0.15f, 0.08f, 0.04f, false, 0, L); 
            addBox(bX, 0.0f, dZ - 1.15f, 0.1f, 1.4f, -0.05f, 0.15f, 0.08f, 0.04f, false, 0, L); 
            addBox(bX, 1.35f, dZ - 0.05f, 0.1f, 0.05f, -1.1f, 0.15f, 0.08f, 0.04f, false, 0, L);
            
            if (dO) { 
                addBox(isL?-4.1f:3.0f, 0, dZ-1.15f, 1.1f, 1.4f, 0.1f, 0.15f, 0.08f, 0.05f, true, 0, L); 
                addBox(isL?-4.05f:3.95f, 0.7f, dZ-1.05f, 0.05f, 0.15f, 0.05f, 0.6f, 0.6f, 0.6f, false, 0, L); 
                collisions.push_back({isL?-4.1f:3.0f, 0.0f, dZ-1.15f, isL?-3.0f:4.1f, 1.8f, dZ-1.05f, 4}); 
            } else { 
                addBox(isL?-3.0f:2.9f, 0, dZ-0.05f, 0.1f, 1.4f, -1.1f, 0.15f, 0.08f, 0.05f, true, 0, L); 
                float handleX = isL ? -2.95f : 2.85f; 
                addBox(handleX, 0.7f, dZ-0.15f, 0.1f, 0.15f, 0.05f, 0.6f, 0.6f, 0.6f, false, 0, L); 
            }
            
            float srZ = dZ + 2.5f, sW = isL ? -9.0f : 3.0f; 
            addTiledSurface(sW, 0, srZ, 6.0f, 0.01f, -5.0f, floorU,floorV,floorUW,floorVH, floorScale, cR,cG,cB, L, false); 
            addTiledSurface(sW, 1.8f, srZ, 6.0f, 0.01f, -5.0f, floorU,floorV,floorUW,floorVH, floorScale, cR,cG,cB, L, false); 
            addTiledSurface(isL?-9.0f:8.9f, 0.4f, srZ, 0.1f, 1.4f, -5.0f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, true); 
            addTiledSurface(isL?-9.02f:8.88f, 0.0f, srZ, 0.12f, 0.4f, -5.0f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, false);
            
            float bbFarX = isL ? -8.88f : 8.84f; 
            addBox(bbFarX, 0.0f, srZ, 0.04f, 0.12f, -5.0f, 0.12f, 0.06f, 0.03f, false, 0, L);
            
            addTiledSurface(sW, 0.4f, srZ, 6.0f, 1.4f, 0.1f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, true); 
            addTiledSurface(sW, 0.0f, srZ, 6.0f, 0.4f, 0.12f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, false);
            addBox(sW, 0.0f, srZ, 6.0f, 0.12f, -0.04f, 0.12f, 0.06f, 0.03f, false, 0, L);
            
            addTiledSurface(sW, 0.4f, srZ-5.0f, 6.0f, 1.4f, -0.1f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, true); 
            addTiledSurface(sW, 0.0f, srZ-5.0f, 6.0f, 0.4f, -0.12f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, false);
            addBox(sW, 0.0f, srZ-5.0f, 6.0f, 0.12f, 0.04f, 0.12f, 0.06f, 0.03f, false, 0, L);
            
            // Side room furnishings
            if (i == pRm && dO) {
                srand(i * (isL ? 123 : 321)); 
                if (rand() % 2 == 0) { 
                    float pY = 0.6f + (rand() % 50) / 100.0f;
                    float pZ = srZ - 1.5f - (rand() % 20) / 10.0f;
                    float pW = 0.5f + (rand() % 40) / 100.0f;
                    float pH = 0.5f + (rand() % 40) / 100.0f; 
                    addBox(isL?-8.95f:8.89f, pY-0.05f, pZ+0.05f, 0.06f, pH+0.1f, -pW-0.1f, 0.15f, 0.1f, 0.05f, false, 0, L); 
                    addBox(isL?-8.9f:8.83f, pY, pZ, 0.07f, pH, -pW, 0.8f, 0.8f, 0.8f, false, 0, L); 
                } 
                srand(time(NULL));
                
                for(int s = 0; s < 3; s++) { 
                    float fZ = srZ - 0.9f - (s * 1.6f); 
                    
                    int tL = isL ? rooms[i].leftRoomSlotTypeL[s] : rooms[i].rightRoomSlotTypeL[s];
                    int iL = isL ? rooms[i].leftRoomSlotItemL[s] : rooms[i].rightRoomSlotItemL[s]; 
                    float oL = isL ? rooms[i].animLL[s] : rooms[i].animRL[s]; 
                    
                    if (tL == 1) buildCabinet(fZ, true, L, isL ? -6.0f : 6.0f, iL); 
                    else if (tL == 3) buildBed(fZ, true, iL, L, isL ? -6.0f : 6.0f); 
                    else if (tL == 5) buildDresser(fZ, true, oL, iL, L, isL ? -6.0f : 6.0f, true); 
                    else if (tL == 7 || tL == 8) buildChest(isL ? -8.4f : 3.6f, fZ, oL, L); 
                    
                    int tR = isL ? rooms[i].leftRoomSlotTypeR[s] : rooms[i].rightRoomSlotTypeR[s];
                    int iR = isL ? rooms[i].leftRoomSlotItemR[s] : rooms[i].rightRoomSlotItemR[s]; 
                    float oR = isL ? rooms[i].animLR[s] : rooms[i].animRR[s]; 
                    
                    if (tR == 2) buildCabinet(fZ, false, L, isL ? -6.0f : 6.0f, iR); 
                    else if (tR == 4) buildBed(fZ, false, iR, L, isL ? -6.0f : 6.0f); 
                    else if (tR == 6) buildDresser(fZ, false, oR, iR, L, isL ? -6.0f : 6.0f, true); 
                    else if (tR == 7 || tR == 8) buildChest(isL ? -3.6f : 8.4f, fZ, oR, L); 
                }
            }
        };
        
        // Draw the Room Shell
        if (isInteriorVisible) {
            addTiledSurface(-3, 0, z, 6, 0.01f, -10, floorU,floorV,floorUW,floorVH, floorScale, cR,cG,cB, L, false); 
            addTiledSurface(-3, 1.8f, z, 6, 0.01f, -10, floorU,floorV,floorUW,floorVH, floorScale, cR,cG,cB, L, false); 
            
            if (rooms[i].hasLeftRoom) {
                drawSide(true); 
            } else { 
                addTiledSurface(-3.0f, 0.4f, z, 0.1f, 1.4f, -10.0f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, true); 
                addTiledSurface(-3.02f, 0.0f, z, 0.12f, 0.4f, -10.0f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, false); 
                addBox(-2.90f, 0.0f, z, 0.04f, 0.12f, -10.0f, 0.12f, 0.06f, 0.03f, false, 0, L); 
            }
            
            if (rooms[i].hasRightRoom) {
                drawSide(false); 
            } else { 
                addTiledSurface(2.9f, 0.4f, z, 0.1f, 1.4f, -10.0f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, true); 
                addTiledSurface(2.88f, 0.0f, z, 0.12f, 0.4f, -10.0f, wallU,wallV,wallUW,wallVH, wallScale, cR,cG,cB, L, false); 
                addBox(2.86f, 0.0f, z, 0.04f, 0.12f, -10.0f, 0.12f, 0.06f, 0.03f, false, 0, L); 
            } 
            
            addBox(-0.4f, 1.78f, z-5.4f, 0.8f, 0.02f, 0.8f, (L > 0.5f ? 0.9f : 0.2f), (L > 0.5f ? 0.9f : 0.2f), (L > 0.5f ? 0.8f : 0.2f), false);
            
            // Build Furniture and Paintings
            if (!tAN) {
                for (int s = 0; s < 3; s++) { 
                    float zC = z - 2.5f - (s * 2.5f); 
                    int t = rooms[i].slotType[s]; 
                    
                    if (t == 1) buildCabinet(zC, true, L); 
                    else if (t == 2) buildCabinet(zC, false, L); 
                    else if (t == 5) buildDresser(zC, true, rooms[i].animMain[s], rooms[i].slotItem[s], L); 
                    else if (t == 6) buildDresser(zC, false, rooms[i].animMain[s], rooms[i].slotItem[s], L); 
                }
                for (int p = 0; p < rooms[i].pCount; p++) { 
                    float pZ = rooms[i].pZ[p], pH = rooms[i].pH[p], pW = rooms[i].pW[p], pY = rooms[i].pY[p]; 
                    float wX = rooms[i].pSide[p] == 0 ? -2.95f : 2.89f;
                    float cX = rooms[i].pSide[p] == 0 ? -2.88f : 2.82f; 
                    float pR = rooms[i].pR[p], pG = rooms[i].pG[p], pB = rooms[i].pB[p]; 
                    
                    addBox(wX, pY-0.05f, z-pZ+0.05f, 0.06f, pH+0.1f, -pW-0.1f, 0.15f, 0.1f, 0.05f, false, 0, L); 
                    addBox(cX, pY, z-pZ, 0.07f, pH, -pW, pR, pG, pB, false, 0, L); 
                }
            }
        }
        globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f; 
    } 
    globalTintR=1.0f; globalTintG=1.0f; globalTintB=1.0f; 
}

void generateRooms() {
    seekStartRoom = 30 + (rand() % 9);
    
    for (int i = 0; i < TOTAL_ROOMS; i++) {
        rooms[i].doorPos = rand() % 3; 
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
            rooms[i].animLL[s] = 0.0f; 
            rooms[i].animLR[s] = 0.0f; 
            rooms[i].animRL[s] = 0.0f; 
            rooms[i].animRR[s] = 0.0f; 
        }
        
        // Setup Seek Events
        if (i >= seekStartRoom - 5 && i < seekStartRoom) {
            rooms[i].hasSeekEyes = true;
            rooms[i].seekEyeCount = ((i - (seekStartRoom - 5) + 1) * 2) + 3;
        } 
        if (i >= seekStartRoom && i <= seekStartRoom + 2) {
            rooms[i].isSeekHallway = true;
            rooms[i].doorPos = 1;
        } 
        if (i >= seekStartRoom + 3 && i <= seekStartRoom + 7) {
            rooms[i].isSeekChase = true;
            rooms[i].doorPos = 1;
        } 
        if (i == seekStartRoom + 8) {
            rooms[i].isSeekFinale = true;
            rooms[i].doorPos = 1;
        }
        
        // Setup Dupe Rooms
        rooms[i].isDupeRoom = (!iSCE && i > 1 && (rand() % 100 < 15)); 
        if (rooms[i].isDupeRoom) {
            rooms[i].correctDupePos = rand() % 3; 
            int dN = getDisplayRoom(i);
            int f1 = dN + (rand() % 3 + 1);
            int f2 = dN - (rand() % 3 + 1); 
            if (f2 <= 0) f2 = dN + 4; 
            
            rooms[i].dupeNumbers[0] = f1; 
            rooms[i].dupeNumbers[1] = f2; 
            rooms[i].dupeNumbers[2] = dN + 5; 
            rooms[i].dupeNumbers[rooms[i].correctDupePos] = dN;
        }
        
        // --- THE FIX: Eyes Entity Back in the Middle! ---
        rooms[i].hasEyes = (!iSE && i > 2 && !rooms[i].isDupeRoom && rand() % 100 < 8); 
        if (rooms[i].hasEyes) {
            rooms[i].eyesX = 0.0f;
            rooms[i].eyesY = 0.9f;
            rooms[i].eyesZ = -10.0f - (i * 10.0f) - 5.0f;
        }
        
        // Regular Room Setup (Furniture & Side Rooms)
        if (!rooms[i].isSeekChase && !rooms[i].isSeekHallway && !rooms[i].isSeekFinale && !rooms[i].isDupeRoom) {
            rooms[i].hasLeftRoom = (rand() % 100 < 60); 
            rooms[i].hasRightRoom = (rand() % 100 < 60);
            
            // Left Side Room Layouts
            if (rooms[i].hasLeftRoom) { 
                rooms[i].leftDoorOffset = -3.0f - (rand() % 40) / 10.0f; 
                bool cL = false; 
                for (int s = 0; s < 3; s++) { 
                    rooms[i].leftRoomDrawerOpenL[s] = false; 
                    if (rand() % 100 < 45) { 
                        int r = rand() % 100; 
                        rooms[i].leftRoomSlotTypeL[s] = (r < 25) ? 1 : ((r < 50) ? 3 : ((r < 75) ? 5 : 7)); 
                        if (rooms[i].leftRoomSlotTypeL[s] == 7) {
                            if (cL) rooms[i].leftRoomSlotTypeL[s] = 5; else cL = true;
                        } 
                        rooms[i].leftRoomSlotItemL[s] = (rooms[i].leftRoomSlotTypeL[s] >= 3 && rooms[i].leftRoomSlotTypeL[s] <= 6 && rand() % 100 < 30) ? 3 : 0; 
                    } else {
                        rooms[i].leftRoomSlotTypeL[s] = 0;
                        rooms[i].leftRoomSlotItemL[s] = 0;
                    } 
                    
                    rooms[i].leftRoomDrawerOpenR[s] = false; 
                    if (s == 1) {
                        rooms[i].leftRoomSlotTypeR[s] = 99;
                        rooms[i].leftRoomSlotItemR[s] = 0;
                    } else { 
                        if (rand() % 100 < 45) { 
                            int r = rand() % 100; 
                            rooms[i].leftRoomSlotTypeR[s] = (r < 33) ? 2 : ((r < 66) ? 6 : 8); 
                            if (rooms[i].leftRoomSlotTypeR[s] == 8) {
                                if(cL) rooms[i].leftRoomSlotTypeR[s] = 6; else cL = true;
                            } 
                            rooms[i].leftRoomSlotItemR[s] = (rooms[i].leftRoomSlotTypeR[s] >= 3 && rooms[i].leftRoomSlotTypeR[s] <= 6 && rand() % 100 < 30) ? 3 : 0; 
                        } else {
                            rooms[i].leftRoomSlotTypeR[s] = 0;
                            rooms[i].leftRoomSlotItemR[s] = 0;
                        } 
                    } 
                } 
            }
            
            // Right Side Room Layouts
            if (rooms[i].hasRightRoom) { 
                rooms[i].rightDoorOffset = -3.0f - (rand() % 40) / 10.0f; 
                bool cR = false; 
                for (int s = 0; s < 3; s++) { 
                    rooms[i].rightRoomDrawerOpenL[s] = false; 
                    if (s == 1) {
                        rooms[i].rightRoomSlotTypeL[s] = 99;
                        rooms[i].rightRoomSlotItemL[s] = 0;
                    } else { 
                        if (rand() % 100 < 45) { 
                            int r = rand() % 100; 
                            rooms[i].rightRoomSlotTypeL[s] = (r < 33) ? 1 : ((r < 66) ? 5 : 7); 
                            if (rooms[i].rightRoomSlotTypeL[s] == 7) {
                                if (cR) rooms[i].rightRoomSlotTypeL[s] = 5; else cR = true;
                            } 
                            rooms[i].rightRoomSlotItemL[s] = (rooms[i].rightRoomSlotTypeL[s] >= 3 && rooms[i].rightRoomSlotTypeL[s] <= 6 && rand() % 100 < 30) ? 3 : 0; 
                        } else {
                            rooms[i].rightRoomSlotTypeL[s] = 0;
                            rooms[i].rightRoomSlotItemL[s] = 0;
                        } 
                    } 
                    
                    rooms[i].rightRoomDrawerOpenR[s] = false; 
                    if (rand() % 100 < 45) { 
                        int r = rand() % 100; 
                        rooms[i].rightRoomSlotTypeR[s] = (r < 25) ? 2 : ((r < 50) ? 4 : ((r < 75) ? 6 : 8)); 
                        if (rooms[i].rightRoomSlotTypeR[s] == 8) {
                            if (cR) rooms[i].rightRoomSlotTypeR[s] = 6; else cR = true;
                        } 
                        rooms[i].rightRoomSlotItemR[s] = (rooms[i].rightRoomSlotTypeR[s] >= 3 && rooms[i].rightRoomSlotTypeR[s] <= 6 && rand() % 100 < 30) ? 3 : 0; 
                    } else {
                        rooms[i].rightRoomSlotTypeR[s] = 0;
                        rooms[i].rightRoomSlotItemR[s] = 0;
                    } 
                } 
            }
            
            // Main Room Layouts
            bool bS = false; 
            for (int s = 0; s < 3; s++) { 
                float sZR = -2.5f - (s * 2.5f); 
                int r = rand() % 100; 
                
                if (r < 25) rooms[i].slotType[s] = 1; 
                else if (r < 50) rooms[i].slotType[s] = 2; 
                else if (r < 75) {
                    rooms[i].slotType[s] = 5;
                    if (!bS && rand() % 100 < 15) { rooms[i].slotItem[s] = 2; bS = true; }
                    else if (rand() % 100 < 30) rooms[i].slotItem[s] = 3;
                } 
                else {
                    rooms[i].slotType[s] = 6;
                    if (!bS && rand() % 100 < 15) { rooms[i].slotItem[s] = 2; bS = true; }
                    else if (rand() % 100 < 30) rooms[i].slotItem[s] = 3;
                } 
                
                // Keep furniture away from side doors
                if (rooms[i].hasLeftRoom && fabsf(rooms[i].leftDoorOffset - sZR) < 2.6f && (rooms[i].slotType[s] == 1 || rooms[i].slotType[s] == 3 || rooms[i].slotType[s] == 5)) {
                    rooms[i].slotType[s] = 0;
                    rooms[i].slotItem[s] = 0;
                } 
                if (rooms[i].hasRightRoom && fabsf(rooms[i].rightDoorOffset - sZR) < 2.6f && (rooms[i].slotType[s] == 2 || rooms[i].slotType[s] == 4 || rooms[i].slotType[s] == 6)) {
                    rooms[i].slotType[s] = 0;
                    rooms[i].slotItem[s] = 0;
                } 
            }
        } else {
            rooms[i].slotType[0] = 0;
            rooms[i].slotType[1] = 0;
            rooms[i].slotType[2] = 0;
            rooms[i].hasLeftRoom = false;
            rooms[i].hasRightRoom = false;
        }
        
        // Generate Paintings/Posters
        if (rooms[i].isSeekHallway || rooms[i].isSeekFinale || rooms[i].isSeekChase) {
            rooms[i].pCount = 0;
        } else if (!iSE || rooms[i].hasSeekEyes) { 
            rooms[i].pCount = rand() % 5 + 3; 
            for(int p = 0; p < rooms[i].pCount; p++) { 
                bool oL; 
                int tr = 0; 
                do { 
                    oL = false; 
                    rooms[i].pSide[p] = rand() % 2; 
                    rooms[i].pZ[p] = 1.0f + (rand() % 70) / 10.0f; 
                    rooms[i].pY[p] = 0.5f + (rand() % 70) / 100.0f; 
                    rooms[i].pW[p] = 0.3f + (rand() % 60) / 100.0f; 
                    rooms[i].pH[p] = 0.3f + (rand() % 60) / 100.0f; 
                    
                    if (rooms[i].pSide[p] == 0 && rooms[i].hasLeftRoom && fabsf(rooms[i].pZ[p] - fabsf(rooms[i].leftDoorOffset)) < 2.8f) oL = true; 
                    if (rooms[i].pSide[p] == 1 && rooms[i].hasRightRoom && fabsf(rooms[i].pZ[p] - fabsf(rooms[i].rightDoorOffset)) < 2.8f) oL = true; 
                    
                    for(int op = 0; op < p; op++) {
                        if(rooms[i].pSide[p] == rooms[i].pSide[op]) {
                            if(fabsf(rooms[i].pZ[p] - rooms[i].pZ[op]) < (rooms[i].pW[p] + rooms[i].pW[op]) * 0.6f && 
                               fabsf(rooms[i].pY[p] - rooms[i].pY[op]) < (rooms[i].pH[p] + rooms[i].pH[op]) * 0.6f) {
                                oL = true;
                                break;
                            }
                        }
                    } 
                    tr++; 
                } while (oL && tr < 10); 
                
                if (oL) {
                    rooms[i].pCount--;
                    p--;
                } else {
                    rooms[i].pR[p] = 0.15f + (rand() % 35) / 100.0f;
                    rooms[i].pG[p] = 0.15f + (rand() % 35) / 100.0f;
                    rooms[i].pB[p] = 0.15f + (rand() % 35) / 100.0f;
                } 
            } 
        } else {
            rooms[i].pCount = 0; 
        }
    }
    
    // Hardcoded special rooms
    rooms[0].doorPos = 1; 
    rooms[0].isDupeRoom = false; 
    rooms[0].isLocked = true; 
    rooms[0].isJammed = false; 
    rooms[0].hasEyes = false; 
    rooms[48].doorPos = 1; 
    rooms[49].doorPos = 1; 
    
    // Assign Locks and Keys
    for (int i = 2; i < TOTAL_ROOMS - 1; i++) { 
        if (!rooms[i].isDupeRoom && !rooms[i-1].isDupeRoom && 
            !(i >= seekStartRoom && i <= seekStartRoom + 9) && 
            !((i-1) >= seekStartRoom && (i-1) <= seekStartRoom + 9) && 
            (rand() % 3 == 0)) { 
            
            rooms[i].isLocked = true; 
            struct SlotLoc { int type; int index; }; 
            std::vector<SlotLoc> vS; 
            
            for(int s = 0; s < 3; s++) if (rooms[i-1].slotType[s] > 2) vS.push_back({0, s}); 
            
            if (rooms[i-1].hasLeftRoom) {
                for(int s = 0; s < 3; s++) {
                    if (rooms[i-1].leftRoomSlotTypeL[s] >= 3 && rooms[i-1].leftRoomSlotTypeL[s] <= 6) vS.push_back({1, s}); 
                    if (rooms[i-1].leftRoomSlotTypeR[s] >= 3 && rooms[i-1].leftRoomSlotTypeR[s] <= 6) vS.push_back({2, s});
                }
            } 
            if (rooms[i-1].hasRightRoom) {
                for(int s = 0; s < 3; s++) {
                    if (rooms[i-1].rightRoomSlotTypeL[s] >= 3 && rooms[i-1].rightRoomSlotTypeL[s] <= 6) vS.push_back({3, s}); 
                    if (rooms[i-1].rightRoomSlotTypeR[s] >= 3 && rooms[i-1].rightRoomSlotTypeR[s] <= 6) vS.push_back({4, s});
                }
            } 
            
            if (!vS.empty()) { 
                SlotLoc c = vS[rand() % vS.size()]; 
                if(c.type == 0) rooms[i-1].slotItem[c.index] = 1; 
                else if(c.type == 1) rooms[i-1].leftRoomSlotItemL[c.index] = 1; 
                else if(c.type == 2) rooms[i-1].leftRoomSlotItemR[c.index] = 1; 
                else if(c.type == 3) rooms[i-1].rightRoomSlotItemL[c.index] = 1; 
                else if(c.type == 4) rooms[i-1].rightRoomSlotItemR[c.index] = 1; 
            } else {
                rooms[i-1].slotType[1] = 5;
                rooms[i-1].slotItem[1] = 1;
            } 
        } 
    }
}
