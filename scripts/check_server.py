#!/usr/bin/python
# -*- coding: utf-8 -*-

import argparse
import json
import os
import requests
import subprocess
import sys
import time


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


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('dir', metavar='<article_dir>')
    parser.add_argument('--port', type=int, default=1994)
    return parser.parse_args()


def call_server(args):
    print('Launching server')
    proc = subprocess.Popen(['./tgnews', 'server', str(args.port)])
    time.sleep(5)

    print('Sending PUT requests')
    host = 'localhost:{}'.format(args.port)
    for path, _, files in os.walk(args.dir):
        for name in files:
            if not name.endswith('.html'):
                continue
            print(name)
            ttl = 3600*8
            with open(os.path.join(path, name), 'r') as f:
                content = f.read().strip().encode('utf-8')
            r = send_put('http', host, name, ttl, content)
            r.raise_for_status()
    time.sleep(60)

    print('Calling /threads')
    r = requests.get('http://{}/threads'.format(host), params={'period': int(1e8), 'lang_code': 'en', 'category': 'any'})
    r.raise_for_status()

    proc.kill()

    return r.json()


def call_cli(args):
    print('Launching cli')
    proc = subprocess.run(['./tgnews', 'top', args.dir], capture_output=True)

    return json.loads(proc.stdout)


if __name__ == "__main__":
    args = parse_args()

    srv_content = call_server(args)
    cli_content = call_cli(args)

    # TODO: write robust compare function
    sys.exit()

    assert(json.dumps(srv_content) == json.dumps(cli_content))
