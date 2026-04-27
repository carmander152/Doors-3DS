#include "entities.h"
#include "render_utils.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "atlas_uvs.h" // Added this to import your dynamic texture coordinates!

void buildEntities(int pRm) {
    entity_mesh_colored.clear(); 
    entity_mesh_textured.clear();
    isBuildingEntities = true;
    
    // Screech Entity
    if (screechActive) { 
        float scale = (screechState == 3) ? 2.5f : 1.5f; 
        addBillboard(screechX, screechY + 0.2f, screechZ, scale, scale, TEX_SCREECH.u, TEX_SCREECH.v, TEX_SCREECH.uw, TEX_SCREECH.vh); 
    }
    
    // Rush Entity
    if (rushActive && rushState == 2) { 
        addBillboard(0.0f, 0.7f, rushZ, 1.4f, 1.4f, TEX_RUSH.u, TEX_RUSH.v, TEX_RUSH.uw, TEX_RUSH.vh); 
    }
    
    // Eyes Entity
    for (int i = pRm - 1; i <= pRm + 2; i++) {
        if (i >= 0 && i < TOTAL_ROOMS && rooms[i].hasEyes) {
            
            // --- THE FIX: Eyes Render Cullling ---
            // Replicates world_gen.cpp's line of sight system for entity culling
            bool isVisible = true;
            if (!seekActive) {
                if (i > pRm && !doorOpen[i] && i != seekStartRoom + 1 && i != seekStartRoom + 2) isVisible = false;
                if (i < pRm && i + 1 < TOTAL_ROOMS && !doorOpen[i + 1] && (i + 1) != seekStartRoom + 1 && (i + 1) != seekStartRoom + 2) isVisible = false;
            }
            
            if (isVisible) {
                bool renderEyes = !(rooms[i].isSeekChase || rooms[i].hasSeekEyes) || (i >= pRm && i <= pRm + 1);
                if (renderEyes) {
                    addBillboardSpherical(rooms[i].eyesX, rooms[i].eyesY, rooms[i].eyesZ, 1.2f, 1.2f, TEX_EYES.u, TEX_EYES.v, TEX_EYES.uw, TEX_EYES.vh);
                }
            }
        }
    }
    
    // Seek Entity (Remains Solid 3D Geometry for now)
    if (seekActive) { 
        float sY = 0.2f, sH = 0.8f;
        if (seekState == 1) { 
            sY = -0.3f + (seekTimer / 115.0f) * 0.5f;
            sH = 0.4f + (seekTimer / 115.0f) * 0.7f;
            
            srand(seekTimer); 
            for (int k = 0; k < 12; k++) {
                float pX = -0.5f + (rand() % 100) / 100.0f;
                float pZ = seekZ - 0.2f - (rand() % 60) / 100.0f;
                float pY = sY + (rand() % 40) / 100.0f;
                addBox(pX, pY, pZ, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, false); 
            } 
            srand(time(NULL)); 
        } else if (seekState == 2) {
            sY = 0;
            sH = 1.1f;
        }
        
        // Seek's body parts
        addBox(-0.3f, sY, seekZ - 0.3f, 0.6f, sH, 0.6f, 0.05f, 0.05f, 0.05f, false); 
        addBox(-0.15f, sY + 0.8f, seekZ - 0.35f, 0.3f, 0.2f, 0.06f, 0.9f, 0.9f, 0.9f, false, 0, 1.5f); 
        addBox(-0.05f, sY + 0.8f, seekZ - 0.38f, 0.1f, 0.2f, 0.04f, 0, 0, 0, false, 0, 1.5f); 
        
        if (seekState == 1) {
            addBox(-1.0f, 0.01f, seekZ - 1.0f, 2.0f, 0.01f, 2.0f, 0.02f, 0.02f, 0.02f, false);
        }
    }
    
    // Figure Entity (Remains Solid 3D Geometry for now)
    if (figureActive) {
        float fY = 0.0f, fH = 1.2f;
        addBox(figureX - 0.3f, fY, figureZ - 0.3f, 0.6f, fH, 0.6f, 0.4f, 0.1f, 0.05f, false); 
        
        float headTilt = (figureState == 1) ? 0.2f : 0.0f;
        addBox(figureX - 0.2f, fY + fH, figureZ - 0.2f + headTilt, 0.4f, 0.4f, 0.4f, 0.5f, 0.2f, 0.1f, false);
        
        addBox(figureX - 0.15f, fY + fH + 0.1f, figureZ - 0.25f + headTilt, 0.3f, 0.2f, 0.1f, 0.1f, 0.0f, 0.0f, false);
    }

    isBuildingEntities = false;
}
