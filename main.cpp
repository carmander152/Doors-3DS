#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdlib.h> 
#include <math.h> 
#include "vshader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float pos[4]; float clr[4]; } vertex;

static const vertex level_mesh[] = {
	// Hallway
	{{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
	{{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.4f, 0.2f, 0.1f, 1.0f }},
	{{ -1.0f, 0.0f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
	{{ -1.0f, 1.5f, -3.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 1.5f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }}, {{ -1.0f, 0.0f,  0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }},
	{{  1.0f, 0.0f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
	{{  1.0f, 1.5f, -3.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 0.0f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }}, {{  1.0f, 1.5f,  0.0f, 1.0f }, { 0.3f, 0.3f, 0.3f, 1.0f }},
	// Door
	{{ -0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{  0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{ -0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
	{{  0.5f, 0.0f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{  0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }}, {{ -0.5f, 1.3f, -3.0f, 1.0f }, { 0.25f, 0.15f, 0.05f, 1.0f }},
};

int main() {
	gfxInitDefault();
	gfxSet3D(false); 
	irrstInit(); 
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgram_s program;
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);

	int uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx");
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 4); 
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 4); 

	void* vbo_data = linearAlloc(sizeof(level_mesh));
	memcpy(vbo_data, level_mesh, sizeof(level_mesh));
	GSPGPU_FlushDataCache(vbo_data, sizeof(level_mesh)); 

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

	float camX = 0.0f, camZ = 1.0f, camYaw = 0.0f, camPitch = 0.0f;

	while (aptMainLoop()) {
		hidScanInput();
		irrstScanInput(); 
		if (hidKeysDown() & KEY_START) break;

		// C-Stick (Camera)
		circlePosition cStick;
		irrstCstickRead(&cStick);
		if (abs(cStick.dx) > 10) camYaw -= cStick.dx / 1560.0f * 0.15f;
		if (abs(cStick.dy) > 10) camPitch += cStick.dy / 1560.0f * 0.15f; 

		// Circle Pad (Movement)
		circlePosition cPad;
		hidCircleRead(&cPad);
		if (abs(cPad.dy) > 10 || abs(cPad.dx) > 10) {
			float speed = 0.08f;
			float stickX = cPad.dx / 1560.0f;
			float stickY = cPad.dy / 1560.0f;

			// Forward/Backward based on Facing Angle
			camX += (sinf(camYaw) * stickY + cosf(camYaw) * stickX) * speed;
			camZ -= (cosf(camYaw) * stickY - sinf(camYaw) * stickX) * speed;
		}

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); 
		C3D_FrameDrawOn(target);

		C3D_Mtx projection;
		Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
		
		C3D_Mtx view;
		Mtx_Identity(&view);
		Mtx_RotateX(&view, -camPitch, true); 
		Mtx_RotateY(&view, -camYaw, true);   
		Mtx_Translate(&view, -camX, -0.8f, -camZ, true); 

		C3D_Mtx projView;
		Mtx_Multiply(&projView, &projection, &view);
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projView);
		
		C3D_DrawArrays(GPU_TRIANGLES, 0, 18); // Walls/Floor
		C3D_DrawArrays(GPU_TRIANGLES, 18, 6); // Door
		C3D_FrameEnd(0);
	}

	linearFree(vbo_data);
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
	C3D_Fini();
	gfxExit();
	return 0;
}
