#include "render_utils.h"
#include <tex3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

ndspWaveBuf loadWav(const char* path) {
    ndspWaveBuf w = {0}; 
    FILE* f = fopen(path, "rb"); 
    if(!f) return w; 
    
    fseek(f, 12, SEEK_SET);
    char id[4]; 
    u32 sz; 
    bool found = false;
    
    while(fread(id, 1, 4, f) == 4) { 
        fread(&sz, 4, 1, f); 
        if(strncmp(id, "data", 4) == 0) {
            found = true;
            break;
        } 
        fseek(f, sz, SEEK_CUR); 
    }
    
    if(!found) { 
        fclose(f); 
        return w; 
    } 
    
    s16* buf = (s16*)linearAlloc(sz); 
    if(!buf) {
        fclose(f); 
        return w;
    }
    
    fread(buf, 1, sz, f); 
    fclose(f); 
    DSP_FlushDataCache(buf, sz);
    
    w.data_vaddr = buf; 
    w.nsamples = sz / 2; 
    w.looping = false; 
    w.status = NDSP_WBUF_FREE; 
    return w;
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
        sprintf(texErrorMessage, "linearAlloc failed"); 
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
    std::vector<vertex>& target = isBuildingEntities ? entity_mesh_textured : world_mesh_textured;
    if (target.size() + 6 >= (isBuildingEntities ? MAX_ENTITY_VERTS : MAX_VERTS)) return;
    target.push_back(v1); target.push_back(v2); target.push_back(v3);
    target.push_back(v4); target.push_back(v5); target.push_back(v6);
}

void addFaceColored(vertex v1, vertex v2, vertex v3, vertex v4, vertex v5, vertex v6) {
    std::vector<vertex>& target = isBuildingEntities ? entity_mesh_colored : world_mesh_colored;
    if (target.size() + 6 >= (isBuildingEntities ? MAX_ENTITY_VERTS : MAX_VERTS)) return;
    target.push_back(v1); target.push_back(v2); target.push_back(v3);
    target.push_back(v4); target.push_back(v5); target.push_back(v6);
}

void addBoxTextured(float x, float y, float z, float w, float h, float d, float u, float v, float uw, float vh, float repW, float repH, float r, float g, float b, float light) {
    float r_c = r * light * globalTintR, g_c = g * light * globalTintG, b_c = b * light * globalTintB;
    float x2=x+w, y2=y+h, z2=z+d; 
    float u1 = u, v1 = v; float u2 = u + (uw * repW), v2 = v + (vh * repH);
    
    addFaceTextured({{x,y,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,y2,z2,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{u1,v1},{r_c,g_c,b_c,1}}); 
    addFaceTextured({{x,y,z,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}); 
    addFaceTextured({{x,y,z,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x,y2,z,1},{u1,v1},{r_c,g_c,b_c,1}}); 
    addFaceTextured({{x2,y,z,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y2,z2,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}); 
    addFaceTextured({{x,y2,z,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y2,z,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x2,y2,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x,y2,z2,1},{u1,v2},{r_c,g_c,b_c,1}}); 
    addFaceTextured({{x,y,z,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,y,z,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,y,z2,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x,y,z2,1},{u1,v1},{r_c,g_c,b_c,1}}); 
}

void addBillboard(float cx, float cy, float cz, float w, float h, float u, float v, float uw, float vh, float light) {
    float r_c = light * globalTintR, g_c = light * globalTintG, b_c = light * globalTintB;
    float hw = w / 2.0f, hh = h / 2.0f, cosY = cosf(camYaw), sinY = sinf(camYaw);
    float rx = hw * cosY, rz = -hw * sinY;
    
    float bl_x = cx - rx, bl_y = cy - hh, bl_z = cz - rz;
    float br_x = cx + rx, br_y = cy - hh, br_z = cz + rz;
    float tl_x = cx - rx, tl_y = cy + hh, tl_z = cz - rz;
    float tr_x = cx + rx, tr_y = cy + hh, tr_z = cz + rz;
    
    addFaceTextured({{bl_x, bl_y, bl_z, 1}, {u, v}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u+uw, v}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u, v+vh}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u+uw, v}, {r_c, g_c, b_c, 1}}, {{tr_x, tr_y, tr_z, 1}, {u+uw, v+vh}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u, v+vh}, {r_c, g_c, b_c, 1}});
}

