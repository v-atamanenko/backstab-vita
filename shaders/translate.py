#import sublime
#import sublime_plugin
import re

semantics_dict = [
    {"name": "colorVarying", "semantic": "COLOR"}, # vec4
    {"name": "fogA", "semantic": "TEXCOORD4"}, # vec3
    {"name": "fogB", "semantic": "TEXCOORD5"}, # vec4
    
    {"name": "cloudUVvarying", "semantic": "TEXCOORD1"}, # vec2
    {"name": "frameFactor", "semantic": "TEXCOORD6"}, # float
    
    {"name": "progressAndAltColor", "semantic": "TEXCOORD1"}, # vec4
    {"name": "progressVarying", "semantic": "TEXCOORD2"}, # float

    {"name": "reflectedVector", "semantic": "TEXCOORD7"}, # vec3
    {"name": "screenColorVarying", "semantic": "TEXCOORD8"}, # vec3
    {"name": "shadowMapUVVarying", "semantic": "TEXCOORD9"}, # vec2
    
    {"name": "uv", "semantic": "TEXCOORD0"},
    {"name": "uv0Varying", "semantic": "TEXCOORD0"}, # vec4
    {"name": "uv1Varying", "semantic": "TEXCOORD1"}, # vec4
    {"name": "uv2Varying", "semantic": "TEXCOORD2"}, # vec4
    {"name": "uv3Varying", "semantic": "TEXCOORD3"}, # vec4
    {"name": "uv0BVarying", "semantic": "TEXCOORD4"}, # vec2
    {"name": "uv0RVarying", "semantic": "TEXCOORD5"}, # vec2
    {"name": "uvCutoff", "semantic": "TEXCOORD0"}, # float
]

# gof2
#semantics_dict = [
#   {"name": "v_color", "semantic": "COLOR"},
#   {"name": "v_VertexColor", "semantic": "COLOR"},
#   {"name": "v_DiffuseLight", "semantic": "TEXCOORD1"},
#   {"name": "v_texCoord", "semantic": "TEXCOORD0"},
#   {"name": "v_eye_dir", "semantic": "TEXCOORD2"},
#   {"name": "v_light_dir", "semantic": "TEXCOORD3"},
#   {"name": "v_lightvec", "semantic": "TEXCOORD4"},
#   {"name": "v_normal", "semantic": "TEXCOORD5"},
#   {"name": "v_specular_dir", "semantic": "TEXCOORD6"}
#]

# gof2hd:
#semantics_dict = [
#   {"name": "uv", "semantic": "TEXCOORD0"},
#   {"name": "v_texCoord", "semantic": "TEXCOORD0"},
#   {"name": "v_FogFactor", "semantic": "FOG"},
#   {"name": "v_color", "semantic": "COLOR"},
#   {"name": "v_VertexColor", "semantic": "COLOR"},
#   {"name": "DiffuseColor", "semantic": "COLOR"},
#   {"name": "v_DiffuseLight", "semantic": "TEXCOORD1"},
#   {"name": "v_eye_dir", "semantic": "TEXCOORD1"},
#   {"name": "v_light_dir", "semantic": "TEXCOORD2"},
#   {"name": "v_lightvec", "semantic": "TEXCOORD3"},
#   {"name": "v_normal", "semantic": "TEXCOORD8"},
#   {"name": "v_normalView", "semantic": "TEXCOORD5"},
#   {"name": "v_reflectdir", "semantic": "TEXCOORD6"},
#   {"name": "v_rimFactor", "semantic": "TEXCOORD7"},
#   {"name": "v_specular_dir", "semantic": "TEXCOORD7"},
#   {"name": "v_SpecularLight", "semantic": "TEXCOORD8"},
#   {"name": "v_z", "semantic": "TEXCOORD9"}
#]

def get_semantic(n):
    for s in semantics_dict:
        if s["name"] == n:
            return s["semantic"]
    return "?SEMANTIC?"

