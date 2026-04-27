#include "render_utils.h"
#include <tex3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

inline void normalizeUVs(float& u, float& v, float& uw, float& vh) {
    const float ATLAS_WIDTH = 1024.0f;
    const float ATLAS_HEIGHT = 1024.0f;
    
    if (uw > 1.0f || vh > 1.0f) {
        u /= ATLAS_WIDTH;
        v /= ATLAS_HEIGHT;
        uw /= ATLAS_WIDTH;
        vh /= ATLAS_HEIGHT;
    }
}

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
    normalizeUVs(u, v, uw, vh);
    float r_c = r * light * globalTintR, g_c = g * light * globalTintG, b_c = b * light * globalTintB;

    float minX = fminf(x, x + w), maxX = fmaxf(x, x + w);
    float minY = fminf(y, y + h), maxY = fmaxf(y, y + h);
    float minZ = fminf(z, z + d), maxZ = fmaxf(z, z + d);
    
    float u1 = u, u2 = u + uw;
    float v1 = 1.0f - v, v2 = 1.0f - (v + vh);

    auto drawQuad = [&](vertex BL, vertex BR, vertex TL, vertex TR) {
        addFaceTextured(BL, BR, TL, BR, TR, TL);
    };

    drawQuad({{minX, maxY, maxZ, 1}, {u1, v2}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, maxZ, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{minX, maxY, minZ, 1}, {u1, v1}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, minZ, 1}, {u2, v1}, {r_c, g_c, b_c, 1}});
    drawQuad({{maxX, minY, maxZ, 1}, {u1, v2}, {r_c, g_c, b_c, 1}}, {{minX, minY, maxZ, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{maxX, minY, minZ, 1}, {u1, v1}, {r_c, g_c, b_c, 1}}, {{minX, minY, minZ, 1}, {u2, v1}, {r_c, g_c, b_c, 1}});
    drawQuad({{maxX, minY, maxZ, 1}, {u1, v2}, {r_c, g_c, b_c, 1}}, {{minX, minY, maxZ, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, maxZ, 1}, {u1, v1}, {r_c, g_c, b_c, 1}}, {{minX, maxY, maxZ, 1}, {u2, v1}, {r_c, g_c, b_c, 1}});
    drawQuad({{minX, minY, minZ, 1}, {u1, v2}, {r_c, g_c, b_c, 1}}, {{maxX, minY, minZ, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{minX, maxY, minZ, 1}, {u1, v1}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, minZ, 1}, {u2, v1}, {r_c, g_c, b_c, 1}});
    drawQuad({{maxX, minY, minZ, 1}, {u1, v2}, {r_c, g_c, b_c, 1}}, {{maxX, minY, maxZ, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, minZ, 1}, {u1, v1}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, maxZ, 1}, {u2, v1}, {r_c, g_c, b_c, 1}});
    drawQuad({{minX, minY, maxZ, 1}, {u1, v2}, {r_c, g_c, b_c, 1}}, {{minX, minY, minZ, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{minX, maxY, maxZ, 1}, {u1, v1}, {r_c, g_c, b_c, 1}}, {{minX, maxY, minZ, 1}, {u2, v1}, {r_c, g_c, b_c, 1}});
}

void addBillboard(float cx, float cy, float cz, float w, float h, float u, float v, float uw, float vh, float light) {
    normalizeUVs(u, v, uw, vh);
    float r_c = light * globalTintR, g_c = light * globalTintG, b_c = light * globalTintB;
    float hw = w / 2.0f, hh = h / 2.0f, cosY = cosf(camYaw), sinY = sinf(camYaw);
    float rx = hw * cosY, rz = -hw * sinY;
    
    float bl_x = cx - rx, bl_y = cy - hh, bl_z = cz - rz;
    float br_x = cx + rx, br_y = cy - hh, br_z = cz + rz;
    float tl_x = cx - rx, tl_y = cy + hh, tl_z = cz - rz;
    float tr_x = cx + rx, tr_y = cy + hh, tr_z = cz + rz;

    float u1 = u, u2 = u + uw;
    float v1 = 1.0f - v, v2 = 1.0f - (v + vh);
    
    addFaceTextured({{bl_x, bl_y, bl_z, 1}, {u1, v2}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u1, v1}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{tr_x, tr_y, tr_z, 1}, {u2, v1}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u1, v1}, {r_c, g_c, b_c, 1}});
}

