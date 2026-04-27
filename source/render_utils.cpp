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
    
    GSPGPU_FlushDataCache(texData, size);
    
    Tex3DS_Texture t3x = Tex3DS_TextureImport(texData, size, tex, NULL, false);
    
    if (!t3x) { 
        sprintf(texErrorMessage, "Tex3DS Import failed"); 
        linearFree(texData); 
        return false; 
    }
    
    C3D_TexSetFilter(tex, GPU_NEAREST, GPU_NEAREST); 
    C3D_TexSetWrap(tex, GPU_REPEAT, GPU_REPEAT);

    Tex3DS_TextureFree(t3x); 
    linearFree(texData); 
    
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
    
    // Invert the V axis so the 3DS doesn't read the empty bottom of the atlas!
    float u1 = u; 
    float u2 = u + uw;
    float v1 = 1.0f - v; 
    float v2 = 1.0f - (v + vh);
    
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

    float u1 = u;
    float u2 = u + uw;
    float v1 = 1.0f - v; 
    float v2 = 1.0f - (v + vh);
    
    addFaceTextured({{bl_x, bl_y, bl_z, 1}, {u1, v2}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u1, v1}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{tr_x, tr_y, tr_z, 1}, {u2, v1}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u1, v1}, {r_c, g_c, b_c, 1}});
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
    
    float u1 = u;
    float u2 = u + uw;
    float v1 = 1.0f - v; 
    float v2 = 1.0f - (v + vh);
    
    addFaceTextured({{bl_x, bl_y, bl_z, 1}, {u1, v2}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u1, v1}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{tr_x, tr_y, tr_z, 1}, {u2, v1}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u1, v1}, {r_c, g_c, b_c, 1}});
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

