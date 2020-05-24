#!/usr/bin/python
# -*- coding: utf-8 -*-

import argparse
import json
import os
import random
import requests
import tqdm

from collections import deque


PERIOD_RANGE = (300, 86400)
LANGS = ['ru', 'en']
CATEGORIES = ['society', 'economy', 'technology', 'sports', 'entertainment', 'science', 'other']


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


def make_threads(protocol, host, period, lang_code, category):
    req = requests.Request(
        'GET',
        '{}://{}/threads'.format(protocol, host),
        params = { 'period': period, 'lang_code': lang_code, 'category': category }
    )
    prepared = req.prepare()
    return print_request(prepared)


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
    parser.add_argument('count', type=int, metavar='<ammo_count>')
    parser.add_argument('--protocol', default='http')
    parser.add_argument('--port', type=int, default=None)
    parser.add_argument('--mode', choices=['put', 'mix'], default='put')
    parser.add_argument('--delete_prob', type=float, default=0.05)
    parser.add_argument('--threads_prob', type=float, default=0.05)
    parser.add_argument('--lang_file', default=None)
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    host = args.host
    if args.port:
        host = '{}:{}'.format(host, args.port)

    n = args.count
    put_names = deque()

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
                ttl = random.randint(5*60, 30*24*60*60)
                with open(os.path.join(path, name), 'r') as f:
                    content = f.read().strip()
                print(make_put(args.protocol, host, name, ttl, content))

                if args.mode == 'mix':
                    put_names.append(name)
                    if random.random() < args.delete_prob and len(put_names):
                        print(make_delete(args.protocol, host, put_names.popleft()))
                    if random.random() < args.threads_prob:
                        period = random.randint(PERIOD_RANGE[0], PERIOD_RANGE[1])
                        lang = random.choice(LANGS)
                        category = random.choice(CATEGORIES)
                        print(make_threads(args.protocol, host, period, lang, category))
                pbar.update(1)
                n = n - 1
    print('Total files: ', args.count - n)
