#include "physics.h"
#include <math.h>

bool checkCollision(float x, float y, float z, float h) {
    if (isDead) return true; 
    
    float r = 0.2f; 
    for(auto& b : collisions) { 
        if (b.type == 4) continue; // Ignore door interaction volumes
        
        // Broad phase check (Optimization)
        if (x + 2.0f < b.minX || x - 2.0f > b.maxX || z + 2.0f < b.minZ || z - 2.0f > b.maxZ) {
            continue;
        }
        
        // Detailed AABB collision check
        if (x + r > b.minX && x - r < b.maxX && 
            z + r > b.minZ && z - r < b.maxZ && 
            y + h > b.minY && y < b.maxY) {
            return true; 
        }
    } 
    return false;
}

bool checkLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2) {
    float minVX = fminf(x1, x2) - 1.0f; 
    float maxVX = fmaxf(x1, x2) + 1.0f;
    float minVZ = fminf(z1, z2) - 1.0f; 
    float maxVZ = fmaxf(z1, z2) + 1.0f;
    
    for(auto& b : collisions) { 
        if (b.type == 4) continue; 
        
        if (maxVX < b.minX || minVX > b.maxX || maxVZ < b.minZ || minVZ > b.maxZ) {
            continue;
        }
        
        float tmin = 0.0f, tmax = 1.0f;
        float dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
        
        // Check X axis
        if (fabsf(dx) > 0.0001f) { 
            float tx1 = (b.minX - x1) / dx;
            float tx2 = (b.maxX - x1) / dx; 
            tmin = fmaxf(tmin, fminf(tx1, tx2)); 
            tmax = fminf(tmax, fmaxf(tx1, tx2)); 
        } else if (x1 <= b.minX || x1 >= b.maxX) {
            continue;
        }
        
        // Check Y axis
        if (fabsf(dy) > 0.0001f) { 
            float ty1 = (b.minY - y1) / dy;
            float ty2 = (b.maxY - y1) / dy; 
            tmin = fmaxf(tmin, fminf(ty1, ty2)); 
            tmax = fminf(tmax, fmaxf(ty1, ty2)); 
        } else if (y1 <= b.minY || y1 >= b.maxY) {
            continue;
        }
        
        // Check Z axis
        if (fabsf(dz) > 0.0001f) { 
            float tz1 = (b.minZ - z1) / dz;
            float tz2 = (b.maxZ - z1) / dz; 
            tmin = fmaxf(tmin, fminf(tz1, tz2)); 
            tmax = fminf(tmax, fmaxf(tz1, tz2)); 
        } else if (z1 <= b.minZ || z1 >= b.maxZ) {
            continue;
        }
        
        if (tmin <= tmax && tmax > 0.0f && tmin < 1.0f) {
            return false; // Line of sight is blocked
        }
    } 
    return true; 
}
