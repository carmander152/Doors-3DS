import os

# Headers first, then source files.
HEADER_ORDER = ["game_state.h", "render_utils.h", "physics.h", "atlas_uvs.h", "world_gen.h", "md2.h", "entities.h", "entity_ai.h"]
SOURCE_ORDER = ["game_state.cpp", "render_utils.cpp", "physics.cpp", "world_gen.cpp", "md2.cpp", "entities.cpp", "entity_ai.cpp", "main.cpp"]

def bundle():
    source_dir = "source" 
    output_file = "source/main_bundle.cpp"
    
    with open(output_file, "w") as out:
        out.write("// =========================================================\n")
        out.write("// AUTO-GENERATED BUNDLE FILE - DO NOT EDIT DIRECTLY\n")
        out.write("// =========================================================\n\n")
        
        # System libs.
        out.write("#include <3ds.h>\n#include <citro3d.h>\n#include <tex3ds.h>\n")
        out.write("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <math.h>\n")
        out.write("#include <vector>\n#include <time.h>\n\n")

        # Shader header.
        out.write('#include "vshader_shbin.h"\n\n')

        # Headers.
        for h in HEADER_ORDER:
            path = os.path.join(source_dir, h)
            if os.path.exists(path):
                out.write(f"// =========================================================\n")
                out.write(f"// SECTION: {h}\n")
                out.write(f"// =========================================================\n")
                with open(path, "r") as f:
                    for line in f:
                        # Skip local includes and pragma once.
                        if not line.strip().startswith('#include "') and not line.strip().startswith('#pragma once'):
                            out.write(line)
                out.write("\n\n")

        # Source files.
        for s in SOURCE_ORDER:
            path = os.path.join(source_dir, s)
            if os.path.exists(path):
                out.write(f"// =========================================================\n")
                out.write(f"// SECTION: {s}\n")
                out.write(f"// =========================================================\n")
                with open(path, "r") as f:
                    for line in f:
                        # Skip local includes.
                        if not line.strip().startswith('#include "'):
                            out.write(line)
                out.write("\n\n")

    print(f"Successfully bundled code into {output_file}")

if __name__ == "__main__":
    bundle()