def func_append_statement(f, s, t):
    stack = 0
    startIndex = None
    fun = ""

    for i in range(t.find(f), len(t)):
        if t[i] == '{':
            if stack == 0:
                startIndex = i + 1 # string to extract starts one index later
            
            # push to stack
            stack += 1
        elif t[i] == '}':
            # pop stack
            stack -= 1

            if stack == 0:
                fun = t[startIndex:i]
                break

    fun_new = fun + "\n\t" + s + "\n"

    if len(fun) > len(f):
        return t.replace(fun, fun_new)
    else:
        print("ERROR: Could not find function " + f)
        return t


def fetch_and_remove_varyings(text):
    pattern = re.compile(r'varying\s+(\w+)\s+([\w\]\[\d]+)\s*;[\t ]*\n?')
    res = []
    for (t, n) in re.findall(pattern, text):
        print("Found var "+ n)
        res.append({"type": t, "name": n, "semantic": get_semantic(n)})
    text = re.sub(pattern, "", text)

    if "gl_Position" in text:
        res.append({"type": "float4", "name": "gl_Position", "semantic": "POSITION"})

    if "gl_FragCoord" in text:
        res.append({"type": "float4", "name": "gl_FragCoord", "semantic": "WPOS"})

    if "gl_FragColor" in text:
        res.append({"type": "float4", "name": "gl_FragColor", "semantic": "COLOR"})

    return text, res


def fetch_and_remove_attributes(text):
    pattern = re.compile(r'attribute\s+(\w+)\s+([\w\]\[\d]+)\s*;[\t ]*\n?')
    res = []
    for (t, n) in re.findall(pattern, text):
        print("Found attr "+ n)
        res.append({"type": t, "name": n})
    text = re.sub(pattern, "", text)
    return text, res


