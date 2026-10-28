#!/usr/bin/env python3
import argparse, datetime
from xml.sax.saxutils import quoteattr

def tree(paths):
    root = {}
    for path in paths:
        node = root
        for part in path.split("/")[:-1]:
            node = node.setdefault(part, {})
    return root

def emit(node, depth, out):
    for name in sorted(node):
        pad = "\t" * depth
        if node[name]:
            out.append("%s<dir targ_name=%s>" % (pad, quoteattr(name)))
            emit(node[name], depth + 1, out)
            out.append("%s</dir>" % pad)
        else:
            out.append("%s<dir targ_name=%s />" % (pad, quoteattr(name)))

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--out", required=True)
    p.add_argument("--content-id", required=True)
    p.add_argument("files", nargs="+")
    a = p.parse_args()
    files = [f.replace("\\", "/") for f in a.files]
    stamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    x = ['<?xml version="1.0"?>',
         '<psproject xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" fmt="gp4" version="1000">',
         "\t<volume>", "\t\t<volume_type>pkg_ps4_app</volume_type>", "\t\t<volume_id>PS4VOLUME</volume_id>",
         "\t\t<volume_ts>%s</volume_ts>" % stamp,
         '\t\t<package content_id=%s passcode="00000000000000000000000000000000" storage_type="digital50" app_type="full" />' % quoteattr(a.content_id),
         '\t\t<chunk_info chunk_count="1" scenario_count="1"><chunks><chunk id="0" layer_no="0" label="Chunk #0" /></chunks><scenarios default_id="0"><scenario id="0" type="sp" initial_chunk_count="1" label="Scenario #0">0</scenario></scenarios></chunk_info>',
         "\t</volume>", '\t<files img_no="0">']
    for f in files:
        x.append('\t\t<file targ_path=%s orig_path=%s />' % (quoteattr(f), quoteattr(f)))
    x.extend(["\t</files>", "\t<rootdir>"])
    emit(tree(files), 2, x)
    x.extend(["\t</rootdir>", "</psproject>"])
    with open(a.out, "w", newline="\n") as out:
        out.write("\n".join(x) + "\n")
if __name__ == "__main__":
    main()