void addBillboardSpherical(float cx, float cy, float cz, float w, float h, float u, float v, float uw, float vh, float light) {
    float r_c = light * globalTintR, g_c = light * globalTintG, b_c = light * globalTintB;
    float hw = w / 2.0f, hh = h / 2.0f;
    float cosY = cosf(camYaw), sinY = sinf(camYaw), cosP = cosf(camPitch), sinP = sinf(camPitch);
    float rx = hw * cosY, rz = -hw * sinY, ux = hh * sinY * sinP, uy = hh * cosP, uz = hh * cosY * sinP;
    
    float bl_x = cx - rx - ux, bl_y = cy - uy, bl_z = cz - rz - uz;
    float br_x = cx + rx - ux, br_y = cy - uy, br_z = cz + rz - uz;
    float tl_x = cx - rx + ux, tl_y = cy + uy, tl_z = cz - rz + uz;
    float tr_x = cx + rx + ux, tr_y = cy + uy, tr_z = cz + rz + uz;
    
    addFaceTextured({{bl_x, bl_y, bl_z, 1}, {u, v}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u+uw, v}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u, v+vh}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u+uw, v}, {r_c, g_c, b_c, 1}}, {{tr_x, tr_y, tr_z, 1}, {u+uw, v+vh}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u, v+vh}, {r_c, g_c, b_c, 1}});
}

void addBoxColored(float x, float y, float z, float w, float h, float d, float r, float g, float b, float light) {
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

void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide, int colType, float light) {
    addBoxColored(x, y, z, w, h, d, r, g, b, light);
    if(collide && !isBuildingEntities) {
        collisions.push_back({fminf(x,x+w), fminf(y,y+h), fminf(z,z+d), fmaxf(x,x+w), fmaxf(y,y+h), fmaxf(z,z+d), colType});
    }
}

