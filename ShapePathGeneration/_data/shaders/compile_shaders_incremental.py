from pathlib import Path
import re
import os
import subprocess
import json

local_path = './sources/'


# TODO: support shader variants via defines

def gen_out_name(f_in):
    f_out = 'dx11/' + Path(f_in).stem + '.bin'
    return f_out

def compile_shader(f_in, t):
    f_out = gen_out_name(f_in)
    _platform = 'windows'
    _p = '...'
    _type = '...' 
    
    if t == 'vs':
        _p = 'vs_5_0'
        _type = 'v'
        
    if t == 'fs':
        _p = 'ps_5_0'
        _type = 'f'
    
    if t == 'cs':
        _p = 'cs_5_0'
        _type = 'c'
    
    result = ''
    try:
        result = subprocess.check_output(f'shaderc -f {f_in} -o {f_out} --type {_type} --platform {_platform} -p {_p} --depends', stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print(e.output.decode('utf-8'))
        return False
    return True



def get_file_mtime(filename):
    f_stat = os.stat(filename)
    return f_stat.st_mtime_ns

def load_mtime_table(filename):
    if os.path.exists(filename):
        d = json.load(open(filename))
        return d
    else:
        return dict()
        
def save_mtime_table(filename, table):
    json.dump(table, open(filename, 'w'))


def create_mtime_table(path):
    path = Path(path)
    table = dict()
    for f in path.iterdir():
        if f.is_dir():
            continue
        mtime = get_file_mtime(f)
        table[f.name] = mtime
    return table

def get_depends(f_in):
    f_out = gen_out_name(f_in)
    f_dep = f_out + '.d'
    if not os.path.exists(f_dep):
        return []
    result = []
    with open(f_dep, 'r') as dep_file:
        for elem in dep_file.read().split('\\')[1:]:
            name = Path(elem.strip()).name
            result.append(name)
        #print(text)
    return result
            
            

def file_was_modified(filename, old_mtime_table, new_mtime_table):
    if filename not in old_mtime_table:
        return True
    if filename not in new_mtime_table:
        return True
    old_mtime = old_mtime_table[filename]
    new_mtime = new_mtime_table[filename]
    if old_mtime < new_mtime:
        return True
    return False


shader_ext = ".glsl"
shader_patt         = re.compile(r"(\w\w)_(\w*)")
shader_patt_main    = re.compile(r"(\w\w)_(\w*)_main\.glsl")
def get_shader_type(filename):   
    ext = filename.suffix
    if ext != shader_ext:
        return None    
    m = re.match(shader_patt, filename.name)
    if m is None:
        return None   
    if m.groups()[0] == 'vs':
        return 'vs'
    if m.groups()[0] == 'fs':
        return 'fs'
    if m.groups()[0] == 'cs':
        return 'cs'    
    return None

# shader variant compilation
variant_patt = re.compile(r"VARIANT(\w+)")
def get_shader_variants(filename):
    result = []
    with open(filename, "r") as source_file:
        source = source_file.read()
        ms = re.findall(variant_patt, source)
        for m in ms:
            result.append(ms.groups()[0])
    return result
        


mtime_table_filename = 'mtime_table.json'
force_recompile = False
# TODO: shader variant compilation
def main():
    
    shaders_to_compile = []
    
    old_mtime_table = load_mtime_table(mtime_table_filename)
    new_mtime_table = create_mtime_table(local_path)
    #print(old_mtime_table)
    #print(new_mtime_table)
    
    def modified(filename):
        return file_was_modified(filename, old_mtime_table, new_mtime_table)
    
    for f in Path(local_path).iterdir():
        if f.is_dir():
            continue
        shader_type = get_shader_type(f)
        if shader_type is not None:
            #print(get_depends(f.name))
            if force_recompile:
                shaders_to_compile.append((f.name, shader_type))
                continue
            if modified(f.name):
                shaders_to_compile.append((f.name, shader_type))
            else:
                for dep in get_depends(f.name):
                    if(modified(dep)):
                        shaders_to_compile.append((f.name, shader_type))
                        break
        
        #shaders_to_compile.append((name, t))
    
    count = len(shaders_to_compile)
    success_count = 0
    print(f'compiling {count} shaders')
    
    for i, (name, t) in enumerate(shaders_to_compile):
        print(f'[{i+1}/{count}] {name}')
        success = compile_shader(local_path + name, t)
        success_count += int(success)
        # TODO: don't modify mtime table if failed to compile
    
    
    failed_count = count - success_count
    print('\n---------------------')
    if failed_count == 0:
        print(f'SUCCESS: {count} shaders compiled successfully')
    else:
        print(f'ERROR: {failed_count} failed, {success_count} succeded')
        
    save_mtime_table(mtime_table_filename, new_mtime_table)
        


if __name__ == '__main__':
    main()