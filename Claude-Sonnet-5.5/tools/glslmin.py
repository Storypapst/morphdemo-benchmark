#!/usr/bin/env python3
"""GLSL minifier used by the build.

    glslmin.py [-p prelude.glsl] body.glsl > out.glsl

Strips comments and whitespace, shortens numbers and renames every identifier that is not a
GLSL builtin. Identifiers of the optional prelude get the first short names (in order of first
appearance), so two shaders sharing one prelude contain byte-identical prelude text.
"""
import re, sys, collections, itertools

KEYWORDS = set('''float int uint bool void vec2 vec3 vec4 ivec2 ivec3 ivec4 uvec2 uvec3 uvec4 bvec2 bvec3 bvec4 mat2 mat3 mat4
 double dvec2 dvec3 dvec4 dmat2 dmat3 dmat4 const in out inout uniform layout buffer shared if else for while do return break continue discard true false
 struct main location binding std430 std140 local_size_x local_size_y local_size_z highp mediump lowp precision flat smooth
 sampler1D sampler2D sampler3D samplerCube sampler2DArray sampler2DShadow image1D image2D image3D uimage2D iimage2D
 texture textureLod textureGrad textureOffset textureProj textureGather texelFetch textureSize textureQueryLod textureLodOffset imageLoad imageStore imageSize
 atomicAdd atomicMin atomicMax atomicAnd atomicOr atomicXor atomicExchange atomicCompSwap groupMemoryBarrier memoryBarrierShared memoryBarrierBuffer
 imageAtomicAdd imageAtomicMax imageAtomicMin uint8_t packSnorm2x16 unpackSnorm2x16 packUnorm4x8 unpackUnorm4x8 uaddCarry usubBorrow umulExtended imulExtended
 coherent volatile restrict readonly writeonly rgba32f rgba16f rgba8 r32f r32ui rg32f early_fragment_tests origin_upper_left pixel_center_integer vertices max_vertices points lines triangles line_strip triangle_strip
 radians degrees sin cos tan asin acos atan sinh cosh tanh asinh acosh atanh pow exp log exp2 log2 sqrt inversesqrt
 abs sign floor trunc round roundEven ceil fract mod modf min max clamp mix step smoothstep isnan isinf fma frexp ldexp
 length distance dot cross normalize faceforward reflect refract matrixCompMult outerProduct transpose determinant inverse
 lessThan lessThanEqual greaterThan greaterThanEqual equal notEqual any all not
 floatBitsToInt floatBitsToUint intBitsToFloat uintBitsToFloat packUnorm2x16 unpackUnorm2x16 packHalf2x16 unpackHalf2x16
 bitCount findLSB findMSB bitfieldExtract bitfieldInsert bitfieldReverse dFdx dFdy fwidth barrier memoryBarrier
 length'''.split())

TOK = re.compile(r'''(?P<pp>\#[^\n]*)|(?P<num>(?:0[xX][0-9a-fA-F]+[uU]?)|(?:(?:\d+\.\d*|\.\d+|\d+)(?:[eE][+-]?\d+)?[fFuU]?))|(?P<id>[A-Za-z_]\w*)|(?P<op>\+\+|--|<<=|>>=|<<|>>|<=|>=|==|!=|&&|\|\||\^\^|\+=|-=|\*=|/=|%=|&=|\|=|\^=|.)''', re.S)

def strip_comments(s):
    s = re.sub(r'/\*.*?\*/', ' ', s, flags=re.S)
    return re.sub(r'//[^\n]*', '', s)

def tokenize(s):
    out = []
    for m in TOK.finditer(s):
        k = m.lastgroup; v = m.group()
        if k == 'op' and v.isspace(): continue
        out.append((k, v))
    return out

def shorten_num(v):
    if v.lower().startswith('0x'): return v
    suffix = ''
    if v[-1] in 'fF': v = v[:-1]
    if v[-1] in 'uU': suffix = v[-1]; v = v[:-1]
    if re.fullmatch(r'\d+', v): return v + suffix
    m = re.fullmatch(r'(\d*)\.?(\d*)(?:[eE]([+-]?\d+))?', v)
    if not m: return v + suffix
    a, b, e = m.group(1), m.group(2), m.group(3)
    b = b.rstrip('0'); a = a.lstrip('0')
    r = a + '.' + b
    if r == '.': r = '0.'
    if e: r += 'e' + e
    return r + suffix

def names():
    letters = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
    for c in letters: yield c
    for c in letters:
        for d in letters + '0123456789': yield c + d

def minify(text, prelude=''):
    ptoks = tokenize(strip_comments(prelude)) if prelude else []
    toks = tokenize(strip_comments(text))
    def user_ids(ts):
        res = []
        prev = None
        for k, v in ts:
            if k == 'id' and v not in KEYWORDS and not v.startswith('gl_') and not (prev and prev[1] == '.'):
                res.append(v)
            prev = (k, v)
        return res
    pids = user_ids(ptoks)
    ids = user_ids(toks)
    cnt = collections.Counter(pids + ids)
    order = []
    for v in pids:
        if v not in order: order.append(v)
    rest = [v for v, _ in cnt.most_common() if v not in order]
    order += rest
    gen = names(); mp = {}
    for v in order: mp[v] = next(gen)
    out = []
    prev = None
    def emit(tok):
        nonlocal prev
        k, v = tok
        if k == 'pp':
            if out and not out[-1].endswith('\n'): out.append('\n')
            out.append(v.strip() + '\n'); prev = None; return
        if k == 'id':
            if v in mp and not (prev and prev[1] == '.'): v = mp[v]
        elif k == 'num': v = shorten_num(v)
        if out and prev and prev[0] != 'pp':
            a = out[-1][-1]; b = v[0]
            need = (a.isalnum() or a == '_') and (b.isalnum() or b == '_' or (b == '.' and a.isdigit()))
            need = need or (a in '+-' and b == a) or (a.isdigit() and b == '.' and False)
            if a.isalnum() and b == '.' and k == 'num': need = True
            if need: out.append(' ')
        out.append(v); prev = (k, v)
    for t in ptoks: emit(t)
    pre_txt = ''.join(out); out.clear(); prev = None
    for t in toks: emit(t)
    body_txt = ''.join(out)
    return pre_txt, body_txt, mp

if __name__ == '__main__':
    args = sys.argv[1:]
    prelude = ''; mode = 'join'
    while args and args[0].startswith('-'):
        if args[0] == '-p': prelude = open(args[1]).read(); args = args[2:]
        elif args[0] == '--prelude-only': mode = 'pre'; args = args[1:]
        elif args[0] == '--body-only': mode = 'body'; args = args[1:]
        else: raise SystemExit('unknown option ' + args[0])
    body = open(args[0]).read()
    # the #version line of the body must stay first
    lines = body.split('\n'); ver = lines[0] if lines[0].startswith('#version') else None
    if ver: body = '\n'.join(lines[1:])
    pre, txt, mp = minify(body, prelude)
    head = (ver + '\n') if ver else ''
    sys.stdout.write({'join': head + pre + txt, 'pre': head + pre, 'body': txt}[mode])
