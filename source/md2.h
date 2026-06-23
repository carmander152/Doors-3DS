#ifndef MD2_H
#define MD2_H

#include <vector>
#include "render_utils.h"

struct MD2Model {
    int numFrames;
    int numVerts;
    int numTris;
    int frameSize;
    int ofsFrames;
    int current_anim_frame = 0;
    int current_anim_slice_prog = 0;
    const char* current_anim_name;
    const char* model_name;
    
    std::vector<float> uvs; 
    std::vector<int> triVerts; 
    std::vector<int> triUVs;   
    std::vector<std::vector<float>> frameVerts; 

    bool load(const char* filepath, bool is_animation, const char* file_name);
    void draw(MD2Model animation_model,int frame, float x, float y, float z, float scale, float L, float rotY);
    void load_anim();
};

#endif
