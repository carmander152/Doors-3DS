#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>
#include <string.h>
#include <stdlib.h> 
#include <stdio.h> 
#include <math.h> 
#include <vector>
#include <time.h>
#include "vshader_shbin.h"

// Your custom modules
#include "game_state.h"
#include "render_utils.h"
#include "physics.h"
#include "world_gen.h"
#include "entities.h"

int main() {
    // --- System Initialization ---
    gfxInitDefault(); 
    gfxSet3D(false); 
    irrstInit(); 
    srand(time(NULL)); 
    consoleInit(GFX_BOTTOM, NULL); 
    romfsInit();
    
    // --- Audio Initialization ---
    bool audio_ok = R_SUCCEEDED(ndspInit()); 
    ndspWaveBuf sPsst={0}, sAttack={0}, sCaught={0}, sDoor={0}, sLockedDoor={0}, sDupeAttack={0}, sRushScream={0}, sEyesAppear={0}, sEyesGarble={0}, sEyesAttack={0}, sEyesHit={0}, sSeekRise={0}, sSeekChase={0}, sSeekEscaped={0}, sDeath={0}, sElevatorJam={0}, sElevatorJamEnd={0};
    ndspWaveBuf sCoinsCollect={0}, sDarkRoomEnter={0}, sDrawerClose={0}, sDrawerOpen={0}, sLightsFlicker={0}, sWardrobeEnter={0}, sWardrobeExit={0};

    if (audio_ok) { 
        ndspSetOutputMode(NDSP_OUTPUT_STEREO); 
        for (int i = 0; i <= 12; i++) {
            ndspChnSetInterp(i, NDSP_INTERP_LINEAR);
            ndspChnSetRate(i, 44100);
            ndspChnSetFormat(i, NDSP_FORMAT_MONO_PCM16);
        } 
        
        sPsst = loadWav("romfs:/Screech_Psst.wav"); 
        sAttack = loadWav("romfs:/Screech_Attack.wav"); 
        sCaught = loadWav("romfs:/Screech_Caught.wav"); 
        sDoor = loadWav("romfs:/Door_Open.wav"); 
        sLockedDoor = loadWav("romfs:/Locked_Door.wav"); 
        sDupeAttack = loadWav("romfs:/Dupe_Attack.wav"); 
        sRushScream = loadWav("romfs:/Rush_Scream.wav"); 
        sEyesAppear = loadWav("romfs:/Eyes_Appear.wav"); 
        sEyesGarble = loadWav("romfs:/Eyes_Garble.wav"); 
        sEyesGarble.looping = true; 
        sEyesAttack = loadWav("romfs:/Eyes_Attack.wav"); 
        sEyesHit = loadWav("romfs:/Eyes_Hit.wav"); 
        sSeekRise = loadWav("romfs:/Seek_Rise.wav"); 
        sSeekChase = loadWav("romfs:/Seek_Chase.wav"); 
        sSeekChase.looping = true; 
        sSeekEscaped = loadWav("romfs:/Seek_Escaped.wav"); 
        sDeath = loadWav("romfs:/Player_Death.wav"); 
        sElevatorJam = loadWav("romfs:/Elevator_Jam.wav"); 
        sElevatorJamEnd = loadWav("romfs:/Elevator_Jam_End.wav"); 
        sCoinsCollect = loadWav("romfs:/Coins_Collect.wav"); 
        sDarkRoomEnter = loadWav("romfs:/Dark_Room_Enter.wav"); 
        sDrawerClose = loadWav("romfs:/Drawer_Close.wav"); 
        sDrawerOpen = loadWav("romfs:/Drawer_Open.wav"); 
        sLightsFlicker = loadWav("romfs:/Lights_Flicker.wav"); 
        sWardrobeEnter = loadWav("romfs:/Wardrobe_Enter.wav"); 
        sWardrobeExit = loadWav("romfs:/Wardrobe_Exit.wav");
    }

    // --- Graphics & GPU Setup ---
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE); 
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8); 
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    
    hasAtlas = loadTextureFromFile("romfs:/atlas.t3x", &atlasTex);
    
    // --- Initial World Generation ---
    generateRooms(); 
    int currentChunk = 0, playerCurrentRoom = -1; 
    
    world_mesh_colored.reserve(MAX_VERTS); 
    world_mesh_textured.reserve(MAX_VERTS); 
    entity_mesh_colored.reserve(MAX_ENTITY_VERTS); 
    entity_mesh_textured.reserve(MAX_ENTITY_VERTS);

    void* vbo_main = linearAlloc((MAX_VERTS + MAX_ENTITY_VERTS) * sizeof(vertex)); 
    if (!vbo_main) { 
        printf("\x1b[31mCRITICAL ERROR: Out of Linear Memory!\x1b[0m\n"); 
        while (aptMainLoop()) { 
            hidScanInput(); 
            if (hidKeysDown() & KEY_START) break; 
            gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank(); 
        } 
        if(audio_ok) ndspExit(); 
        C3D_TexDelete(&atlasTex); 
        romfsExit(); C3D_Fini(); gfxExit(); 
        return 0; 
    }
    
    buildWorld(currentChunk, playerCurrentRoom);
    
    // --- Shader Setup ---
    DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size); 
    shaderProgram_s program; 
    shaderProgramInit(&program); 
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]); 
    C3D_BindProgram(&program);
    
    int uLoc_proj = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx"); 
    C3D_AttrInfo* attr = C3D_GetAttrInfo(); 
    AttrInfo_Init(attr); 
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 4); 
    AttrInfo_AddLoader(attr, 1, GPU_FLOAT, 2); 
    AttrInfo_AddLoader(attr, 2, GPU_FLOAT, 4); 
    
    int colored_size = world_mesh_colored.size(); 
    int textured_size = world_mesh_textured.size();
    if (colored_size + textured_size > MAX_VERTS) textured_size = MAX_VERTS - colored_size; 
    
    memcpy(vbo_main, world_mesh_colored.data(), colored_size * sizeof(vertex)); 
    memcpy((vertex*)vbo_main + colored_size, world_mesh_textured.data(), textured_size * sizeof(vertex)); 
    GSPGPU_FlushDataCache(vbo_main, (colored_size + textured_size) * sizeof(vertex));
    
    // --- Render States ---
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL); 
    C3D_CullFace(GPU_CULL_NONE); 
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    C3D_AlphaTest(true, GPU_GREATER, 128);

    const char symbols[] = "@!$#&*%?"; 
    static float startTouchX = 0, startTouchY = 0; 
    static bool wasTouching = false; 
    static int lastRoomForDarkCheck = -1; 
    u64 lastTime = osGetTime(); 
    int frames = 0; 
    float currentFps = 0.0f;

    static float currentRoll = 0.0f, bobTime = 0.0f, camBobY = 0.0f, camBobX = 0.0f;

    // ==========================================
    //               GAMEPLAY LOGIC
    // ==========================================
    while (aptMainLoop()) {
        u64 currTime = osGetTime(); 
        frames++; 
        if (currTime - lastTime >= 1000) { 
            currentFps = frames / ((currTime - lastTime) / 1000.0f); 
            frames = 0; 
            lastTime = currTime; 
        }
        
        hidScanInput(); 
        irrstScanInput(); 
        u32 kDown = hidKeysDown(), kHeld = hidKeysHeld(); 
        if (kDown & KEY_SELECT) break;
        
        bool needsVBOUpdate = false; 
        static bool deathSoundPlayed = false; 
        static u64 totalFrames = 0; 
        totalFrames++;

        // --- Death Audio Handling ---
        if (isDead) { 
            if (!deathSoundPlayed) { 
                if (audio_ok) { 
                    ndspChnWaveBufClear(3); ndspChnWaveBufClear(5); 
                    ndspChnWaveBufClear(7); ndspChnWaveBufClear(9); 
                    bool isAttackActive = (sAttack.status == NDSP_WBUF_PLAYING || sAttack.status == NDSP_WBUF_QUEUED || 
                                           sDupeAttack.status == NDSP_WBUF_PLAYING || sDupeAttack.status == NDSP_WBUF_QUEUED); 
                    if (!isAttackActive) { 
                        ndspChnWaveBufClear(8); 
                        if (sDeath.data_vaddr) {
                            sDeath.status = NDSP_WBUF_FREE;
                            ndspChnWaveBufAdd(8, &sDeath);
                        } 
                        deathSoundPlayed = true; 
                    } 
                } else {
                    deathSoundPlayed = true; 
                }
            } 
        } else {
            deathSoundPlayed = false;
        }
        
        // --- Restart Game ---
        if (kDown & KEY_START) { 
            if (isDead) { 
                isDead = false; hasKey = false; lobbyKeyPickedUp = false; isCrouching = false; 
                hideState = NOT_HIDING; playerHealth = 100; screechActive = false; screechState = 0; 
                flashRedFrames = 0; playerCoins = 0; screechCooldown = 900; rushActive = false; 
                rushState = 0; rushCooldown = 0; messageTimer = 0; inElevator = true; 
                elevatorTimer = 796; elevatorDoorsOpen = false; elevatorClosing = false; 
                elevatorDoorOffset = 0; elevatorJamFinished = false; 
                
                camX = 0; camZ = 7.5f; camYaw = 0; camPitch = 0; 
                currentChunk = 0; playerCurrentRoom = -1; lastRoomForDarkCheck = -1; 
                
                for (int i = 0; i < TOTAL_ROOMS; i++) doorOpen[i] = false; 
                
                seekActive = false; seekState = 0; seekTimer = 0; eyesSoundCooldown = 0; 
                
                generateRooms(); 
                buildWorld(currentChunk, playerCurrentRoom); 
                
                colored_size = world_mesh_colored.size(); 
                textured_size = world_mesh_textured.size(); 
                if (colored_size + textured_size > MAX_VERTS) textured_size = MAX_VERTS - colored_size; 
                
                memcpy(vbo_main, world_mesh_colored.data(), colored_size * sizeof(vertex)); 
                memcpy((vertex*)vbo_main + colored_size, world_mesh_textured.data(), textured_size * sizeof(vertex)); 
                GSPGPU_FlushDataCache(vbo_main, (colored_size + textured_size) * sizeof(vertex)); 
                
                if (audio_ok) {
                    for(int i = 3; i <= 12; i++) ndspChnWaveBufClear(i);
                }
                
                inEyesRoom = false; isLookingAtEyes = false; eyesDamageTimer = 0; 
                eyesDamageAccumulator = 0; eyesGraceTimer = 0; consoleClear(); 
                continue; 
            } 
        }
        
        // --- Debug Cheats ---
        if (!isDead && (kHeld & KEY_R) && (kHeld & KEY_Y)) { 
            if (kDown & KEY_DDOWN) { 
                camZ = -10.0f - ((seekStartRoom - 1) * 10.0f) + 5.0f; 
                camX = 0.0f; camYaw = 0.0f; camPitch = 0.0f; 
                needsVBOUpdate = true; 
                sprintf(uiMessage, "Teleported to Seek!"); 
                messageTimer = 45; 
            } 
            if (kDown & KEY_DLEFT) { 
                camZ = -10.0f - (48 * 10.0f) + 5.0f; 
                camX = 0.0f; camYaw = 0.0f; camPitch = 0.0f; 
                needsVBOUpdate = true; 
                sprintf(uiMessage, "Teleported near Library!"); 
                messageTimer = 45; 
            } 
        }
        
        // --- Room Management ---
        playerCurrentRoom = (camZ >= -10.0f) ? -1 : (int)((-camZ - 10.0f) / 10.0f); 
        if (playerCurrentRoom < -1) playerCurrentRoom = -1; 
        if (playerCurrentRoom > TOTAL_ROOMS - 2) playerCurrentRoom = TOTAL_ROOMS - 2;
        
        bool isGlitch = false; 
        int tDR = -1; 
        for (int k = 1; k < TOTAL_ROOMS; k++) {
            if (rooms[k].isDupeRoom && playerCurrentRoom == (k - 1)) {
                isGlitch = true; 
                tDR = k; 
                break;
            }
        }
        
        // --- Timers ---
        if (messageTimer > 0) messageTimer--; 
        if (flashRedFrames > 0 && !isDead) flashRedFrames--; 
        if (screechCooldown > 0) screechCooldown--; 
        if (rushCooldown > 0) rushCooldown--; 
        if (eyesSoundCooldown > 0) eyesSoundCooldown--;

        // --- Dark Room Logic ---
        if (playerCurrentRoom != lastRoomForDarkCheck && playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) { 
            lastRoomForDarkCheck = playerCurrentRoom; 
            if (rooms[playerCurrentRoom].lightLevel < 0.5f && audio_ok && sDarkRoomEnter.data_vaddr) { 
                float m[12] = {0}; 
                m[0] = 0.3f; m[1] = 0.3f; 
                ndspChnSetMix(11, m); 
                ndspChnWaveBufClear(11); 
                sDarkRoomEnter.status = NDSP_WBUF_FREE; 
                ndspChnWaveBufAdd(11, &sDarkRoomEnter); 
            } 
        }
        
        // --- Elevator Logic ---
        if (inElevator && !elevatorDoorsOpen) { 
            if (elevatorTimer == 796 && audio_ok && sElevatorJam.data_vaddr) {
                ndspChnWaveBufClear(9);
                sElevatorJam.status = NDSP_WBUF_FREE;
                ndspChnWaveBufAdd(9, &sElevatorJam);
            } 
            if (elevatorTimer > 0) elevatorTimer--; 
            
            bool timeToOpen = false; 
            if (audio_ok && sElevatorJam.data_vaddr) {
                if (elevatorTimer < 794 && sElevatorJam.status == NDSP_WBUF_DONE && !elevatorJamFinished) timeToOpen = true;
            } else if (elevatorTimer <= 0 && !elevatorJamFinished) {
                timeToOpen = true;
            }
            
            if (timeToOpen) {
                elevatorJamFinished = true;
                elevatorDoorsOpen = true;
                needsVBOUpdate = true;
                if (audio_ok) {
                    ndspChnWaveBufClear(9);
                    if (sElevatorJamEnd.data_vaddr) {
                        sElevatorJamEnd.status = NDSP_WBUF_FREE;
                        ndspChnWaveBufAdd(9, &sElevatorJamEnd);
                    }
                }
            } 
        }
        
        if (elevatorDoorsOpen && !elevatorClosing && elevatorDoorOffset < 2.0f) {
            elevatorDoorOffset += 0.06f;
            needsVBOUpdate = true;
        } 
        if (elevatorDoorsOpen && !elevatorClosing && camZ < 2.0f) {
            inElevator = false;
            elevatorClosing = true;
            needsVBOUpdate = true;
        } 
        if (elevatorClosing && elevatorDoorOffset > 0.0f) {
            elevatorDoorOffset -= 0.08f;
            if (elevatorDoorOffset <= 0.0f) elevatorDoorOffset = 0.0f;
            needsVBOUpdate = true;
        }

        // --- Animations ---
        bool animActive = false;
        if (playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) {
            auto stepAnim = [&](bool target, float& val) { 
                if (target && val < 1.0f) { val += 0.15f; if (val > 1.0f) val = 1.0f; return true; } 
                if (!target && val > 0.0f) { val -= 0.15f; if (val < 0.0f) val = 0.0f; return true; } 
                return false; 
            };
            for (int s = 0; s < 3; s++) { 
                if (stepAnim(rooms[playerCurrentRoom].drawerOpen[s], rooms[playerCurrentRoom].animMain[s])) animActive = true; 
                if (rooms[playerCurrentRoom].hasLeftRoom) { 
                    if (stepAnim(rooms[playerCurrentRoom].leftRoomDrawerOpenL[s], rooms[playerCurrentRoom].animLL[s])) animActive = true; 
                    if (stepAnim(rooms[playerCurrentRoom].leftRoomDrawerOpenR[s], rooms[playerCurrentRoom].animLR[s])) animActive = true; 
                } 
                if (rooms[playerCurrentRoom].hasRightRoom) { 
                    if (stepAnim(rooms[playerCurrentRoom].rightRoomDrawerOpenL[s], rooms[playerCurrentRoom].animRL[s])) animActive = true; 
                    if (stepAnim(rooms[playerCurrentRoom].rightRoomDrawerOpenR[s], rooms[playerCurrentRoom].animRR[s])) animActive = true; 
                } 
            }
        } 
        if (animActive) needsVBOUpdate = true;

        // --- Bottom Screen UI ---
        if (totalFrames % 5 == 0) {
            static bool prevScreech = false; 
            if (screechActive != prevScreech) { consoleClear(); prevScreech = screechActive; }
            
            printf("\x1b[1;1H==============================\n");
            if (isDead) { 
                printf("         YOU DIED!            \n==============================\n\n                              \n\n\n    [PRESS START TO RESTART]  \n\x1b[0J"); 
            } else if (screechActive) { 
                printf("  >> SCREECH ATTACK!! <<      \n\n     (PSST!)                  \n   LOOK AROUND QUICKLY!       \n\n\x1b[0J"); 
            } else { 
                printf("        PLAYER STATUS         \n==============================\n\n"); 
                int dC = getDisplayRoom(playerCurrentRoom);
                int nD = getNextDoorIndex(playerCurrentRoom);
                int dN = getDisplayRoom(nD); 
                
                if (playerCurrentRoom == -1) { 
                    printf(" Current Room : 000 (Lobby) \x1b[K\n"); 
                    if (isGlitch) {
                        char g2[4]; 
                        for(int i=0; i<3; i++) g2[i] = symbols[rand()%8]; 
                        g2[3] = '\0';
                        printf(" Next Door     : %s         \x1b[K\n", g2);
                    } else {
                        printf(" Next Door     : 001         \x1b[K\n"); 
                    }
                    printf("                            \x1b[K\n\n"); 
                } else if (isGlitch) { 
                    char g1[4], g2[4];
                    for(int i=0; i<3; i++) { g1[i] = symbols[rand()%8]; g2[i] = symbols[rand()%8]; }
                    g1[3] = '\0'; g2[3] = '\0'; 
                    printf(" Current Room : %s         \x1b[K\n Next Door     : %s         \x1b[K\n                            \x1b[K\n\n", g1, g2); 
                } else {
                    printf(" Current Room : %03d         \x1b[K\n Next Door     : %03d         \x1b[K\n                            \x1b[K\n\n", dC, dN);
                }
                
                // Read Door Plaques
                if (nD >= 0 && nD < TOTAL_ROOMS && fabsf(camZ - (-10.0f - (nD * 10.0f))) < 4.0f && fabsf(camX) < 2.0f) { 
                    if (isGlitch && tDR == nD) { 
                        if (camX < -1.4f) printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n", rooms[tDR].dupeNumbers[0]); 
                        else if (camX >= -1.4f && camX <= 0.6f) printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n", rooms[tDR].dupeNumbers[1]); 
                        else printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n", rooms[tDR].dupeNumbers[2]); 
                    } else {
                        printf(" >> PLAQUE READS: %03d <<  \x1b[K\n\n", dN); 
                    }
                } else {
                    printf("                           \x1b[K\n\n");
                }
                
                printf(" Health       : %d / 100   \x1b[K\n Golden Key   : %s         \x1b[K\n Coins        : %04d       \x1b[K\n FPS          : %.2f       \x1b[K\n\n        --- CONTROLS ---      \x1b[K\n [A] Interact  [B] Crouch    \x1b[K\n [X] Hide(Cab/Bed) [CPAD] Move \x1b[K\n [TOUCH/CSTICK] Look Around  \x1b[K\n", playerHealth, hasKey?"EQUIPPED":"None    ", playerCoins, currentFps);
                
                if (texErrorMessage[0] != '\0') printf("\n \x1b[31m[TEX ERROR] %s\x1b[0m \x1b[K\n", texErrorMessage);
                
                if (messageTimer > 0) printf("\n ** %s ** \x1b[K\n", uiMessage);
                else if (rushActive && rushState == 1) printf("\n ** The lights are flickering... ** \x1b[K\n");
                else if (hideState == BEHIND_DOOR) printf("\n ** Hiding behind door... ** \x1b[K\n");
                else printf("\n                                    \x1b[K\n");
                
                if (!audio_ok) printf("\x1b[31m WARNING: dspfirm.cdc MISSING!\x1b[0m\x1b[K\n\x1b[31m Sound chip could not turn on.\x1b[0m\x1b[K\n"); 
                else printf("                                    \x1b[K\n"); 
                
                printf("\x1b[0J");
            }
        }

        if (!isDead) {
            
            // --- The Figure ---
            if (playerCurrentRoom == 49 && !figureActive) { 
                figureActive = true; 
                figureZ = -10.0f - (49 * 10.0f) - 5.0f; 
                sprintf(uiMessage, "Shh... He can hear you."); 
                messageTimer = 75; 
            }
            if (figureActive) { 
                float distSq = (camX - figureX)*(camX - figureX) + (camZ - figureZ)*(camZ - figureZ); 
                if (figureState == 0) { 
                    if (!isCrouching && distSq < 16.0f) figureState = 2; 
                } else if (figureState == 2) { 
                    if (camX > figureX) figureX += figureSpeed; else figureX -= figureSpeed; 
                    if (camZ > figureZ) figureZ += figureSpeed; else figureZ -= figureSpeed; 
                    if (distSq < 0.64f) { 
                        playerHealth = 0; 
                        isDead = true; 
                        flashRedFrames = 15; 
                        sprintf(uiMessage, "The Figure found you..."); 
                        messageTimer = 60; 
                    } 
                } 
            }
            
            // --- Seek Logic ---
            if (playerCurrentRoom >= seekStartRoom && playerCurrentRoom <= seekStartRoom + 2) { 
                if (camZ < -10.0f - ((seekStartRoom + 2) * 10.0f) - 8.0f && seekState == 0) {
                    seekState = 1; seekActive = true; seekTimer = 0; seekSpeed = 0.0f; 
                    seekZ = -10.0f - (seekStartRoom * 10.0f); needsVBOUpdate = true;
                    if (audio_ok && sSeekRise.data_vaddr) {
                        ndspChnWaveBufClear(7);
                        sSeekRise.status = NDSP_WBUF_FREE;
                        ndspChnWaveBufAdd(7, &sSeekRise);
                    }
                } 
            }
            
            if (seekState == 1) { 
                seekTimer++; 
                if (seekTimer >= 90 && seekTimer < 115) {
                    if (seekSpeed < 0.24f) seekSpeed += 0.01f;
                    seekZ -= seekSpeed;
                } 
                if (seekTimer >= 115) {
                    seekState = 2; seekSpeed = 0; 
                    sprintf(uiMessage, "RUN!"); messageTimer = 45;
                    if (audio_ok && sSeekChase.data_vaddr) {
                        ndspChnWaveBufClear(7);
                        sSeekChase.status = NDSP_WBUF_FREE;
                        ndspChnWaveBufAdd(7, &sSeekChase);
                    }
                } 
            } else if (seekState == 2) { 
                seekSpeed = (seekZ > -10.0f - ((seekStartRoom + 2) * 10.0f)) ? 0.13f : seekMaxSpeed; 
                seekZ -= seekSpeed; 
                
                if (audio_ok && sSeekChase.data_vaddr) { 
                    float mix[12] = {0}; mix[0] = 0.8f; mix[1] = 0.8f; ndspChnSetMix(7, mix); 
                } 
                
                if (fabsf(seekZ - camZ) < 1.2f) {
                    playerHealth = 0; isDead = true; 
                    sprintf(uiMessage, "Seek caught you..."); messageTimer = 60;
                    if (audio_ok) ndspChnWaveBufClear(7);
                } 
                
                if (playerCurrentRoom >= 0 && rooms[playerCurrentRoom].isSeekFinale) {
                    for(int h = 0; h < 6; h++) {
                        if (fabsf(camX - rooms[playerCurrentRoom].pW[h]) < 0.6f && fabsf(camZ - rooms[playerCurrentRoom].pZ[h]) < 0.6f) {
                            if (rooms[playerCurrentRoom].pSide[h] == 0 && !isDead && messageTimer <= 0) {
                                playerHealth -= 40; camZ += 1.5f; 
                                sprintf(uiMessage, "Burned! (-40 HP)"); messageTimer = 30;
                                if (playerHealth <= 0) { isDead = true; if (audio_ok) ndspChnWaveBufClear(7); }
                            } else if (rooms[playerCurrentRoom].pSide[h] == 1 && !isDead && !isCrouching) {
                                playerHealth = 0; isDead = true; 
                                sprintf(uiMessage, "The hands grabbed you!"); messageTimer = 60;
                                if (audio_ok) ndspChnWaveBufClear(7);
                            }
                        }
                    }
                } 
                
                if (seekActive) { 
                    float fLZ = -10.0f - ((seekStartRoom + 8) * 10.0f) - 10.0f; 
                    int sR = seekStartRoom + 9; 
                    bool playerSafe = (camZ < fLZ - 1.5f); 
                    
                    if (playerSafe || seekZ < fLZ + 3.0f) { 
                        if (!rooms[sR].isJammed) {
                            doorOpen[sR] = false; rooms[sR].isLocked = false; rooms[sR].isJammed = true; 
                            needsVBOUpdate = true;
                            if (audio_ok) {
                                if (sDoor.data_vaddr) {
                                    ndspChnWaveBufClear(1); sDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sDoor);
                                }
                                ndspChnWaveBufClear(7);
                                if (sSeekEscaped.data_vaddr) {
                                    sSeekEscaped.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(7, &sSeekEscaped);
                                }
                            }
                        }
                    } 
                    if (playerSafe) {
                        seekActive = false; seekState = 0; 
                        sprintf(uiMessage, "You escaped!"); messageTimer = 75; 
                        needsVBOUpdate = true;
                    } else if (seekZ < fLZ + 3.0f) {
                        playerHealth = 0; isDead = true; 
                        sprintf(uiMessage, "The door slammed shut!"); messageTimer = 60; 
                        seekActive = false; seekState = 0; needsVBOUpdate = true;
                        if (audio_ok) ndspChnWaveBufClear(7);
                    } 
                } 
            }
            
            // --- Eyes Logic ---
            bool inE = (playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS && rooms[playerCurrentRoom].hasEyes); 
            if (inE && !inEyesRoom) { 
                inEyesRoom = true; eyesGraceTimer = 30; 
                if (audio_ok) {
                    ndspChnWaveBufClear(4);
                    if (sEyesAppear.data_vaddr) { sEyesAppear.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(4, &sEyesAppear); }
                    ndspChnWaveBufClear(5);
                    if (sEyesGarble.data_vaddr) { 
                        float m[12] = {0}; m[0] = 1.8f; m[1] = 1.8f; ndspChnSetMix(5, m); 
                        sEyesGarble.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(5, &sEyesGarble); 
                    }
                } 
            } else if (!inE && inEyesRoom) {
                inEyesRoom = false;
                if (audio_ok) ndspChnWaveBufClear(5);
            } 
            
            if (inE) {
                if (eyesGraceTimer > 0) eyesGraceTimer--;
                if (hideState == NOT_HIDING) { 
                    float vx = rooms[playerCurrentRoom].eyesX - camX;
                    float vy = rooms[playerCurrentRoom].eyesY - (isCrouching ? 0.4f : 0.9f);
                    float vz = rooms[playerCurrentRoom].eyesZ - camZ;
                    float distSq = vx*vx + vy*vy + vz*vz; 
                    
                    if (distSq > 0) { float dist = sqrtf(distSq); vx /= dist; vy /= dist; vz /= dist; } 
                    
                    float fx = -sinf(camYaw) * cosf(camPitch);
                    float fy = sinf(camPitch);
                    float fz = -cosf(camYaw) * cosf(camPitch);
                    float dP = (fx*vx) + (fy*vy) + (fz*vz); 
                    
                    if (dP > 0.85f && checkLineOfSight(camX, (isCrouching ? 0.4f : 0.9f), camZ, rooms[playerCurrentRoom].eyesX, rooms[playerCurrentRoom].eyesY, rooms[playerCurrentRoom].eyesZ)) { 
                        if (eyesGraceTimer <= 0) { 
                            if (!isLookingAtEyes) {
                                isLookingAtEyes = true; eyesDamageTimer = 2; eyesDamageAccumulator = 4;
                                if (audio_ok && sEyesAttack.data_vaddr && eyesSoundCooldown <= 0) {
                                    ndspChnWaveBufClear(4); sEyesAttack.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(4, &sEyesAttack);
                                    eyesSoundCooldown = 45;
                                }
                            } 
                            if (audio_ok && sEyesAttack.data_vaddr && (sEyesAttack.status == NDSP_WBUF_DONE || sEyesAttack.status == NDSP_WBUF_FREE) && eyesSoundCooldown <= 0) {
                                ndspChnWaveBufClear(4); sEyesAttack.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(4, &sEyesAttack);
                                eyesSoundCooldown = 45;
                            } 
                            
                            eyesDamageTimer++; 
                            if (eyesDamageTimer >= 3) {
                                playerHealth -= 1; eyesDamageTimer = 0; flashRedFrames = 1; eyesDamageAccumulator++;
                                if (eyesDamageAccumulator >= 5) {
                                    eyesDamageAccumulator = 0;
                                    if (audio_ok && sEyesHit.data_vaddr) {
                                        ndspChnWaveBufClear(6); sEyesHit.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(6, &sEyesHit);
                                    }
                                }
                            } 
                            if (playerHealth <= 0) {
                                isDead = true; sprintf(uiMessage, "You stared at Eyes!"); messageTimer = 60;
                            } 
                        } 
                    } else {
                        isLookingAtEyes = false; eyesDamageTimer = 0; eyesDamageAccumulator = 0;
                    } 
                } else {
                    isLookingAtEyes = false; eyesDamageTimer = 0; eyesDamageAccumulator = 0;
                }
            } else {
                isLookingAtEyes = false; eyesDamageTimer = 0; eyesDamageAccumulator = 0;
            }
            
            // --- Screech Logic ---
            bool iSE = (playerCurrentRoom >= seekStartRoom - 5 && playerCurrentRoom <= seekStartRoom + 9); 
            int sC = (playerCurrentRoom > 0 && rooms[playerCurrentRoom].lightLevel < 0.5f) ? 400 : 12000;
            
            if (!screechActive && screechCooldown <= 0 && hideState == NOT_HIDING && playerCurrentRoom > 0 && !iSE && (rand() % sC == 0)) { 
                screechActive = true; screechState = 1; screechTimer = 120; 
                float aO = 1.57f + ((rand() % 200) / 100.0f) * 1.57f;
                float sY = camYaw + aO; 
                screechOffsetX = -sinf(sY) * 1.2f; 
                screechOffsetZ = -cosf(sY) * 1.2f; 
                screechOffsetY = (rand() % 8) / 10.0f; 
                
                if (audio_ok) {
                    ndspChnWaveBufClear(0);
                    if (sPsst.data_vaddr) { sPsst.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(0, &sPsst); }
                } 
            }
            
            if (screechActive) { 
                if (screechState == 1) {
                    screechTimer--; 
                    float cOX = screechOffsetX, cOZ = screechOffsetZ; 
                    float pY = isCrouching ? 0.4f : 0.9f; 
                    float nSX = camX + cOX, nSZ = camZ + cOZ, nSY = 0.8f + screechOffsetY;
                    int att = 0; float currentDist = 1.2f; 
                    
                    while ((!checkLineOfSight(camX, pY, camZ, nSX, nSY, nSZ) || checkCollision(nSX, nSY, nSZ, 0.4f)) && att < 10 && currentDist > 1.2f) { 
                        cOX *= 0.72f; cOZ *= 0.72f; currentDist *= 0.72f; nSX = camX + cOX; nSZ = camZ + cOZ; att++; 
                    }
                    if (screechX != nSX || screechZ != nSZ) { screechX = nSX; screechZ = nSZ; screechY = nSY; } 
                    
                    float vx = screechX - camX, vy = screechY - (isCrouching ? 0.4f : 0.9f), vz = screechZ - camZ;
                    float distSq = vx*vx + vy*vy + vz*vz; 
                    if (distSq > 0.0f) { float dist = sqrtf(distSq); vx /= dist; vy /= dist; vz /= dist; } 
                    
                    float fx = -sinf(camYaw) * cosf(camPitch), fy = sinf(camPitch), fz = -cosf(camYaw) * cosf(camPitch);
                    float dP = (fx*vx) + (fy*vy) + (fz*vz); 
                    
                    if (dP > 0.85f) { 
                        screechState = 2; screechTimer = 15; sprintf(uiMessage, "Dodged Screech!"); messageTimer = 45; 
                        if (audio_ok) { ndspChnWaveBufClear(0); if (sCaught.data_vaddr) { sCaught.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(0, &sCaught); } } 
                    } else if (screechTimer <= 0) { 
                        screechState = 3; screechTimer = 24; playerHealth -= 20; 
                        sprintf(uiMessage, "Screech bit you! (-20 HP)"); messageTimer = 45; 
                        if (playerHealth <= 0) isDead = true; 
                        if (audio_ok) { ndspChnWaveBufClear(0); if (sAttack.data_vaddr) { sAttack.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(0, &sAttack); } } 
                    } 
                } else if (screechState == 2) {
                    screechTimer--; 
                    float escX = screechOffsetX, escZ = screechOffsetZ, escDist = sqrtf(escX*escX + escZ*escZ);
                    if (escDist > 0.001f) { escX /= escDist; escZ /= escDist; } 
                    
                    screechOffsetX += escX * 0.3f; screechOffsetZ += escZ * 0.3f; screechY += 0.18f; 
                    screechX = camX + screechOffsetX; screechZ = camZ + screechOffsetZ;
                    if (screechTimer <= 0) { screechActive = false; screechState = 0; screechCooldown = 900; }
                } else if (screechState == 3) {
                    screechTimer--; 
                    screechOffsetX = -sinf(camYaw) * 0.4f; screechOffsetZ = -cosf(camYaw) * 0.4f; 
                    screechX = camX + screechOffsetX; screechZ = camZ + screechOffsetZ; 
                    screechY = (isCrouching ? 0.4f : 0.9f) - 0.2f;
                    
                    if (screechTimer > 18) flashRedFrames = 0; 
                    else if (screechTimer > 12) flashRedFrames = 2; 
                    else if (screechTimer > 6) flashRedFrames = 0; 
                    else flashRedFrames = 2;
                    
                    if (screechTimer <= 0) { screechActive = false; screechState = 0; screechCooldown = 900; flashRedFrames = 0; }
                }
            }

            // --- Rush Logic ---
            if (rushActive) { 
                if (rushState == 1) {
                    rushTimer--;
                    if (rushTimer % 5 == 0) {
                        if (playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) {
                            rooms[playerCurrentRoom].lightLevel = (rand() % 2 == 0) ? 0.3f : 1.0f;
                            needsVBOUpdate = true;
                        }
                    }
                    if (rushTimer <= 0) {
                        rushState = 2; rushZ = camZ + 40.0f; rushTargetZ = camZ - 60.0f;
                    }
                } else if (rushState == 2) {
                    rushZ -= 1.6f;
                    if (playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) {
                        if (rooms[playerCurrentRoom].lightLevel != 0.3f) { rooms[playerCurrentRoom].lightLevel = 0.3f; needsVBOUpdate = true; }
                        if (playerCurrentRoom + 1 < TOTAL_ROOMS && rooms[playerCurrentRoom + 1].lightLevel != 0.3f) { rooms[playerCurrentRoom + 1].lightLevel = 0.3f; needsVBOUpdate = true; }
                    }
                    if (fabsf(rushZ - camZ) < 3.0f && fabsf(camX) < 3.0f && hideState == NOT_HIDING) {
                        playerHealth = 0; isDead = true; flashRedFrames = 15;
                    }
                    if (rushZ < rushTargetZ) {
                        rushActive = false; rushState = 0; rushCooldown = 900;
                        if (audio_ok) ndspChnWaveBufClear(3);
                    }
                } 
                if (audio_ok && sRushScream.data_vaddr) {
                    float dist = (rushState == 1) ? 40.0f + (rushTimer / rushStartTimer) * 110.0f : (fabsf(rushZ - camZ) * (rushZ < camZ ? 1.5f : 1.0f));
                    float vol = 1.0f - (dist / 150.0f);
                    if (vol < 0) vol = 0; if (vol > 1) vol = 1; vol = vol * vol * vol;
                    float mix[12] = {0}; mix[0] = vol * 3.5f; mix[1] = vol * 3.5f; ndspChnSetMix(3, mix);
                } 
            }
            
            // --- Hiding in Closets / Under Beds ---
            if (kDown & KEY_X && hideState == NOT_HIDING) { 
                float r = 0.5f; 
                for (auto& b : collisions) { 
                    if ((b.type == 1 || b.type == 2) && camX + r > b.minX && camX - r < b.maxX && camZ + r > b.minZ && camZ - r < b.maxZ) { 
                        float rCZ = (b.minZ + b.maxZ) / 2.0f, cCX = ((b.minX + b.maxX) / 2.0f);
                        float tOX = (cCX < -3.0f) ? -6.0f : ((cCX > 3.0f) ? 6.0f : 0.0f); 
                        
                        if (b.type == 1) { 
                            hideState = IN_CABINET; camZ = rCZ; camPitch = 0; 
                            camX = (cCX - tOX < 0) ? -2.5f + tOX : 2.5f + tOX; 
                            camYaw = (cCX - tOX < 0) ? -1.57f : 1.57f; 
                        } else { 
                            hideState = UNDER_BED; camZ = rCZ; camPitch = 0; 
                            camX = (cCX - tOX < 0) ? -2.2f + tOX : 2.2f + tOX; 
                            camYaw = (cCX - tOX < 0) ? -1.57f : 1.57f; 
                        } 
                        
                        isCrouching = false; needsVBOUpdate = true; 
                        if (audio_ok && sWardrobeEnter.data_vaddr) { ndspChnWaveBufClear(10); sWardrobeEnter.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(10, &sWardrobeEnter); } 
                        break; 
                    } 
                } 
            } else if (kDown & KEY_X) { 
                if (hideState == IN_CABINET || hideState == UNDER_BED) {
                    float tOX = (camX < -3.0f) ? -6.0f : ((camX > 3.0f) ? 6.0f : 0.0f);
                    camX = (camX - tOX < 0) ? -1.4f + tOX : 1.4f + tOX; 
                    camYaw = (camX - tOX < 0) ? -1.57f : 1.57f; 
                    hideState = NOT_HIDING; needsVBOUpdate = true; 
                    if (audio_ok && sWardrobeExit.data_vaddr) { ndspChnWaveBufClear(10); sWardrobeExit.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(10, &sWardrobeExit); }
                } 
            }
            
            // --- Interactions (Doors, Drawers, Chests, Items) ---
            bool iA = false;
            
            // Dupe Door Logic
            if (!iA && (kDown & KEY_A)) { 
                int nI = getNextDoorIndex(playerCurrentRoom); 
                if (nI >= 0 && nI < TOTAL_ROOMS && rooms[nI].isDupeRoom) { 
                    float dZ = -10.0f - (nI * 10.0f); 
                    if (fabsf(camZ - dZ) < 2.5f) { 
                        float fx = -sinf(camYaw), fz = -cosf(camYaw); 
                        int tD = -1; float bD = 0.85f; 
                        for (int d = 0; d < 3; d++) { 
                            float dx = (-2.0f + d * 2.0f) - camX, dz = dZ - camZ, distSq = dx*dx + dz*dz; 
                            if (distSq > 0 && distSq < 9.0f) { 
                                float dist = sqrtf(distSq); 
                                float dot = (fx * (dx / dist)) + (fz * (dz / dist)); 
                                if (dot > bD) { bD = dot; tD = d; } 
                            } 
                        } 
                        if (tD != -1) { 
                            if (tD == rooms[nI].correctDupePos) { 
                                if (!doorOpen[nI]) {
                                    if (audio_ok && sDoor.data_vaddr) { ndspChnWaveBufClear(1); sDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sDoor); }
                                    doorOpen[nI] = true; needsVBOUpdate = true;
                                } 
                            } else { 
                                if (audio_ok && sDupeAttack.data_vaddr) { ndspChnWaveBufClear(2); sDupeAttack.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(2, &sDupeAttack); }
                                playerHealth -= 34; flashRedFrames = 8; camZ += 2.0f; 
                                if (playerHealth <= 0) isDead = true; 
                            } 
                            iA = true; 
                        } 
                    } 
                } 
            }
            
            // Drawer & Chest Logic
            if ((kDown & KEY_A) || (kDown & KEY_X && hideState == NOT_HIDING)) {
                auto cI = [&](int ty, float zC, float sX, bool& iO, int& it) { 
                    if (ty != 0) {
                        float dx = sX - camX, dz = zC - camZ, distSq = dx*dx + dz*dz;
                        if (distSq < 6.25f) {
                            float dist = sqrtf(distSq);
                            float fx = -sinf(camYaw), fz = -cosf(camYaw), dot = (fx*(dx/dist)) + (fz*(dz/dist));
                            if (dot > 0.7f) {
                                float aX = sX - (dx/dist)*0.5f, aZ = zC - (dz/dist)*0.5f; 
                                float targetY = (ty == 3 || ty == 4) ? 0.7f : 0.5f; 
                                if (checkLineOfSight(camX, (isCrouching ? 0.4f : 0.9f), camZ, aX, targetY, aZ)) {
                                    if (kDown & KEY_X && (ty == 5 || ty == 6)) {
                                        iO = !iO; needsVBOUpdate = true;
                                        if (audio_ok) {
                                            ndspChnWaveBufClear(10);
                                            if (iO && sDrawerOpen.data_vaddr) { sDrawerOpen.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(10, &sDrawerOpen); }
                                            else if (!iO && sDrawerClose.data_vaddr) { sDrawerClose.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(10, &sDrawerClose); }
                                        }
                                        return true;
                                    } 
                                    if (kDown & KEY_A) { 
                                        if ((ty == 5 || ty == 6) && iO) {
                                            if (it == 1) { hasKey = true; it = 0; needsVBOUpdate = true; sprintf(uiMessage, "Grabbed the Golden Key!"); messageTimer = 45; return true; }
                                            else if (it == 2) { playerHealth += 10; if (playerHealth > 100) playerHealth = 100; it = 0; needsVBOUpdate = true; sprintf(uiMessage, "Used a Bandaid! (+10 HP)"); messageTimer = 45; return true; }
                                            else if (it == 3) { playerCoins += (rand() % 15) + 5; it = 0; needsVBOUpdate = true; sprintf(uiMessage, "Looted some Coins!"); messageTimer = 45; if (audio_ok && sCoinsCollect.data_vaddr) { ndspChnWaveBufClear(10); sCoinsCollect.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(10, &sCoinsCollect); } return true; }
                                            else { sprintf(uiMessage, "Drawer is empty..."); messageTimer = 30; return true; }
                                        } else if ((ty == 3 || ty == 4)) {
                                            if (it == 1) { hasKey = true; it = 0; needsVBOUpdate = true; sprintf(uiMessage, "Grabbed Key off the bed!"); messageTimer = 45; return true; }
                                            else if (it == 3) { playerCoins += (rand() % 15) + 5; it = 0; needsVBOUpdate = true; sprintf(uiMessage, "Looted Coins off the bed!"); messageTimer = 45; if (audio_ok && sCoinsCollect.data_vaddr) { ndspChnWaveBufClear(10); sCoinsCollect.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(10, &sCoinsCollect); } return true; }
                                        }
                                    } 
                                }
                            }
                        }
                    } 
                    return false; 
                };
                
                auto cC = [&](float cX, float cZ, bool& iO) { 
                    if ((kDown & KEY_X) && !iO) {
                        float dx = cX - camX, dz = cZ - camZ, distSq = dx*dx + dz*dz;
                        if (distSq < 6.25f) {
                            float dist = sqrtf(distSq);
                            float fx = -sinf(camYaw), fz = -cosf(camYaw), dot = (fx*(dx/dist)) + (fz*(dz/dist));
                            if (dot > 0.7f) {
                                float aX = cX - (dx/dist)*0.5f, aZ = cZ - (dz/dist)*0.5f;
                                if (checkLineOfSight(camX, (isCrouching ? 0.4f : 0.9f), camZ, aX, 0.3f, aZ)) {
                                    iO = true; needsVBOUpdate = true; playerCoins += 20; 
                                    sprintf(uiMessage, "Opened Chest!"); messageTimer = 45;
                                    if (audio_ok && sDrawerOpen.data_vaddr) { ndspChnWaveBufClear(10); sDrawerOpen.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(10, &sDrawerOpen); }
                                    return true;
                                }
                            }
                        }
                    } 
                    return false; 
                };
                
                // Elevator Exit
                if (inElevator && !elevatorDoorsOpen && camX > 0 && camZ > 5.0f && camZ < 8.0f) { 
                    elevatorJamFinished = true; elevatorDoorsOpen = true; iA = true; needsVBOUpdate = true;
                    if (audio_ok && sElevatorJamEnd.data_vaddr) { ndspChnWaveBufClear(9); sElevatorJamEnd.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(9, &sElevatorJamEnd); } 
                }
                
                // Checking current room for interactables
                if (!iA && playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) { 
                    for (int s = 0; s < 3; s++) {
                        if (cI(rooms[playerCurrentRoom].slotType[s], (-10.0f - (playerCurrentRoom * 10.0f)) - 2.5f - (s * 2.5f), ((rooms[playerCurrentRoom].slotType[s] % 2 != 0) ? -2.4f : 2.4f), rooms[playerCurrentRoom].drawerOpen[s], rooms[playerCurrentRoom].slotItem[s])) { iA = true; break; } 
                    }
                    if (!iA && rooms[playerCurrentRoom].hasLeftRoom) { 
                        float srZ = (-10.0f - (playerCurrentRoom * 10.0f)) + rooms[playerCurrentRoom].leftDoorOffset + 2.5f; 
                        for (int s = 0; s < 3; s++) {
                            float fZ = srZ - 0.9f - (s * 1.6f);
                            int tL = rooms[playerCurrentRoom].leftRoomSlotTypeL[s];
                            if (tL > 0 && tL != 99) {
                                if (tL == 7 || tL == 8) { if (cC(-8.4f, fZ, rooms[playerCurrentRoom].leftRoomDrawerOpenL[s])) { iA = true; break; } }
                                else if (cI(tL, fZ, -8.4f, rooms[playerCurrentRoom].leftRoomDrawerOpenL[s], rooms[playerCurrentRoom].leftRoomSlotItemL[s])) { iA = true; break; }
                            } 
                            int tR = rooms[playerCurrentRoom].leftRoomSlotTypeR[s];
                            if (tR > 0 && tR != 99) {
                                if (tR == 7 || tR == 8) { if (cC(-3.6f, fZ, rooms[playerCurrentRoom].leftRoomDrawerOpenR[s])) { iA = true; break; } }
                                else if (cI(tR, fZ, -3.6f, rooms[playerCurrentRoom].leftRoomDrawerOpenR[s], rooms[playerCurrentRoom].leftRoomSlotItemR[s])) { iA = true; break; }
                            } 
                        } 
                    } 
                    if (!iA && rooms[playerCurrentRoom].hasRightRoom) { 
                        float srZ = (-10.0f - (playerCurrentRoom * 10.0f)) + rooms[playerCurrentRoom].rightDoorOffset + 2.5f; 
                        for (int s = 0; s < 3; s++) {
                            float fZ = srZ - 0.9f - (s * 1.6f);
                            int tL = rooms[playerCurrentRoom].rightRoomSlotTypeL[s];
                            if (tL > 0 && tL != 99) {
                                if (tL == 7 || tL == 8) { if (cC(3.6f, fZ, rooms[playerCurrentRoom].rightRoomDrawerOpenL[s])) { iA = true; break; } }
                                else if (cI(tL, fZ, 3.6f, rooms[playerCurrentRoom].rightRoomDrawerOpenL[s], rooms[playerCurrentRoom].rightRoomSlotItemL[s])) { iA = true; break; }
                            } 
                            int tR = rooms[playerCurrentRoom].rightRoomSlotTypeR[s];
                            if (tR > 0 && tR != 99) {
                                if (tR == 7 || tR == 8) { if (cC(8.4f, fZ, rooms[playerCurrentRoom].rightRoomDrawerOpenR[s])) { iA = true; break; } }
                                else if (cI(tR, fZ, 8.4f, rooms[playerCurrentRoom].rightRoomDrawerOpenR[s], rooms[playerCurrentRoom].rightRoomSlotItemR[s])) { iA = true; break; }
                            } 
                        } 
                    } 
                }
                
                // Lobby Key Pickup
                if (!iA && !lobbyKeyPickedUp && rooms[0].isLocked && camX < -3.5f && camZ < -8.5f) {
                    lobbyKeyPickedUp = true; hasKey = true; needsVBOUpdate = true; 
                    sprintf(uiMessage, "Found the Lobby Key!"); messageTimer = 45; iA = true;
                    if (audio_ok && sCoinsCollect.data_vaddr) { ndspChnWaveBufClear(10); sCoinsCollect.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(10, &sCoinsCollect); }
                }
                
                // Unlocking Doors
                if (!iA) { 
                    for (int i = 0; i < TOTAL_ROOMS; i++) { 
                        if (i == seekStartRoom + 1 || i == seekStartRoom + 2) continue; 
                        if (rooms[i].isLocked || rooms[i].isJammed) { 
                            float dZ = -10.0f - (i * 10.0f), dX = (rooms[i].doorPos == 0) ? -2.0f : ((rooms[i].doorPos == 1) ? 0.0f : 2.0f); 
                            if (fabsf(camZ - dZ) < 2.5f && fabsf(camX - dX) < 2.0f) { 
                                if (rooms[i].isJammed) {
                                    if (audio_ok && sLockedDoor.data_vaddr) { ndspChnWaveBufClear(1); sLockedDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sLockedDoor); }
                                    sprintf(uiMessage, "The door is jammed shut!"); messageTimer = 30;
                                } else if (hasKey) {
                                    rooms[i].isLocked = false; hasKey = false; needsVBOUpdate = true; 
                                    sprintf(uiMessage, "Door Unlocked!"); messageTimer = 30;
                                } else {
                                    if (audio_ok && sLockedDoor.data_vaddr) { ndspChnWaveBufClear(1); sLockedDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sLockedDoor); }
                                    sprintf(uiMessage, "It's locked..."); messageTimer = 30;
                                } 
                                iA = true; break; 
                            } 
                        } 
                    } 
                }
            } 

            // --- Movement & Input ---
            if ((kDown & KEY_B) && hideState == NOT_HIDING) { 
                if (isCrouching) { if (!checkCollision(camX, 0, camZ, 1.1f)) isCrouching = false; } 
                else { isCrouching = true; } 
            }
            
            // Spawning Rush randomly
            if (playerCurrentRoom > 1 && !rushActive && rushCooldown <= 0 && !iSE && rand() % 10000 < 2) {
                rushActive = true; rushState = 1; rushTimer = 150 + (rand() % 60); rushStartTimer = (float)rushTimer;
                if (audio_ok) {
                    if (sLightsFlicker.data_vaddr) { float m[12] = {0}; m[0] = 2.5f; m[1] = 2.5f; ndspChnSetMix(12, m); ndspChnWaveBufClear(12); sLightsFlicker.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(12, &sLightsFlicker); }
                    if (sRushScream.data_vaddr) { float m[12] = {0}; ndspChnSetMix(3, m); ndspChnWaveBufClear(3); sRushScream.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(3, &sRushScream); }
                }
            }
            
            // Door Hiding Logic
            if (hideState == NOT_HIDING || hideState == BEHIND_DOOR) { 
                bool iDZ = false; 
                for (auto& b : collisions) if (b.type == 4 && camX > b.minX && camX < b.maxX && camZ > b.minZ && camZ < b.maxZ) { iDZ = true; break; } 
                if (iDZ && hideState == NOT_HIDING) hideState = BEHIND_DOOR; 
                else if (!iDZ && hideState == BEHIND_DOOR) hideState = NOT_HIDING; 
            }
            
            float pH = isCrouching ? 0.5f : 1.1f; 
            circlePosition cS, cP; irrstCstickRead(&cS); hidCircleRead(&cP); touchPosition t; hidTouchRead(&t);
            
            float turnSpeed = 0.0f;
            bool isMoving = false;
            
            if ((hideState == NOT_HIDING || hideState == BEHIND_DOOR) && seekState != 1) {
                if (abs(cS.dx) > 10) {
                    turnSpeed = cS.dx / 1560.0f * 1.6f;
                    camYaw -= turnSpeed; 
                }
                if (abs(cS.dy) > 10) camPitch += cS.dy / 1560.0f * 1.6f;
                
                if (kHeld & KEY_TOUCH) { 
                    if (!wasTouching) { startTouchX = t.px; startTouchY = t.py; wasTouching = true; } 
                    else {
                        float dx = (float)t.px - startTouchX, dy = (float)t.py - startTouchY;
                        if (fabsf(dx) < 10.0f) dx = 0; if (fabsf(dy) < 10.0f) dy = 0;
                        float tTurn = (dx / 160.0f) * 0.12f;
                        camYaw -= tTurn; 
                        turnSpeed += tTurn; // Combine touch turning
                        camPitch -= (dy / 120.0f) * 0.12f;
                    } 
                } else { wasTouching = false; } 
                
                if (camPitch > 1.57f) camPitch = 1.57f; 
                if (camPitch < -1.57f) camPitch = -1.57f;
                
                if (abs(cP.dy) > 15 || abs(cP.dx) > 15) { 
                    isMoving = true;
                    float s = (seekState == 2) ? (isCrouching ? 0.50f : 0.84f) : (isCrouching ? 0.32f : 0.56f);
                    float sy = cP.dy / 1560.0f, sx = cP.dx / 1560.0f;
                    float nX = camX - (sinf(camYaw) * sy - cosf(camYaw) * sx) * s;
                    float nZ = camZ - (cosf(camYaw) * sy + sinf(camYaw) * sx) * s; 
                    if (!checkCollision(nX, 0, camZ, pH)) camX = nX; 
                    if (!checkCollision(camX, 0, nZ, pH)) camZ = nZ; 
                }
            }
            
            // --- Camera Effects Math ---
            float chaseMultiplier = (seekState == 2) ? 2.5f : 1.0f; // Multiplies chaos during Seek
            
            float targetRoll = turnSpeed * 0.6f * chaseMultiplier; 
            if (targetRoll > 0.08f * chaseMultiplier) targetRoll = 0.08f * chaseMultiplier; // Clamp max tilt
            if (targetRoll < -0.08f * chaseMultiplier) targetRoll = -0.08f * chaseMultiplier;
            currentRoll += (targetRoll - currentRoll) * 0.15f; // Smoothly lerp to tilt
            
            if (isMoving) {
                bobTime += (isCrouching ? 0.12f : 0.22f) * chaseMultiplier; 
            } else {
                bobTime += (round(bobTime / 3.14159f) * 3.14159f - bobTime) * 0.1f; // Smoothly return to center
            }
            
            camBobY = sinf(bobTime) * 0.05f * chaseMultiplier; // Vertical bounce
            camBobX = cosf(bobTime / 2.0f) * 0.025f * chaseMultiplier; // Horizontal sway
        } // End of if(!isDead)

        // --- Door Open/Close Checks ---
        // 1. Check Main Forward Doors
        for (int i = 0; i < TOTAL_ROOMS; i++) { 
            if (rooms[i].isDupeRoom || i == seekStartRoom+1 || i == seekStartRoom+2) continue; 
            float dZ = -10.0f - (i * 10.0f);
            float dX = (rooms[i].doorPos == 0) ? -2.0f : ((rooms[i].doorPos == 1) ? 0.0f : 2.0f);
            bool isClose = (fabsf(camZ - dZ) < 1.5f && fabsf(camX - dX) < 1.5f); 
            if (rooms[i].isLocked || rooms[i].isJammed) isClose = false; 
            
            if (doorOpen[i] != isClose) {
                doorOpen[i] = isClose;
                needsVBOUpdate = true;
                if (isClose && audio_ok && sDoor.data_vaddr) { ndspChnWaveBufClear(1); sDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sDoor); }
            }
        }

        // 2. Check Side Doors
        if (playerCurrentRoom >= 0 && playerCurrentRoom < TOTAL_ROOMS) {
            if (rooms[playerCurrentRoom].hasLeftRoom && !rooms[playerCurrentRoom].leftDoorOpen) {
                float lDoorZ = (-10.0f - (playerCurrentRoom * 10.0f)) + rooms[playerCurrentRoom].leftDoorOffset - 0.6f;
                if (fabsf(camZ - lDoorZ) < 2.0f && camX < -1.5f) {
                    rooms[playerCurrentRoom].leftDoorOpen = true;
                    needsVBOUpdate = true;
                    if (audio_ok && sDoor.data_vaddr) { ndspChnWaveBufClear(1); sDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sDoor); }
                }
            }
            if (rooms[playerCurrentRoom].hasRightRoom && !rooms[playerCurrentRoom].rightDoorOpen) {
                float rDoorZ = (-10.0f - (playerCurrentRoom * 10.0f)) + rooms[playerCurrentRoom].rightDoorOffset - 0.6f;
                if (fabsf(camZ - rDoorZ) < 2.0f && camX > 1.5f) {
                    rooms[playerCurrentRoom].rightDoorOpen = true;
                    needsVBOUpdate = true;
                    if (audio_ok && sDoor.data_vaddr) { ndspChnWaveBufClear(1); sDoor.status = NDSP_WBUF_FREE; ndspChnWaveBufAdd(1, &sDoor); }
                }
            }
        }

        // --- Mesh Updates ---
        if (needsVBOUpdate) {
            buildWorld(currentChunk, playerCurrentRoom);
            colored_size = world_mesh_colored.size();
            textured_size = world_mesh_textured.size();
            if (colored_size + textured_size > MAX_VERTS) textured_size = MAX_VERTS - colored_size; 
            memcpy(vbo_main, world_mesh_colored.data(), colored_size * sizeof(vertex));
            memcpy((vertex*)vbo_main + colored_size, world_mesh_textured.data(), textured_size * sizeof(vertex));
        }

        buildEntities(playerCurrentRoom);
        int world_total = colored_size + textured_size;
        int ent_col_size = entity_mesh_colored.size();
        int ent_tex_size = entity_mesh_textured.size();
        if (ent_col_size + ent_tex_size > MAX_ENTITY_VERTS) ent_tex_size = MAX_ENTITY_VERTS - ent_col_size; 
        
        if (ent_col_size > 0) memcpy((vertex*)vbo_main + world_total, entity_mesh_colored.data(), ent_col_size * sizeof(vertex));
        if (ent_tex_size > 0) memcpy((vertex*)vbo_main + world_total + ent_col_size, entity_mesh_textured.data(), ent_tex_size * sizeof(vertex));
        
        if (needsVBOUpdate) {
            GSPGPU_FlushDataCache(vbo_main, (world_total + ent_col_size + ent_tex_size) * sizeof(vertex));
        } else if ((ent_col_size + ent_tex_size) > 0) {
            GSPGPU_FlushDataCache((vertex*)vbo_main + world_total, (ent_col_size + ent_tex_size) * sizeof(vertex));
        }

        // ==========================================
        //               DRAWING PHASE
        // ==========================================
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW); 
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); 
        C3D_FrameDrawOn(target);
        
        float dCX = camX, dCZ = camZ, dCY = camYaw, dCP = camPitch; 
        static float lCCZ = 0.0f;
        
        if (seekState == 1) { 
            if (seekTimer == 1) lCCZ = seekZ - 4.0f; 
            if (seekTimer <= 65) {
                float t = seekTimer / 65.0f; t = t * t * (3.0f - 2.0f * t); 
                dCZ = camZ + (lCCZ - camZ) * t; dCY = camYaw + (3.14159f - camYaw) * t;
            } else {
                dCZ = lCCZ; dCY = 3.14159f; 
                if (seekTimer > 90) dCP = sinf(seekTimer * 1.6f) * 0.03f;
            } 
        }
        
        C3D_Mtx proj, view; 
        Mtx_PerspTilt(&proj, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false); 
        Mtx_Identity(&view); 
        
        // 1. Apply Camera Roll (Tilt)
        Mtx_RotateZ(&view, currentRoll, true);
        
        // 2. Apply Pitch and Yaw
        Mtx_RotateX(&view, -dCP, true); 
        Mtx_RotateY(&view, -dCY, true); 
        
        // 3. Apply Base Height + Bobbing
        float vY = isDead ? -0.1f : (isCrouching ? -0.4f : (hideState == NOT_HIDING || hideState == BEHIND_DOOR ? -0.9f : (hideState == IN_CABINET ? -0.7f : -0.15f)));
        float finalX = -dCX;
        float finalY = vY;
        
        // Only apply bobbing if you are walking/running (not dead or hiding)
        if (!isDead && hideState == NOT_HIDING && seekState != 1) {
            finalY -= camBobY; 
            finalX -= camBobX; 
        }
        
        Mtx_Translate(&view, finalX, finalY, -dCZ, true); 
        Mtx_Multiply(&view, &proj, &view);
        
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_proj, &view); 
        
        C3D_BufInfo* buf = C3D_GetBufInfo(); 
        BufInfo_Init(buf); 
        BufInfo_Add(buf, vbo_main, sizeof(vertex), 3, 0x210);
        
        // Draw Colored World
        if (colored_size > 0) {
            C3D_TexEnv* env = C3D_GetTexEnv(0); C3D_TexEnvInit(env);
            if (flashRedFrames > 0 && !isDead) { 
                C3D_TexEnvColor(env, 0xFF0000FF); C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            } else { 
                C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE); 
            }
            C3D_DrawArrays(GPU_TRIANGLES, 0, colored_size);
        }
        // Draw Textured World
        if (textured_size > 0) {
            C3D_TexBind(0, &atlasTex); C3D_TexEnv* env = C3D_GetTexEnv(0); C3D_TexEnvInit(env); 
            if (flashRedFrames > 0 && !isDead) { 
                C3D_TexEnvColor(env, 0xFF0000FF); C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            } else if (hasAtlas) { 
                C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR); C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR); C3D_TexEnvOpAlpha(env, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA); C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
            } else { 
                C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE); 
            }
            C3D_DrawArrays(GPU_TRIANGLES, colored_size, textured_size);
        }

        // Draw Colored Entities
        if (ent_col_size > 0) {
            C3D_TexEnv* env = C3D_GetTexEnv(0); C3D_TexEnvInit(env);
            if (flashRedFrames > 0 && !isDead) { 
                C3D_TexEnvColor(env, 0xFF0000FF); C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            } else { 
                C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE); 
            }
            C3D_DrawArrays(GPU_TRIANGLES, world_total, ent_col_size); 
        }
        // Draw Textured Entities
        if (ent_tex_size > 0) {
            C3D_TexBind(0, &atlasTex); C3D_TexEnv* env = C3D_GetTexEnv(0); C3D_TexEnvInit(env); 
            if (flashRedFrames > 0 && !isDead) { 
                C3D_TexEnvColor(env, 0xFF0000FF); C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            } else if (hasAtlas) { 
                C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR); C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR); C3D_TexEnvOpAlpha(env, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA); C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
            } else { 
                C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR); C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE); 
            }
            C3D_DrawArrays(GPU_TRIANGLES, world_total + ent_col_size, ent_tex_size);
        }

        C3D_FrameEnd(0);
        gspWaitForVBlank(); 
    }
    
    // ==========================================
    //               CLEANUP
    // ==========================================
    if (audio_ok) { 
        if (sPsst.data_vaddr) linearFree((void*)sPsst.data_vaddr); 
        if (sAttack.data_vaddr) linearFree((void*)sAttack.data_vaddr); 
        if (sCaught.data_vaddr) linearFree((void*)sCaught.data_vaddr); 
        if (sDoor.data_vaddr) linearFree((void*)sDoor.data_vaddr); 
        if (sLockedDoor.data_vaddr) linearFree((void*)sLockedDoor.data_vaddr); 
        if (sDupeAttack.data_vaddr) linearFree((void*)sDupeAttack.data_vaddr); 
        if (sRushScream.data_vaddr) linearFree((void*)sRushScream.data_vaddr); 
        if (sEyesAppear.data_vaddr) linearFree((void*)sEyesAppear.data_vaddr); 
        if (sEyesGarble.data_vaddr) linearFree((void*)sEyesGarble.data_vaddr); 
        if (sEyesAttack.data_vaddr) linearFree((void*)sEyesAttack.data_vaddr); 
        if (sEyesHit.data_vaddr) linearFree((void*)sEyesHit.data_vaddr); 
        if (sSeekRise.data_vaddr) linearFree((void*)sSeekRise.data_vaddr); 
        if (sSeekChase.data_vaddr) linearFree((void*)sSeekChase.data_vaddr); 
        if (sSeekEscaped.data_vaddr) linearFree((void*)sSeekEscaped.data_vaddr); 
        if (sDeath.data_vaddr) linearFree((void*)sDeath.data_vaddr); 
        if (sElevatorJam.data_vaddr) linearFree((void*)sElevatorJam.data_vaddr); 
        if (sElevatorJamEnd.data_vaddr) linearFree((void*)sElevatorJamEnd.data_vaddr); 
        if (sCoinsCollect.data_vaddr) linearFree((void*)sCoinsCollect.data_vaddr); 
        if (sDarkRoomEnter.data_vaddr) linearFree((void*)sDarkRoomEnter.data_vaddr); 
        if (sDrawerClose.data_vaddr) linearFree((void*)sDrawerClose.data_vaddr); 
        if (sDrawerOpen.data_vaddr) linearFree((void*)sDrawerOpen.data_vaddr); 
        if (sLightsFlicker.data_vaddr) linearFree((void*)sLightsFlicker.data_vaddr); 
        if (sWardrobeEnter.data_vaddr) linearFree((void*)sWardrobeEnter.data_vaddr); 
        if (sWardrobeExit.data_vaddr) linearFree((void*)sWardrobeExit.data_vaddr); 
        ndspExit(); 
    }
    
    C3D_TexDelete(&atlasTex); 
    linearFree(vbo_main); 
    romfsExit(); 
    C3D_Fini(); 
    gfxExit(); 
    
    return 0;
}