void addBillboardSpherical(float cx, float cy, float cz, float w, float h, float u, float v, float uw, float vh, float light) {
    normalizeUVs(u, v, uw, vh);
    float r_c = light * globalTintR, g_c = light * globalTintG, b_c = light * globalTintB;
    float hw = w / 2.0f, hh = h / 2.0f;
    float cosY = cosf(camYaw), sinY = sinf(camYaw), cosP = cosf(camPitch), sinP = sinf(camPitch);
    float rx = hw * cosY, rz = -hw * sinY, ux = hh * sinY * sinP, uy = hh * cosP, uz = hh * cosY * sinP;
    
    float bl_x = cx - rx - ux, bl_y = cy - uy, bl_z = cz - rz - uz;
    float br_x = cx + rx - ux, br_y = cy - uy, br_z = cz + rz - uz;
    float tl_x = cx - rx + ux, tl_y = cy + uy, tl_z = cz - rz + uz;
    float tr_x = cx + rx + ux, tr_y = cy + uy, tr_z = cz + rz + uz;
    
    float u1 = u, u2 = u + uw;
    float v1 = 1.0f - v, v2 = 1.0f - (v + vh);
    
    addFaceTextured({{bl_x, bl_y, bl_z, 1}, {u1, v2}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u1, v1}, {r_c, g_c, b_c, 1}}, {{br_x, br_y, br_z, 1}, {u2, v2}, {r_c, g_c, b_c, 1}}, {{tr_x, tr_y, tr_z, 1}, {u2, v1}, {r_c, g_c, b_c, 1}}, {{tl_x, tl_y, tl_z, 1}, {u1, v1}, {r_c, g_c, b_c, 1}});
}