// === DYNAMIC SUBDIVISION LOOP ===
void addTiledSurface(float x, float y, float z, float w, float h, float d, float u, float v, float uw, float vh, float texScale, float r, float g, float b, float light, bool collide) {
    if(collide && !isBuildingEntities) {
        collisions.push_back({fminf(x,x+w), fminf(y,y+h), fminf(z,z+d), fmaxf(x,x+w), fmaxf(y,y+h), fmaxf(z,z+d), 0});
    }
    
    float r_c = r * light * globalTintR, g_c = g * light * globalTintG, b_c = b * light * globalTintB;

    float signW = (w >= 0) ? 1.0f : -1.0f;
    float signH = (h >= 0) ? 1.0f : -1.0f;
    float signD = (d >= 0) ? 1.0f : -1.0f;

    float absW = fabsf(w);
    float absH = fabsf(h);
    float absD = fabsf(d);

    int stepsX = (absW > 0) ? ceilf(absW / texScale) : 1;
    int stepsY = (absH > 0) ? ceilf(absH / texScale) : 1;
    int stepsZ = (absD > 0) ? ceilf(absD / texScale) : 1;

    bool isFloor = (absH <= 0.05f);

    if (isFloor) {
        for (int ix = 0; ix < stepsX; ix++) {
            float sW = ix * texScale;
            float eW = fminf((ix + 1) * texScale, absW);
            float nextU = u + uw * ((eW - sW) / texScale); 
            
            float curX = x + sW * signW;
            float nextX = x + eW * signW;

            for (int iz = 0; iz < stepsZ; iz++) {
                float sD = iz * texScale;
                float eD = fminf((iz + 1) * texScale, absD);
                
                float curZ = z + sD * signD;
                float nextZ = z + eD * signD;

                // Invert the V calculations
                float pv1 = 1.0f - v;
                float pv2 = 1.0f - (v + vh * ((eD - sD) / texScale));
                
                float yTop = y + h;

                addFaceTextured({{curX, yTop, curZ, 1}, {u, pv1}, {r_c, g_c, b_c, 1}},
                                {{nextX, yTop, curZ, 1}, {nextU, pv1}, {r_c, g_c, b_c, 1}},
                                {{curX, yTop, nextZ, 1}, {u, pv2}, {r_c, g_c, b_c, 1}},
                                {{nextX, yTop, curZ, 1}, {nextU, pv1}, {r_c, g_c, b_c, 1}},
                                {{nextX, yTop, nextZ, 1}, {nextU, pv2}, {r_c, g_c, b_c, 1}},
                                {{curX, yTop, nextZ, 1}, {u, pv2}, {r_c, g_c, b_c, 1}});
            }
        }
    } else {
        if (w >= d) {
            for (int ix = 0; ix < stepsX; ix++) {
                float sW = ix * texScale;
                float eW = fminf((ix + 1) * texScale, absW);
                float curX = x + sW * signW;
                float nextX = x + eW * signW;
                float nextU = u + uw * ((eW - sW) / texScale);
                
                for (int iy = 0; iy < stepsY; iy++) {
                    float sH = iy * texScale;
                    float eH = fminf((iy + 1) * texScale, absH);
                    float curY = y + sH * signH;
                    float nextY = y + eH * signH;
                    
                    // Invert the V calculations
                    float pv1 = 1.0f - v;
                    float pv2 = 1.0f - (v + vh * ((eH - sH) / texScale));

                    float zBack = z;
                    float zFront = z + d;

                    addFaceTextured({{curX, curY, zFront, 1}, {u, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{nextX, curY, zFront, 1}, {nextU, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{curX, nextY, zFront, 1}, {u, pv1}, {r_c, g_c, b_c, 1}}, 
                                    {{nextX, curY, zFront, 1}, {nextU, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{nextX, nextY, zFront, 1}, {nextU, pv1}, {r_c, g_c, b_c, 1}}, 
                                    {{curX, nextY, zFront, 1}, {u, pv1}, {r_c, g_c, b_c, 1}});
                    
                    addFaceTextured({{curX, curY, zBack, 1}, {nextU, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{nextX, curY, zBack, 1}, {u, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{curX, nextY, zBack, 1}, {nextU, pv1}, {r_c, g_c, b_c, 1}}, 
                                    {{nextX, curY, zBack, 1}, {u, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{nextX, nextY, zBack, 1}, {u, pv1}, {r_c, g_c, b_c, 1}}, 
                                    {{curX, nextY, zBack, 1}, {nextU, pv1}, {r_c, g_c, b_c, 1}});
                }
            }
        } else {
            for (int iz = 0; iz < stepsZ; iz++) {
                float sD = iz * texScale;
                float eD = fminf((iz + 1) * texScale, absD);
                float curZ = z + sD * signD;
                float nextZ = z + eD * signD;
                float nextU = u + uw * ((eD - sD) / texScale);

                for (int iy = 0; iy < stepsY; iy++) {
                    float sH = iy * texScale;
                    float eH = fminf((iy + 1) * texScale, absH);
                    float curY = y + sH * signH;
                    float nextY = y + eH * signH;
                    
                    // Invert the V calculations
                    float pv1 = 1.0f - v;
                    float pv2 = 1.0f - (v + vh * ((eH - sH) / texScale));

                    float xLeft = x;
                    float xRight = x + w;

                    addFaceTextured({{xRight, curY, curZ, 1}, {nextU, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{xRight, curY, nextZ, 1}, {u, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{xRight, nextY, curZ, 1}, {nextU, pv1}, {r_c, g_c, b_c, 1}}, 
                                    {{xRight, curY, nextZ, 1}, {u, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{xRight, nextY, nextZ, 1}, {u, pv1}, {r_c, g_c, b_c, 1}}, 
                                    {{xRight, nextY, curZ, 1}, {nextU, pv1}, {r_c, g_c, b_c, 1}});
                    
                    addFaceTextured({{xLeft, curY, curZ, 1}, {u, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{xLeft, curY, nextZ, 1}, {nextU, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{xLeft, nextY, curZ, 1}, {u, pv1}, {r_c, g_c, b_c, 1}}, 
                                    {{xLeft, curY, nextZ, 1}, {nextU, pv2}, {r_c, g_c, b_c, 1}}, 
                                    {{xLeft, nextY, nextZ, 1}, {nextU, pv1}, {r_c, g_c, b_c, 1}}, 
                                    {{xLeft, nextY, curZ, 1}, {u, pv1}, {r_c, g_c, b_c, 1}});
                }
            }
        }
    }
}
