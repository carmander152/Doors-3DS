#pragma once
#include "game_state.h"

void buildLamp(float x, float z, float L=1.0f);
void buildCabinet(float zC, bool isL, float L=1.0f, float offX=0.0f, int item=0);
void buildBed(float zC, bool isL, int item, float L=1.0f, float offX=0.0f);
void buildDresser(float zC, bool isL, float openFactor, int item, float L=1.0f, float offX=0.0f, bool hasLamp=false);
void buildChest(float x, float z, float openFactor, float L=1.0f);
void addWallWithDoors(float z, bool lD, bool lO, bool cD, bool cO, bool rD, bool rO, int rm, float L=1.0f);
void buildWorld(int cChunk, int pRm);
void generateRooms();
