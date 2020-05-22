#!/usr/bin/python
# -*- coding: utf-8 -*-

import argparse
import random
import requests
import os

from collections import deque


def print_request(request):
    req = "{method} {path_url} HTTP/1.1\r\n{headers}\r\n{body}".format(
        method = request.method,
        path_url = request.path_url,
        headers = ''.join('{0}: {1}\r\n'.format(k, v) for k, v in request.headers.items()),
        body = request.body or "",
    )
    return "{req_size}\n{req}\r\n".format(req_size = len(req), req = req)


def make_put(protocol, host, file_name, ttl, content):
    req = requests.Request(
        'PUT',
        '{}://{}/{}'.format(protocol, host, file_name),
        headers = {
            "Cache-Control": "max-age={}".format(ttl)
        },
        data = content
    )
    prepared = req.prepare()
    return print_request(prepared)


def make_delete(protocol, host, file_name):
    req = requests.Request(
        'DELETE',
        '{}://{}/{}'.format(protocol, host, file_name)
    )
    prepared = req.prepare()
    return print_request(prepared)



def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('host', metavar='<host>')
    parser.add_argument('dir', metavar='<article_dir>')
    parser.add_argument('count', type=int, metavar='<ammo_count>')
    parser.add_argument('--protocol', default='http')
    parser.add_argument('--port', type=int, default=None)
    parser.add_argument('--mode', choices=['put', 'mix'], default='put')
    parser.add_argument('--prob', type=float, default=0.05)
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    host = args.host
    if args.port:
        host = '{}:{}'.format(host, args.port)

    n = args.count
    put_names = deque()

    for path, _, files in os.walk(args.dir):
        for name in files:
            if n <= 0:
                exit()
            ttl = random.randint(5*60, 30*24*60*60)
            with open(os.path.join(path, name), 'r') as f:
                content = f.read()
            print(make_put(args.protocol, host, name, ttl, content))
            if args.mode == 'mix':
                put_names.append(name)
                if random.random() < args.prob and len(put_names):
                    print(make_delete(args.protocol, host, put_names.popleft()))
            n = n - 1
