#!/usr/bin/python
# -*- coding: utf-8 -*-

import argparse
import json
import os
import requests
import tqdm


def send_put(protocol, host, file_name, ttl, content):
    req = requests.Request(
        'PUT',
        '{}://{}/{}'.format(protocol, host, file_name),
        headers = {
            "Cache-Control": "max-age={}".format(ttl)
        },
        data = content
    )
    prepared = req.prepare()

    s = requests.Session()
    r = s.send(prepared)
    r.raise_for_status()
    return r

def read_lang_file(lang_file):
    with open(lang_file, 'r') as f:
        data = json.load(f)
    files = set()
    for lang in data:
        if lang['lang_code'] not in ('ru', 'en'):
            continue
        for name in lang['articles']:
            files.add(name)
    return files


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('host', metavar='<host>')
    parser.add_argument('dir', metavar='<article_dir>')
    parser.add_argument('count', type=int, metavar='<docs_count>')
    parser.add_argument('--protocol', default='http')
    parser.add_argument('--port', type=int, default=None)
    parser.add_argument('--lang_file', default=None)
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    host = args.host
    if args.port:
        host = '{}:{}'.format(host, args.port)

    n = args.count

    lang_files = None
    if args.lang_file:
        lang_files = read_lang_file(args.lang_file)

    with tqdm.tqdm(total=n) as pbar:
        for path, _, files in os.walk(args.dir):
            for name in files:
                if not name.endswith('.html'):
                    continue
                if lang_files and not name in lang_files:
                    continue
                if n <= 0:
                    exit()
                ttl = 30*24*60*60
                # ttl = random.randint(5*60, 30*24*60*60)
                with open(os.path.join(path, name), 'r') as f:
                    content = f.read().strip()
                r = send_put(args.protocol, host, name, ttl, content)
                # print(r)
                pbar.update(1)
                n = n - 1
    print('Total files: ', args.count - n)
