# Requires shadercross CLI installed from SDL_shadercross

mkdir -p "../Compiled/SPIRV"
mkdir -p "../Compiled/MSL"
mkdir -p "../Compiled/DXIL"
mkdir -p "../Compiled/JSON"

for filename in *.vert.hlsl; do
    if [ -f "$filename" ]; then
        shadercross "$filename" -o "../Compiled/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filename" -o "../Compiled/MSL/${filename/.hlsl/.msl}"
        shadercross "$filename" -o "../Compiled/DXIL/${filename/.hlsl/.dxil}"
        shadercross "$filename" -o "../Compiled/JSON/${filename/.hlsl/.json}"
    fi
done

for filename in *.frag.hlsl; do
    if [ -f "$filename" ]; then
        shadercross "$filename" -o "../Compiled/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filename" -o "../Compiled/MSL/${filename/.hlsl/.msl}"
        shadercross "$filename" -o "../Compiled/DXIL/${filename/.hlsl/.dxil}"
        shadercross "$filename" -o "../Compiled/JSON/${filename/.hlsl/.json}"
    fi
done

for filename in *.comp.hlsl; do
    if [ -f "$filename" ]; then
        shadercross "$filename" -o "../Compiled/SPIRV/${filename/.hlsl/.spv}"
        shadercross "$filename" -o "../Compiled/MSL/${filename/.hlsl/.msl}"
        shadercross "$filename" -o "../Compiled/DXIL/${filename/.hlsl/.dxil}"
        shadercross "$filename" -o "../Compiled/JSON/${filename/.hlsl/.json}"
    fi
done
