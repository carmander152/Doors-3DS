#include "entities.h"
#include "render_utils.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

void buildEntities(int pRm) {
    entity_mesh_colored.clear(); 
    entity_mesh_textured.clear();
    isBuildingEntities = true;
    
    // Screech Entity
    if (screechActive) { 
        float scale = (screechState == 3) ? 2.5f : 1.5f; 
        addBillboard(screechX, screechY + 0.2f, screechZ, scale, scale, 0.6f, 0.0f, 0.4f, 0.33f); 
    }
    
    // Rush Entity
    if (rushActive && rushState == 2) { 
        addBillboard(0.0f, 0.7f, rushZ, 1.4f, 1.4f, 0.6f, 0.66f, 0.4f, 0.34f); 
    }
    
    // Eyes Entity
    for (int i = pRm - 1; i <= pRm + 2; i++) {
        if (i >= 0 && i < TOTAL_ROOMS && rooms[i].hasEyes) {
            bool renderEyes = !(rooms[i].isSeekChase || rooms[i].hasSeekEyes) || (i >= pRm && i <= pRm + 1);
            if (renderEyes) {
                addBillboardSpherical(rooms[i].eyesX, rooms[i].eyesY + 0.3f, rooms[i].eyesZ, 1.4f, 1.4f, 0.6f, 0.33f, 0.4f, 0.33f, rooms[i].lightLevel);
            }
        }
    }

    // Seek Entity
    if (seekActive) {
        float sY = 0.0f, sH = 1.1f; 
        
        if (seekState == 1) { 
            if (seekTimer <= 65) {
                sY = -1.1f + (seekTimer / 65.0f) * 1.1f; 
            } else {
                sY = 0; 
                sH = 1.1f;
            } 
            
            srand(seekTimer); 
            for (int d = 0; d < 8; d++) { 
                float dx = -1.5f + (rand() % 30) / 10.0f;
                float dz = seekZ - 0.5f - (rand() % 30) / 10.0f;
                float dy = 1.8f - fmodf((seekTimer + d * 10) * 0.10f, 1.8f); 
                addBox(dx, dy, dz, 0.08f, 0.2f, 0.08f, 0.05f, 0.05f, 0.05f, false); 
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
    
    // Figure Entity
    if (figureActive) {
        addBox(figureX - 0.2f, figureY, figureZ - 0.1f, 0.15f, 0.9f, 0.2f, 0.3f, 0.05f, 0.05f, false); 
        addBox(figureX + 0.05f, figureY, figureZ - 0.1f, 0.15f, 0.9f, 0.2f, 0.3f, 0.05f, 0.05f, false); 
        addBox(figureX - 0.2f, figureY + 0.9f, figureZ - 0.1f, 0.4f, 0.7f, 0.2f, 0.4f, 0.1f, 0.05f, false); 
        addBox(figureX - 0.35f, figureY + 0.5f, figureZ - 0.1f, 0.15f, 1.1f, 0.2f, 0.3f, 0.05f, 0.05f, false); 
        addBox(figureX + 0.2f, figureY + 0.5f, figureZ - 0.1f, 0.15f, 1.1f, 0.2f, 0.3f, 0.05f, 0.05f, false); 
        addBox(figureX - 0.15f, figureY + 1.6f, figureZ - 0.15f, 0.3f, 0.3f, 0.3f, 0.5f, 0.1f, 0.05f, false); 
        addBox(figureX - 0.1f, figureY + 1.65f, figureZ - 0.16f, 0.2f, 0.2f, 0.02f, 0.8f, 0.8f, 0.5f, false, 0, 1.5f);
    }
    
    isBuildingEntities = false;
}
