"""Stage A resource builds only. Never invoke a target or change SDK policy."""
from pathlib import Path
import hashlib, importlib.util, json, shutil, subprocess, sys, time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
RALLY = ROOT.parent / 'AgonArcade'
SDK = ROOT.parent / 'agon-vdplet'
BUILD = HERE / 'build'
EVIDENCE = HERE / 'evidence'
SHA = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()

def run(args):
    return subprocess.check_output([str(x) for x in args], stderr=subprocess.STDOUT, text=True)

def verify_manifest(relative, digest):
    path = RALLY / relative
    assert SHA(path) == digest, path
    rows = json.loads(path.read_text())['files']
    for row in rows:
        source = RALLY / row['path']
        assert source.stat().st_size == row['bytes'] and SHA(source) == row['sha256'], source
    return {'path': str(path), 'sha256': digest, 'files_verified': len(rows)}

def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
    return mod

def main():
    start = time.monotonic()
    BUILD.mkdir(exist_ok=True); EVIDENCE.mkdir(exist_ok=True)
    assert SHA(HERE.parent / 'README.md') == '896e9bd4467a5ef0ec32a817ed6a627798d5d638ed46857fdacbc12ab87d8ae2'
    pin = json.loads((ROOT / 'docs/tasks/GOLEM-037/helper-pin.json').read_text())
    manifests = [verify_manifest('docs/tasks/RALLY-25/route-kernels/MANIFEST.json',
                    'd9e5238859d6faa402dd069f145834cd3eabb47f46db5b91e1d276ad359e8296'),
                 verify_manifest('docs/tasks/RALLY-25/renderer-helper/MANIFEST.json',
                    '3fc5d32f16d84f8ecc53aabe2d418e317a5323aee1ea2543c85fd657b1d6f62c')]
    for name, digest in pin['golem_source_inputs'].items():
        path = ROOT / name; assert SHA(path) == digest
        assert hashlib.sha256(subprocess.check_output(['git','-C',ROOT,'show','b32c596:'+name])).hexdigest() == digest
        target = BUILD / 'closure' / name; target.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(path,target)
    helper = BUILD / 'helper'; helper.mkdir(exist_ok=True)
    for name, digest in pin['copied_sources'].items():
        source = ROOT / 'docs/tasks/GOLEM-037/helper' / name; assert SHA(source) == digest
        shutil.copyfile(source, helper / name)
    for source, expected in [(SDK/'tools/build_module.py',pin['sdk_builder_sha256']),
                             (SDK/'include/vdplet.h',pin['sdk_header_sha256'])]:
        assert SHA(source) == expected
    compiler = BUILD / 'compiler'
    run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',
         BUILD/'closure/docs/tasks/GOLEM-036/right-shift/compiler.cpp','-o',compiler])
    source_dir = RALLY / 'docs/tasks/RALLY-25/route-kernels'
    bindings_path = RALLY / 'docs/tasks/RALLY-25/generated-state/bindings.json'
    bindings = json.loads(bindings_path.read_text())
    # Identity of fixture maps is independently recorded by Rally's master receipt.
    owner_result = json.loads((source_dir/'RESULTS.json').read_text())
    key = str(bindings_path.relative_to(RALLY))
    assert SHA(bindings_path) == owner_result['dependency_sha256'][key]
    fit = load('resource_tools', ROOT/'docs/tasks/GOLEM-037/measure_fit.py')
    fit.BUILD = BUILD; fit.EVIDENCE = EVIDENCE
    builder = load('sdk_builder', SDK/'tools/build_module.py')
    report = {'schema':1,'scope':'resource-only real-source composition; no execution or D2 qualification',
              'manifest_inputs':manifests,'compiler_inputs':pin['golem_source_inputs'],
              'fixture_bindings_sha256':SHA(bindings_path),'plan_sha256':SHA(HERE.parent/'README.md'),
              'measurement_script_sha256':SHA(HERE/'measure.py'),
              'scaffold_template_sha256':SHA(HERE/'scaffold.c.in'),
              'sdk_commit':run(['git','-C',SDK,'rev-parse','HEAD']).strip(),
              'sdk_files':{n:SHA(SDK/n) for n in ('include/vdplet.h','src/vdplet_runtime.h','src/vdplet_image.h','tools/build_module.py')},
              'compiler':run([fit.COMPILER,'--version']).splitlines()[0],
              'compiler_sha256':SHA(fit.COMPILER),'layouts':{}}
    for name in ('default','relocated'):
        generated = BUILD / name; mapping = bindings[name]
        command = [compiler,source_dir/'kernels.golem',generated,mapping['prefix']]
        for row in mapping['bindings']: command += ['--bind',*row]
        for array in ('routes','sine'):command += ['--array-data',array+'='+str(source_dir/(array+'.bin'))]
        run(command)
        # Compare actual generated files with the peer's immutable result hashes.
        compared = 0
        for filename, info in owner_result['generated_artifacts'][name].items():
            assert SHA(generated/filename)==info['sha256'],(name,filename)
            compared += 1
        interface = json.loads((generated/'interface.json').read_text())
        original = (generated/'native.c').read_text()
        assert original.count('visibility("default")') == 1
        (generated/'internal.c').write_text(original.replace('visibility("default")','visibility("hidden")'))
        layout = {'root':interface['state_buffer'],
                  'globals':{g['name']:g['offset'] for g in interface['globals']},
                  'arrays':{a['name']:a for a in interface['arrays']},
                  'entries':{e['name']:e['logical_id'] for e in interface['entries']}}
        template = (HERE/'scaffold.c.in').read_text()
        template = template.replace('@NATIVE@',str(generated/'internal.c')).replace('@HELPER@',str(helper/'rally_render.c'))
        for field in ('worldTick','playerStation','playerLateral'):
            template = template.replace('@'+field+'@',str(layout['globals'][field]))
        for field in ('position','lateral','active'):
            template = template.replace('@'+field+'@',str(layout['arrays'][field]['buffer']))
        for field in ('Tick120','ResetRace','StopRace'):
            template = template.replace('@'+field+'@',str(layout['entries'][field]))
        template = template.replace('@ROOT@',str(layout['root']))
        assert '@' not in template
        scaffold = generated/'scaffold.c'; scaffold.write_text(template)
        source_only = fit.linked_build(builder,generated/'native.c',name+'-source')
        combined = fit.linked_build(builder,scaffold,name+'-combined')
        report['layouts'][name] = {'matched_peer_generated_files':compared,
            'root_bytes':interface['state_bytes'],'retained_array_bytes':sum(a['bytes'] for a in interface['arrays']),
            'source_native_sha256':SHA(generated/'native.c'),'scaffold_sha256':SHA(scaffold),
            'layout':layout,'source_only':source_only,'combined':combined}
        for f in ('native.c','scaffold.c','interface.json','root.json'):
            shutil.copyfile(generated/f,EVIDENCE/(name+'-'+f))
        print(name,'source/combined text',source_only['resident_text_bytes'],combined['resident_text_bytes'],flush=True)
    (EVIDENCE/'resources.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Resource-only build seconds:',round(time.monotonic()-start,3))

if __name__=='__main__':main()
