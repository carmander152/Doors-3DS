#include "md2.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>

extern std::vector<vertex> seek_mesh;

bool MD2Model::load(const char* filepath,bool is_animation, const char* file_name) {
    std::string full_path = std::string(filepath) + file_name;
    FILE* file = fopen(full_path.c_str(), "rb");
    if (!file) return false;

    int header[17];
    fread(header, sizeof(int), 17, file);
    
    // Check if it's a valid MD2 file (IDP2, version 8)
    if (header[0] != 844121161 || header[1] != 8) { fclose(file); return false; }
    
    int ofsTex = header[12], ofsTris = header[13];
    ofsFrames = header[14];

    model_name = file_name;

    if (is_animation == false) {
        numTris = header[8];
        int numTexCoords = header[7];
        int skinW = header[2], skinH = header[3];
        // Load UVs
        fseek(file, ofsTex, SEEK_SET);
        for (int i = 0; i < numTexCoords; i++) {
            short st[2];
            fread(st, sizeof(short), 2, file);

            // Push U coordinate normally
            uvs.push_back((float)st[0] / skinW);

            // THE FIX: Invert the V coordinate because MD2 reads from Top-Left, but 3DS reads from Bottom-Left!
            uvs.push_back(1.0f - ((float)st[1] / skinH));
        }

        // Load Triangles
        fseek(file, ofsTris, SEEK_SET);
        for (int i = 0; i < numTris; i++) {
            short v[3], t[3];
            fread(v, sizeof(short), 3, file);
            fread(t, sizeof(short), 3, file);
            for (int j = 0; j < 3; j++) { triVerts.push_back(v[j]); triUVs.push_back(t[j]); }
        }
    }
    
    else {
        // Load Frames
        numVerts = header[6];
        numFrames = header[10];
        frameSize = header[4];
        load_anim();
    }

    fclose(file);
    return true;
}

void MD2Model::load_anim() {
    sprintf(uiMessage, "loading anim");
    messageTimer = 30;
    std::string full_path = std::string("romfs:/Models/Animations/") + model_name;
    FILE* file = fopen(full_path.c_str(), "rb");
    if (!file) {
        sprintf(uiMessage, "could not load anim fuck");
        messageTimer = 30;
        return;
    }
    fseek(file, ofsFrames, SEEK_SET);
    for (int i = current_anim_frame; i < current_anim_frame + 5; i++) {
        fseek(file, ofsFrames + i * frameSize, SEEK_SET);
        float scale[3], trans[3];
        fread(scale, sizeof(float), 3, file);
        fread(trans, sizeof(float), 3, file);
        char name[16];
        fread(name, 1, 16, file);

        std::vector<float> verts;
        for (int v = 0; v < numVerts; v++) {
            unsigned char p[4];
            fread(p, 1, 4, file);

            // Calculate coords and swap Quake's Up-Axis to match Citro3D
            verts.push_back((p[0] * scale[0]) + trans[0]);        // X
            verts.push_back((p[2] * scale[2]) + trans[2]);        // Y (Up)
            verts.push_back(-((p[1] * scale[1]) + trans[1]));     // Z (Depth)
        }
        frameVerts.push_back(verts);
    }

}

void MD2Model::draw(MD2Model animation_model,int frame, float x, float y, float z, float scale, float L, float rotY) {
    if (frame < 0 || frame >= animation_model.numFrames) return;
    if (frame == 0) {
        frame = 1;
        current_anim_frame = 1;
        animation_model.frameVerts.clear();
    }
    float cosR = cosf(rotY);
    float sinR = sinf(rotY);

    if (animation_model.frameVerts.size() == 0) {
        animation_model.load_anim();
    }

    for (int i = 0; i < numTris * 3; i++) {
        animation_model.frameVerts.erase(animation_model.frameVerts.begin(), animation_model.frameVerts.begin());
        int vIdx = triVerts[i] * 3;
        int uvIdx = triUVs[i] * 2;

        float vx = animation_model.frameVerts[0][vIdx] * scale;
        float vy = animation_model.frameVerts[0][vIdx+1] * scale;
        float vz = animation_model.frameVerts[0][vIdx+2] * scale;

        // Apply Y-rotation so he faces the right way
        float rx = vx * cosR - vz * sinR;
        float rz = vx * sinR + vz * cosR;

        vertex vert;
        vert.pos[0] = rx + x;
        vert.pos[1] = vy + y;
        vert.pos[2] = rz + z;
        vert.pos[3] = 1.0f;
        
        vert.texcoord[0] = uvs[uvIdx];
        vert.texcoord[1] = uvs[uvIdx+1];
        
        vert.clr[0] = L; vert.clr[1] = L; vert.clr[2] = L; vert.clr[3] = 1.0f;

        seek_mesh.push_back(vert);
        current_anim_frame += 1;
        
        
    }
}