def format(input):
    isFrag = "gl_FragColor" in input

    ## Add profile comment for psp2cgc
    if isFrag:
        input = "// profile sce_fp_psp2\n\n" + input
    else:
        input = "// profile sce_vp_psp2\n\n" + input

    # Add newlines between statements
    input = re.sub(r";[\t ]*(\w)", r";\n\1", input)

    ## Remove precision specification from uniforms
    input = re.sub("uniform\s+highp", "uniform", input)
    input = re.sub("uniform\s+mediump", "uniform", input)
    input = re.sub("uniform\s+lowp", "uniform", input)

    ## Remove precision specification from attributes
    input = re.sub("attribute\s+highp", "attribute", input)
    input = re.sub("attribute\s+mediump", "attribute", input)
    input = re.sub("attribute\s+lowp", "attribute", input)

    ## Remove precision specification from varyings
    input = re.sub("varying\s+highp", "varying", input)
    input = re.sub("varying\s+mediump", "varying", input)
    input = re.sub("varying\s+lowp", "varying", input)

    ## Remove other precision specifications
    input = re.sub("highp\s+", "", input)
    input = re.sub("mediump\s+", "", input)
    input = re.sub("lowp\s+", "", input)

    ## Remove global precision specification
    input = re.sub("precision\s+float\s*;\s*\n?", "", input)

    ## Replace ivecN types with Cg equivalents
    input = input.replace("ivec4", "int4")
    input = input.replace("ivec3", "int3")
    input = input.replace("ivec2", "int2")
    
    ## Replace vecN types with Cg equivalents
    input = input.replace("vec4", "float4")
    input = input.replace("vec3", "float3")
    input = input.replace("vec2", "float2")

    ## Replace matN types with Cg equivalents
    input = input.replace("mat4", "float4x4")
    input = input.replace("mat3", "float3x3")
    input = input.replace("mat2", "float2x2")

    ## Replace sampler* types with Cg equivalents
    input = input.replace("samplerCube", "samplerCUBE")

    ## Replace texture2D and textureCube initializers with Cg equivalents
    input = re.sub(r"texture2D[\t\n ]*\(", "tex2D(", input)
    input = re.sub(r"textureCube[\t\n ]*\(", "texCUBE(", input)
    
    ## Replace mix() with lerp()
    input = re.sub(r"([;}{)( \t\n=\*/\-\+])mix[\t\n ]*\(", r"\1lerp(", input)

    # Make sure there is an empty line before void main() {}
    input = re.sub(r";[\s\n]*void[\s\n]+main[\s\n]*\(", ";\n\nvoid main(", input)

    ## Fix "void main(void)"
    input = re.sub("void\s+main\s*\n*\(\s*void\s*\)\s*\{", "void main() {", input)

    ## For frag shaders, update function definition and make it return gl_FragColor
    #if (isFrag):
    #   input = re.sub("void\s+main\s*\n*\(\s*\)\s*\{", "float4 main() {\n\tfloat4 gl_FragColor;\n", input)
    #   input = func_append_statement("float4 main()", "return gl_FragColor;", input)

    ## For vert shaders, make sure function definition is standardized
    #if not isFrag:
    input = re.sub("void\s+main\s*\n*\(\s*\)\s*\{\n?", "void main() {\n", input)

    ## For frag shaders, move varyings into main() args
    if isFrag:
        varyings = []
        input, varyings = fetch_and_remove_varyings(input)
        
        varyings_str = ''
        for v in varyings:
            if v["name"] == "gl_FragColor":
                varyings_str += "\n\t" + v["type"] + " out " + v["name"] + " : " + v["semantic"] + ","
            else:
                varyings_str += "\n\t" + v["type"] + " " + v["name"] + " : " + v["semantic"] + ","
        if (len(varyings) != 0):
            varyings_str = varyings_str[:-1] + "\n"
        input = input.replace("void main()", "void main("+varyings_str+")")

    ## For vert shaders, move attributes and varyings into main() args
    if not isFrag:
        attributes = []
        input, attributes = fetch_and_remove_attributes(input)
        varyings = []
        input, varyings = fetch_and_remove_varyings(input)

        attrs_str = ''
        for v in attributes:
            attrs_str += "\n\t" + v["type"] + " " + v["name"] + ","

        if len(varyings) == 0:
            attrs_str = attrs_str[:-1] + "\n"

        varyings_str = ''
        for v in varyings:
            varyings_str += "\n\t" + v["type"] + " out " + v["name"] + " : " + v["semantic"] + ","

        if (len(varyings) != 0):
            varyings_str = varyings_str[:-1] + "\n"

        input = input.replace("void main()", "void main("+attrs_str+varyings_str+")")

    # Ensure newline at the end of file
    if input[-1] != "\n":
        input += "\n"

    ## Clean up excess whitespaces
    input = re.sub(r"\n[     ]*\n", "\n\n", input)
    input = re.sub(";[   ]*\n", ";\n", input)
    input = re.sub("\}[  ]*\n", "}\n", input)
    input = re.sub("\{\n\n", "{\n", input)
    input = re.sub(r"\n   ([\w/])", r"\n\t\1", input)
    input = re.sub(r"uniform[\t ]+(\w+)[\t ]+([\w\]\[\d]+)[\t ]*;[\t ]*\n?", r"uniform \1 \2;\n", input)

    return input

import os, fnmatch

def findReplace(directory, filePattern):
    for path, dirs, files in os.walk(os.path.abspath(directory)):
        for filename in fnmatch.filter(files, filePattern):
            targetpath = path.replace(directory, "cg")
            os.makedirs(targetpath, exist_ok=True)
            targetfilepath = os.path.join(targetpath, filename.replace(".glsl", ".cg"))

            filepath = os.path.join(path, filename)
            with open(filepath) as f:
                s = f.read()
            s = format(s)
            with open(targetfilepath, "w") as f:
                f.write(s)

if __name__ == "__main__":
    findReplace("glsl", "*.glsl")
