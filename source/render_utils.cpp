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
    
    // Map directly to the passed sub-texture coordinates!
    float u1 = u, v1 = v; 
    float u2 = u + uw, v2 = v + vh;
    
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

    float u1 = u, v1 = v;
    float u2 = u + uw, v2 = v + vh;
    
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
    
    float u1 = u, v1 = v;
    float u2 = u + uw, v2 = v + vh;
    
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

// === NEW DYNAMIC SUBDIVISION LOOP ===
// This handles texture atlas repeating safely by slicing long geometry into multiple smaller tiles
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

    int stepsX = ceilf(absW / texScale); if(stepsX == 0) stepsX = 1;
    int stepsY = ceilf(absH / texScale); if(stepsY == 0) stepsY = 1;
    int stepsZ = ceilf(absD / texScale); if(stepsZ == 0) stepsZ = 1;

    bool isFloor = (absH <= 0.05f);

    if (isFloor) {
        // Tile Floor along X and Z
        for(int ix = 0; ix < stepsX; ix++) {
            float startW = ix * texScale;
            float endW = fminf((ix + 1) * texScale, absW);
            float fracW = (endW - startW) / texScale;

            for(int iz = 0; iz < stepsZ; iz++) {
                float startD = iz * texScale;
                float endD = fminf((iz + 1) * texScale, absD);
                float fracD = (endD - startD) / texScale;

                float px1 = x + startW * signW;
                float px2 = x + endW * signW;
                float pz1 = z + startD * signD;
                float pz2 = z + endD * signD;

                float pu1 = u;
                float pu2 = u + uw * fracW;
                float pv1 = v;
                float pv2 = v + vh * fracD;
                float py = y + h;

                addFaceTextured({{px1, py, pz1, 1}, {pu1, pv1}, {r_c, g_c, b_c, 1}},
                                {{px2, py, pz1, 1}, {pu2, pv1}, {r_c, g_c, b_c, 1}},
                                {{px1, py, pz2, 1}, {pu1, pv2}, {r_c, g_c, b_c, 1}},
                                {{px2, py, pz1, 1}, {pu2, pv1}, {r_c, g_c, b_c, 1}},
                                {{px2, py, pz2, 1}, {pu2, pv2}, {r_c, g_c, b_c, 1}},
                                {{px1, py, pz2, 1}, {pu1, pv2}, {r_c, g_c, b_c, 1}});
            }
        }
    } else if (absW >= absD) {
        // Tile Wall along X and Y axes
        for(int ix = 0; ix < stepsX; ix++) {
            float startW = ix * texScale;
            float endW = fminf((ix + 1) * texScale, absW);
            float fracW = (endW - startW) / texScale;

            for(int iy = 0; iy < stepsY; iy++) {
                float startH = iy * texScale;
                float endH = fminf((iy + 1) * texScale, absH);
                float fracH = (endH - startH) / texScale;

                float px1 = x + startW * signW;
                float px2 = x + endW * signW;
                float py1 = y + startH * signH;
                float py2 = y + endH * signH;
                
                float pz1 = z;
                float pz2 = z + d;

                float pu1 = u;
                float pu2 = u + uw * fracW;
                float pv1 = v;
                float pv2 = v + vh * fracH;

                addFaceTextured({{px1, py1, pz2, 1}, {pu1, pv2}, {r_c, g_c, b_c, 1}},
                                {{px2, py1, pz2, 1}, {pu2, pv2}, {r_c, g_c, b_c, 1}},
                                {{px1, py2, pz2, 1}, {pu1, pv1}, {r_c, g_c, b_c, 1}},
                                {{px2, py1, pz2, 1}, {pu2, pv2}, {r_c, g_c, b_c, 1}},
                                {{px2, py2, pz2, 1}, {pu2, pv1}, {r_c, g_c, b_c, 1}},
                                {{px1, py2, pz2, 1}, {pu1, pv1}, {r_c, g_c, b_c, 1}});

                addFaceTextured({{px1, py1, pz1, 1}, {pu2, pv2}, {r_c, g_c, b_c, 1}},
                                {{px2, py1, pz1, 1}, {pu1, pv2}, {r_c, g_c, b_c, 1}},
                                {{px1, py2, pz1, 1}, {pu2, pv1}, {r_c, g_c, b_c, 1}},
                                {{px2, py1, pz1, 1}, {pu1, pv2}, {r_c, g_c, b_c, 1}},
                                {{px2, py2, pz1, 1}, {pu1, pv1}, {r_c, g_c, b_c, 1}},
                                {{px1, py2, pz1, 1}, {pu2, pv1}, {r_c, g_c, b_c, 1}});
            }
        }
    } else {
        // Tile Wall along Z and Y axes
        for(int iz = 0; iz < stepsZ; iz++) {
            float startD = iz * texScale;
            float endD = fminf((iz + 1) * texScale, absD);
            float fracD = (endD - startD) / texScale;

            for(int iy = 0; iy < stepsY; iy++) {
                float startH = iy * texScale;
                float endH = fminf((iy + 1) * texScale, absH);
                float fracH = (endH - startH) / texScale;

                float pz1 = z + startD * signD;
                float pz2 = z + endD * signD;
                float py1 = y + startH * signH;
                float py2 = y + endH * signH;
                
                float px1 = x;
                float px2 = x + w;

                float pu1 = u;
                float pu2 = u + uw * fracD;
                float pv1 = v;
                float pv2 = v + vh * fracH;

                addFaceTextured({{px2, py1, pz1, 1}, {pu2, pv2}, {r_c, g_c, b_c, 1}},
                                {{px2, py1, pz2, 1}, {pu1, pv2}, {r_c, g_c, b_c, 1}},
                                {{px2, py2, pz1, 1}, {pu2, pv1}, {r_c, g_c, b_c, 1}},
                                {{px2, py1, pz2, 1}, {pu1, pv2}, {r_c, g_c, b_c, 1}},
                                {{px2, py2, pz2, 1}, {pu1, pv1}, {r_c, g_c, b_c, 1}},
                                {{px2, py2, pz1, 1}, {pu2, pv1}, {r_c, g_c, b_c, 1}});

                addFaceTextured({{px1, py1, pz1, 1}, {pu1, pv2}, {r_c, g_c, b_c, 1}},
                                {{px1, py1, pz2, 1}, {pu2, pv2}, {r_c, g_c, b_c, 1}},
                                {{px1, py2, pz1, 1}, {pu1, pv1}, {r_c, g_c, b_c, 1}},
                                {{px1, py1, pz2, 1}, {pu2, pv2}, {r_c, g_c, b_c, 1}},
                                {{px1, py2, pz2, 1}, {pu2, pv1}, {r_c, g_c, b_c, 1}},
                                {{px1, py2, pz1, 1}, {pu1, pv1}, {r_c, g_c, b_c, 1}});
            }
        }
    }
}
