// === WORLD-SPACE TILING LOOP ===
// Guarantees perfect continuous textures across doorways without squishing or gaps!
void addTiledSurface(float x, float y, float z, float w, float h, float d, float u, float v, float uw, float vh, float texScale, float r, float g, float b, float light, bool collide) {
    normalizeUVs(u, v, uw, vh);

    if(collide && !isBuildingEntities) {
        collisions.push_back({fminf(x,x+w), fminf(y,y+h), fminf(z,z+d), fmaxf(x,x+w), fmaxf(y,y+h), fmaxf(z,z+d), 0});
    }
    
    float r_c = r * light * globalTintR, g_c = g * light * globalTintG, b_c = b * light * globalTintB;

    float minX = fminf(x, x + w), maxX = fmaxf(x, x + w);
    float minY = fminf(y, y + h), maxY = fmaxf(y, y + h);
    float minZ = fminf(z, z + d), maxZ = fmaxf(z, z + d);

    float width = maxX - minX;
    float height = maxY - minY;
    float depth = maxZ - minZ;

    // This mathematical magic perfectly aligns the texture to the absolute world grid!
    auto getP = [](float val, float s) { 
        float p = val - floorf(val / s) * s; 
        if (p < 0) p += s; 
        if (s - p < 0.001f) p = 0.0f; 
        return p; 
    };

    auto drawQuad = [&](vertex BL, vertex BR, vertex TL, vertex TR) {
        addFaceTextured(BL, BR, TL, BR, TR, TL);
    };

    bool isFloor = (height <= 0.05f);

    if (isFloor) {
        float scaleU = texScale * 1.5f; 
        float scaleV = texScale * 1.5f; 
        
        float currX = minX;
        while(currX < maxX - 0.0001f) {
            float pX = getP(currX, scaleU);
            float segW = fminf(scaleU - pX, maxX - currX);
            if (segW < 0.001f) segW = 0.001f;
            
            float u1 = u + (pX / scaleU) * uw;
            float u2 = u1 + (segW / scaleU) * uw;
            
            float currZ = minZ;
            while(currZ < maxZ - 0.0001f) {
                float pZ = getP(currZ, scaleV);
                float segD = fminf(scaleV - pZ, maxZ - currZ);
                if (segD < 0.001f) segD = 0.001f;
                
                float pv1 = 1.0f - (v + (pZ / scaleV) * vh);              
                float pv2 = 1.0f - (v + ((pZ + segD) / scaleV) * vh);     

                float x1 = currX, x2 = currX + segW;
                float z1 = currZ, z2 = currZ + segD;
                
                // THE GAP FIX: Draw at minY so the ceiling perfectly touches the top of the walls!
                float yLevel = minY;

                drawQuad({{x1, yLevel, z2, 1}, {u1, pv2}, {r_c, g_c, b_c, 1}},
                         {{x2, yLevel, z2, 1}, {u2, pv2}, {r_c, g_c, b_c, 1}},
                         {{x1, yLevel, z1, 1}, {u1, pv1}, {r_c, g_c, b_c, 1}},
                         {{x2, yLevel, z1, 1}, {u2, pv1}, {r_c, g_c, b_c, 1}});

                currZ += segD;
            } 
            currX += segW;
        }
    } else if (width >= depth) { 
        // Front and Back Walls (Tile along X and Y)
        float scaleU = texScale;
        float scaleV = texScale; 
        
        float currX = minX;
        while(currX < maxX - 0.0001f) {
            float pX = getP(currX, scaleU);
            float segW = fminf(scaleU - pX, maxX - currX);
            if (segW < 0.001f) segW = 0.001f;
            
            float u1 = u + (pX / scaleU) * uw;
            float u2 = u1 + (segW / scaleU) * uw;
            
            float currY = minY;
            while(currY < maxY - 0.0001f) {
                float pY = getP(currY, scaleV);
                float segH = fminf(scaleV - pY, maxY - currY);
                if (segH < 0.001f) segH = 0.001f;
                
                float pv1 = 1.0f - (v + (pY / scaleV) * vh);
                float pv2 = 1.0f - (v + ((pY + segH) / scaleV) * vh);

                float x1 = currX, x2 = currX + segW;
                float y1 = currY, y2 = currY + segH;

                // Front Face (+Z)
                drawQuad({{x1, y1, maxZ, 1}, {u1, pv2}, {r_c, g_c, b_c, 1}},
                         {{x2, y1, maxZ, 1}, {u2, pv2}, {r_c, g_c, b_c, 1}},
                         {{x1, y2, maxZ, 1}, {u1, pv1}, {r_c, g_c, b_c, 1}},
                         {{x2, y2, maxZ, 1}, {u2, pv1}, {r_c, g_c, b_c, 1}});

                // Back Face (-Z)
                drawQuad({{x2, y1, minZ, 1}, {u1, pv2}, {r_c, g_c, b_c, 1}},
                         {{x1, y1, minZ, 1}, {u2, pv2}, {r_c, g_c, b_c, 1}},
                         {{x2, y2, minZ, 1}, {u1, pv1}, {r_c, g_c, b_c, 1}},
                         {{x1, y2, minZ, 1}, {u2, pv1}, {r_c, g_c, b_c, 1}});

                currY += segH;
            } 
            currX += segW;
        }
    } else { 
        // Left and Right Walls (Tile along Z and Y)
        float scaleU = texScale;
        float scaleV = texScale; 
        
        float currZ = minZ;
        while(currZ < maxZ - 0.0001f) {
            float pZ = getP(currZ, scaleU);
            float segD = fminf(scaleU - pZ, maxZ - currZ);
            if (segD < 0.001f) segD = 0.001f;
            
            float u1 = u + (pZ / scaleU) * uw;
            float u2 = u1 + (segD / scaleU) * uw;
            
            float currY = minY;
            while(currY < maxY - 0.0001f) {
                float pY = getP(currY, scaleV);
                float segH = fminf(scaleV - pY, maxY - currY);
                if (segH < 0.001f) segH = 0.001f;
                
                float pv1 = 1.0f - (v + (pY / scaleV) * vh);
                float pv2 = 1.0f - (v + ((pY + segH) / scaleV) * vh);

                float z1 = currZ, z2 = currZ + segD;
                float y1 = currY, y2 = currY + segH;

                // Right Face (+X)
                drawQuad({{maxX, y1, z2, 1}, {u1, pv2}, {r_c, g_c, b_c, 1}},
                         {{maxX, y1, z1, 1}, {u2, pv2}, {r_c, g_c, b_c, 1}},
                         {{maxX, y2, z2, 1}, {u1, pv1}, {r_c, g_c, b_c, 1}},
                         {{maxX, y2, z1, 1}, {u2, pv1}, {r_c, g_c, b_c, 1}});

                // Left Face (-X)
                drawQuad({{minX, y1, z1, 1}, {u1, pv2}, {r_c, g_c, b_c, 1}},
                         {{minX, y1, z2, 1}, {u2, pv2}, {r_c, g_c, b_c, 1}},
                         {{minX, y2, z1, 1}, {u1, pv1}, {r_c, g_c, b_c, 1}},
                         {{minX, y2, z2, 1}, {u2, pv1}, {r_c, g_c, b_c, 1}});

                currY += segH;
            } 
            currZ += segD;
        }
    }
}
