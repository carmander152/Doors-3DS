#include "game_state.h"

std::vector<vertex> world_mesh_colored;
std::vector<vertex> world_mesh_textured; 
std::vector<vertex> entity_mesh_colored;
std::vector<vertex> entity_mesh_textured;
std::vector<BBox> collisions;

float globalTintR = 1.0f, globalTintG = 1.0f, globalTintB = 1.0f;
C3D_Tex atlasTex;
bool hasAtlas = false; 
bool isBuildingEntities = false;
char texErrorMessage[100] = "";

RoomSetup rooms[TOTAL_ROOMS];

int playerHealth = 100, flashRedFrames = 0, seekStartRoom = 0, playerCoins = 0, elevatorTimer = 796, messageTimer = 0;
bool hasKey = false, lobbyKeyPickedUp = false, isCrouching = false, isDead = false;
bool inElevator = true, elevatorDoorsOpen = false, elevatorClosing = false, elevatorJamFinished = false;
bool doorOpen[TOTAL_ROOMS] = {false}; 
HideState hideState = NOT_HIDING; 

float camX = 0.0f, camZ = 7.5f, camYaw = 0.0f, camPitch = 0.0f; 
float elevatorDoorOffset = 0.0f; 
char uiMessage[50] = "";

bool screechActive = false, rushActive = false, seekActive = false, inEyesRoom = false, isLookingAtEyes = false;
int screechState = 0, screechTimer = 0, screechCooldown = 0, rushState = 0, rushTimer = 0, rushCooldown = 0;
int seekState = 0, seekTimer = 0, eyesDamageTimer = 0, eyesDamageAccumulator = 0, eyesGraceTimer = 0, eyesSoundCooldown = 0;

float screechX = 0.0f, screechY = 0.0f, screechZ = 0.0f, screechOffsetX = 0.0f, screechOffsetY = 0.0f, screechOffsetZ = 0.0f;
float rushStartTimer = 1.0f, rushZ = 0.0f, rushTargetZ = 0.0f;
float seekZ = 0.0f, seekSpeed = 0.0f, seekMaxSpeed = 0.076f; 

bool figureActive = false, figureState = 0; 
float figureX = 0.0f, figureY = 0.0f, figureZ = 0.0f, figureSpeed = 0.06f; 
int figureWaypoint = 0;

// --- NEW FIGURE VARIABLES ---
int figureTargetWP = 0;
float figureTargetX = 0.0f;
float figureTargetZ = 0.0f;
// ----------------------------

int getDisplayRoom(int idx) { 
    return (idx < 0) ? 0 : idx + 1; 
}

int getNextDoorIndex(int currentIdx) { 
    if (currentIdx >= seekStartRoom && currentIdx <= seekStartRoom + 2) {
        return seekStartRoom + 3;
    }
    return currentIdx + 1; 
}