void addTiledSurface(float x, float y, float z, float w, float h, float d, float u, float v, float uw, float vh, float texScale, float r, float g, float b, float light, bool collide) {
    if(collide && !isBuildingEntities) {
        collisions.push_back({fminf(x,x+w), fminf(y,y+h), fminf(z,z+d), fmaxf(x,x+w), fmaxf(y,y+h), fmaxf(z,z+d), 0});
    }
    
    float minX = fminf(x, x+w), maxX = fmaxf(x, x+w);
    float minY = fminf(y, y+h), maxY = fmaxf(y, y+h);
    float minZ = fminf(z, z+d), maxZ = fmaxf(z, z+d);
    float width = maxX - minX, height = maxY - minY, depth = maxZ - minZ;
    float r_c = r * light * globalTintR, g_c = g * light * globalTintG, b_c = b * light * globalTintB;
    
    auto getP = [](float val, float s) { 
        float p = val - (float)((int)(val / s)) * s; 
        if (p < 0) p += s; 
        if (s - p < 0.001f) p = 0.0f; 
        return p; 
    };
    
    bool isFloor = (height <= 0.05f);
    if (isFloor) {
        float scaleU = texScale * 1.5f, scaleV = texScale * 1.5f; 
        float currX = minX;
        while(currX < maxX - 0.0001f) {
            float pX = getP(currX, scaleU), segW = fminf(scaleU - pX, maxX - currX);
            float u1 = u + (pX / scaleU) * uw, repX = segW / scaleU;
            if (segW < 0.001f) segW = 0.001f; 
            
            float currZ = minZ;
            while(currZ < maxZ - 0.0001f) {
                float pZ = getP(currZ, scaleV), segD = fminf(scaleV - pZ, maxZ - currZ);
                float v1 = v + (pZ / scaleV) * vh, repZ = segD / scaleV;
                if (segD < 0.001f) segD = 0.001f; 
                
                float u2 = u1 + (uw * repX), v2 = v1 + (vh * repZ);
                float x2 = currX + segW, z2 = currZ + segD;
                
                addFaceTextured({{currX,maxY,currZ,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,maxY,currZ,1},{u2,v1},{r_c,g_c,b_c,1}}, {{currX,maxY,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,maxY,currZ,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x2,maxY,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{currX,maxY,z2,1},{u1,v2},{r_c,g_c,b_c,1}}); 
                addFaceTextured({{currX,minY,currZ,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,minY,currZ,1},{u2,v2},{r_c,g_c,b_c,1}}, {{currX,minY,z2,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,minY,currZ,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,minY,z2,1},{u2,v1},{r_c,g_c,b_c,1}}, {{currX,minY,z2,1},{u1,v1},{r_c,g_c,b_c,1}}); 
                currZ += segD;
            } 
            currX += segW;
        }
    } else {
        float scaleU = texScale, scaleV = texScale; 
        if (width >= depth) { 
            float currX = minX;
            while(currX < maxX - 0.0001f) {
                float pX = getP(currX, scaleU), segW = fminf(scaleU - pX, maxX - currX);
                float u1 = u + (pX / scaleU) * uw, repX = segW / scaleU;
                if (segW < 0.001f) segW = 0.001f; 
                
                float currY = minY;
                while(currY < maxY - 0.0001f) {
                    float pY = getP(currY, scaleV), segH = fminf(scaleV - pY, maxY - currY);
                    float v1 = v + (pY / scaleV) * vh, repY = segH / scaleV;
                    if (segH < 0.001f) segH = 0.001f;
                    
                    float u2 = u1 + (uw * repX), v2 = v1 + (vh * repY);
                    float x2 = currX + segW, y2 = currY + segH;
                    
                    addFaceTextured({{currX,currY,maxZ,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,currY,maxZ,1},{u2,v2},{r_c,g_c,b_c,1}}, {{currX,y2,maxZ,1},{u1,v1},{r_c,g_c,b_c,1}}, {{x2,currY,maxZ,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,y2,maxZ,1},{u2,v1},{r_c,g_c,b_c,1}}, {{currX,y2,maxZ,1},{u1,v1},{r_c,g_c,b_c,1}}); 
                    addFaceTextured({{currX,currY,minZ,1},{u2,v2},{r_c,g_c,b_c,1}}, {{x2,currY,minZ,1},{u1,v2},{r_c,g_c,b_c,1}}, {{currX,y2,minZ,1},{u2,v1},{r_c,g_c,b_c,1}}, {{x2,currY,minZ,1},{u1,v2},{r_c,g_c,b_c,1}}, {{x2,y2,minZ,1},{u1,v1},{r_c,g_c,b_c,1}}, {{currX,y2,minZ,1},{u2,v1},{r_c,g_c,b_c,1}}); 
                    currY += segH;
                } 
                currX += segW;
            }
        } else { 
            float currZ = minZ;
            while(currZ < maxZ - 0.0001f) {
                float pZ = getP(currZ, scaleU), segD = fminf(scaleU - pZ, maxZ - currZ);
                float u1 = u + (pZ / scaleU) * uw, repZ = segD / scaleU;
                if (segD < 0.001f) segD = 0.001f; 
                
                float currY = minY;
                while(currY < maxY - 0.0001f) {
                    float pY = getP(currY, scaleV), segH = fminf(scaleV - pY, maxY - currY);
                    float v1 = v + (pY / scaleV) * vh, repY = segH / scaleV;
                    if (segH < 0.001f) segH = 0.001f;
                    
                    float u2 = u1 + (uw * repZ), v2 = v1 + (vh * repY);
                    float z2 = currZ + segD, y2 = currY + segH;
                    
                    addFaceTextured({{maxX,currY,currZ,1},{u2,v2},{r_c,g_c,b_c,1}}, {{maxX,currY,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{maxX,y2,currZ,1},{u2,v1},{r_c,g_c,b_c,1}}, {{maxX,currY,z2,1},{u1,v2},{r_c,g_c,b_c,1}}, {{maxX,y2,z2,1},{u1,v1},{r_c,g_c,b_c,1}}, {{maxX,y2,currZ,1},{u2,v1},{r_c,g_c,b_c,1}}); 
                    addFaceTextured({{minX,currY,currZ,1},{u1,v2},{r_c,g_c,b_c,1}}, {{minX,currY,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{minX,y2,currZ,1},{u1,v1},{r_c,g_c,b_c,1}}, {{minX,currY,z2,1},{u2,v2},{r_c,g_c,b_c,1}}, {{minX,y2,z2,1},{u2,v1},{r_c,g_c,b_c,1}}, {{minX,y2,currZ,1},{u1,v1},{r_c,g_c,b_c,1}}); 
                    currY += segH;
                } 
                currZ += segD;
            }
        }
    }
}
