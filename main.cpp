#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "vshader_shbin.h" // FIX: Underscore instead of a dot!

typedef struct { float pos[3]; float clr[3]; } vertex;

static const vertex hallway_mesh[] = {
    // Floor
    {{ -1.0f, 0.0f, -3.0f }, { 0.2f, 0.1f, 0.0f }}, {{  1.0f, 0.0f, -3.0f }, { 0.2f, 0.1f, 0.0f }}, {{ -1.0f, 0.0f,  0.0f }, { 0.2f, 0.1f, 0.0f }},
    {{  1.0f, 0.0f, -3.0f }, { 0.2f, 0.1f, 0.0f }}, {{  1.0f, 0.0f,  0.0f }, { 0.2f, 0.1f, 0.0f }}, {{ -1.0f, 0.0f,  0.0f }, { 0.2f, 0.1f, 0.0f }},
    // Left Wall
    {{ -1.0f, 0.0f, -3.0f }, { 0.3f, 0.3f, 0.3f }}, {{ -1.0f, 1.5f, -3.0f }, { 0.3f, 0.3f, 0.3f }}, {{ -1.0f, 0.0f,  0.0f }, { 0.3f, 0.3f, 0.3f }},
};

int main() {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, 0x01001000);

    // 1. Load the Shader
    DVLB_s* vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgram_s program;
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
    C3D_BindProgram(&program);

    // 2. Setup the Camera (Projection Matrix)
    int uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "proj_mtx");
    C3D_Mtx projection;
    Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);

    // 3. Map our 3D data to the shader
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // Position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 3); // Color

    void* vbo_data = linearAlloc(sizeof(hallway_mesh));
    memcpy(vbo_data, hallway_mesh, sizeof(hallway_mesh));
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x000000FF, 0); // Black background
        C3D_FrameDrawOn(target);

        // 4. Send the Camera to the Shader and Draw!
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
        C3D_DrawArrays(GPU_TRIANGLES, 0, 9);

        C3D_FrameEnd(0);
    }

    linearFree(vbo_data);
    shaderProgramFree(&program);
    DVLB_Free(vshader_dvlb);
    C3D_Fini();
    gfxExit();
    return 0;
}
