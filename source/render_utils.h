#pragma once
#include "game_state.h"

ndspWaveBuf loadWav(const char* path);
bool loadTextureFromFile(const char* path, C3D_Tex* tex);

void addFaceTextured(vertex v1, vertex v2, vertex v3, vertex v4, vertex v5, vertex v6);
void addFaceColored(vertex v1, vertex v2, vertex v3, vertex v4, vertex v5, vertex v6);
void addBoxTextured(float x, float y, float z, float w, float h, float d, float u, float v, float uw, float vh, float repW, float repH, float r, float g, float b, float light = 1.0f);
void addBillboard(float cx, float cy, float cz, float w, float h, float u, float v, float uw, float vh, float light = 1.0f);
void addBillboardSpherical(float cx, float cy, float cz, float w, float h, float u, float v, float uw, float vh, float light = 1.0f);
void addBoxColored(float x, float y, float z, float w, float h, float d, float r, float g, float b, float light = 1.0f);
void addBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, bool collide, int colType = 0, float light = 1.0f);
void addTiledSurface(float x, float y, float z, float w, float h, float d, float u, float v, float uw, float vh, float texScale, float r, float g, float b, float light, bool collide);
