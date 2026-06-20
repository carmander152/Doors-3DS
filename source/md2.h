#ifndef MD2_H
#define MD2_H

#include <vector>
#include "render_utils.h"

struct MD2Model {
    int numFrames;
    int numVerts;
    int numTris;
    
    std::vector<float> uvs; 
    std::vector<int> triVerts; 
    std::vector<int> triUVs;   
    std::vector<std::vector<float>> frameVerts; 

    bool load(const char* filepath, const char* file_name);
    void draw(int frame, float x, float y, float z, float scale, float L, float rotY);
};

#endif