void addBoxColored(float x, float y, float z, float w, float h, float d, float r, float g, float b, float light) {
    float r_c=r*light*globalTintR, g_c=g*light*globalTintG, b_c=b*light*globalTintB;
    if(r_c>1.0f) r_c=1.0f; if(g_c>1.0f) g_c=1.0f; if(b_c>1.0f) b_c=1.0f;
    
    float minX = fminf(x, x + w), maxX = fmaxf(x, x + w);
    float minY = fminf(y, y + h), maxY = fmaxf(y, y + h);
    float minZ = fminf(z, z + d), maxZ = fmaxf(z, z + d);

    auto drawQuad = [&](vertex BL, vertex BR, vertex TL, vertex TR) {
        addFaceColored(BL, BR, TL, BR, TR, TL);
    };

    drawQuad({{minX, maxY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{minX, maxY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}});
    drawQuad({{maxX, minY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{minX, minY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{maxX, minY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{minX, minY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}});
    drawQuad({{maxX, minY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{minX, minY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{minX, maxY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}});
    drawQuad({{minX, minY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{maxX, minY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{minX, maxY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}});
    drawQuad({{maxX, minY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{maxX, minY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{maxX, maxY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}});
    drawQuad({{minX, minY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{minX, minY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{minX, maxY, maxZ, 1}, {0,0}, {r_c, g_c, b_c, 1}}, {{minX, maxY, minZ, 1}, {0,0}, {r_c, g_c, b_c, 1}});
}

void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide, int colType, float light) {
    addBoxColored(x, y, z, w, h, d, r, g, b, light);
    if(collide && !isBuildingEntities) {
        collisions.push_back({fminf(x,x+w), fminf(y,y+h), fminf(z,z+d), fmaxf(x,x+w), fmaxf(y,y+h), fmaxf(z,z+d), colType});
    }
}

// === FLAWLESS TILING LOOP (Absolute Measurement Edition) ===
void addTiledSurface(float x, float y, float z, float w, float h, float d, float u, float v, float uw, float vh, float texScale, float r, float g, float b, float light, bool collide) {
    normalizeUVs(u, v, uw, vh);

    if(collide && !isBuildingEntities) {
        collisions.push_back({fminf(x,x+w), fminf(y,y+h), fminf(z,z+d), fmaxf(x,x+w), fmaxf(y,y+h), fmaxf(z,z+d), 0});
    }
    
    float r_c = r * light * globalTintR, g_c = g * light * globalTintG, b_c = b * light * globalTintB;

    float minX = fminf(x, x + w), maxX = fmaxf(x, x + w);
    float minY = fminf(y, y + h), maxY = fmaxf(y, y + h);
    float minZ = fminf(z, z + d), maxZ = fmaxf(z, z + d);

    float absW = maxX - minX;
    float absH = maxY - minY;
    float absD = maxZ - minZ;

    bool isFloor = (absH <= 0.05f);

    auto drawQuad = [&](vertex BL, vertex BR, vertex TL, vertex TR) {
        addFaceTextured(BL, BR, TL, BR, TR, TL);
    };

    if (isFloor) {
        float yTop = maxY;
        for (float curW = 0.0f; curW < absW - 0.001f; curW += texScale) {
            float nextW = curW + texScale;
            if (nextW > absW) nextW = absW;
            float fracW = (nextW - curW) / texScale;
            float curU2 = u + uw * fracW;

            for (float curD = 0.0f; curD < absD - 0.001f; curD += texScale) {
                float nextD = curD + texScale;
                if (nextD > absD) nextD = absD;
                float fracD = (nextD - curD) / texScale;
                
                float pv1 = 1.0f - v; 
                float pv2 = 1.0f - (v + vh * fracD);

                float X1 = minX + curW;
                float X2 = minX + nextW;
                float Z1 = minZ + curD;
                float Z2 = minZ + nextD;

                vertex BL = {{X1, yTop, Z2, 1}, {u, pv2}, {r_c, g_c, b_c, 1}};
                vertex BR = {{X2, yTop, Z2, 1}, {curU2, pv2}, {r_c, g_c, b_c, 1}};
                vertex TL = {{X1, yTop, Z1, 1}, {u, pv1}, {r_c, g_c, b_c, 1}};
                vertex TR = {{X2, yTop, Z1, 1}, {curU2, pv1}, {r_c, g_c, b_c, 1}};
                drawQuad(BL, BR, TL, TR);
            }
        }
    } else {
        // Build Walls Bottom to Top
        for (float curH = 0.0f; curH < absH - 0.001f; curH += texScale) {
            float nextH = curH + texScale;
            // The ultimate fix: This GUARANTEES the final wall tile perfectly touches the ceiling!
            if (nextH > absH) nextH = absH; 
            
            float fracH = (nextH - curH) / texScale;
            float pv2 = 1.0f - (v + vh); // Floor anchor
            float pv1 = 1.0f - (v + vh - vh * fracH); // Ceiling slice

            float Y1 = minY + curH;
            float Y2 = minY + nextH;

            if (absW >= absD) { 
                // Front Wall (+Z) - Tiles Left to Right
                for (float curW = 0.0f; curW < absW - 0.001f; curW += texScale) {
                    float nextW = curW + texScale;
                    if (nextW > absW) nextW = absW;
                    float fracW = (nextW - curW) / texScale;
                    float curU2 = u + uw * fracW;

                    float X1 = minX + curW;
                    float X2 = minX + nextW;
                    
                    vertex F_BL = {{X1, Y1, maxZ, 1}, {u, pv2}, {r_c, g_c, b_c, 1}};
                    vertex F_BR = {{X2, Y1, maxZ, 1}, {curU2, pv2}, {r_c, g_c, b_c, 1}};
                    vertex F_TL = {{X1, Y2, maxZ, 1}, {u, pv1}, {r_c, g_c, b_c, 1}};
                    vertex F_TR = {{X2, Y2, maxZ, 1}, {curU2, pv1}, {r_c, g_c, b_c, 1}};
                    drawQuad(F_BL, F_BR, F_TL, F_TR);
                }

                // Back Wall (-Z) - Tiles Right to Left (so it matches the Front visual mapping!)
                for (float curW = 0.0f; curW < absW - 0.001f; curW += texScale) {
                    float nextW = curW + texScale;
                    if (nextW > absW) nextW = absW;
                    float fracW = (nextW - curW) / texScale;
                    float curU2 = u + uw * fracW;

                    float X1 = maxX - curW; 
                    float X2 = maxX - nextW;
                    
                    vertex B_BL = {{X1, Y1, minZ, 1}, {u, pv2}, {r_c, g_c, b_c, 1}};
                    vertex B_BR = {{X2, Y1, minZ, 1}, {curU2, pv2}, {r_c, g_c, b_c, 1}};
                    vertex B_TL = {{X1, Y2, minZ, 1}, {u, pv1}, {r_c, g_c, b_c, 1}};
                    vertex B_TR = {{X2, Y2, minZ, 1}, {curU2, pv1}, {r_c, g_c, b_c, 1}};
                    drawQuad(B_BL, B_BR, B_TL, B_TR);
                }
            } else { 
                // Right Wall (+X)
                for (float curW = 0.0f; curW < absD - 0.001f; curW += texScale) {
                    float nextW = curW + texScale;
                    if (nextW > absD) nextW = absD;
                    float fracW = (nextW - curW) / texScale;
                    float curU2 = u + uw * fracW;

                    float Z1 = maxZ - curW;
                    float Z2 = maxZ - nextW;
                    
                    vertex R_BL = {{maxX, Y1, Z1, 1}, {u, pv2}, {r_c, g_c, b_c, 1}};
                    vertex R_BR = {{maxX, Y1, Z2, 1}, {curU2, pv2}, {r_c, g_c, b_c, 1}};
                    vertex R_TL = {{maxX, Y2, Z1, 1}, {u, pv1}, {r_c, g_c, b_c, 1}};
                    vertex R_TR = {{maxX, Y2, Z2, 1}, {curU2, pv1}, {r_c, g_c, b_c, 1}};
                    drawQuad(R_BL, R_BR, R_TL, R_TR);
                }

                // Left Wall (-X)
                for (float curW = 0.0f; curW < absD - 0.001f; curW += texScale) {
                    float nextW = curW + texScale;
                    if (nextW > absD) nextW = absD;
                    float fracW = (nextW - curW) / texScale;
                    float curU2 = u + uw * fracW;

                    float Z1 = minZ + curW;
                    float Z2 = minZ + nextW;
                    
                    vertex L_BL = {{minX, Y1, Z1, 1}, {u, pv2}, {r_c, g_c, b_c, 1}};
                    vertex L_BR = {{minX, Y1, Z2, 1}, {curU2, pv2}, {r_c, g_c, b_c, 1}};
                    vertex L_TL = {{minX, Y2, Z1, 1}, {u, pv1}, {r_c, g_c, b_c, 1}};
                    vertex L_TR = {{minX, Y2, Z2, 1}, {curU2, pv1}, {r_c, g_c, b_c, 1}};
                    drawQuad(L_BL, L_BR, L_TL, L_TR);
                }
            }
        }
    }
}
