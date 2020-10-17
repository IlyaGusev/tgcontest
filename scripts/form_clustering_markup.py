import argparse
import csv
import json
import random
from collections import defaultdict

from util import read_markup_tsv, write_markup_tsv

def main(original_jsonl, threads_json, honey_tsv, current_markup_tsv, output_tsv):
    url2record = dict()
    filename2url = dict()
    with open(original_jsonl, "r") as r:
        for line in r:
            record = json.loads(line)
            url2record[record["url"]] = record
            filename2url[record["file_name"]] = record["url"]

    existing_urls = set()
    if current_markup_tsv:
        current_markup = read_markup_tsv(current_markup_tsv)
        existing_urls = {(r["first_url"], r["second_url"]) for r in current_markup}
        existing_urls |= {(r["second_url"], r["first_url"]) for r in current_markup}
    honey_records = read_markup_tsv(honey_tsv)

    prev_thread_url = None
    markup_keys = []
    with open(threads_json, "r") as r:
        threads = json.load(r)
        for thread in threads:
            if len(thread) == 1:
                prev_thread_url = thread[0]
                continue
            thread_urls = list({filename2url[filename] for filename in thread["articles"]})
            random.shuffle(thread_urls)
            first_url = thread_urls[0]
            for second_url in thread_urls[1:]:
                if key in existing_urls:
                    continue
                if random.random() < 0.8:
                    key = (first_url, second_url)
                    markup_keys.append(key)
                    existing_urls.add(key)
            key = (prev_thread_url, first_url)
            if prev_thread_url and random.random() < 0.00001 and key not in existing_urls:
                markup_keys.append(key)
                existing_urls.add(key)
            prev_thread_url = first_url

    markup = []
    for url1, url2 in markup_keys:
        first = url2record[url1]
        second = url2record[url2]
        markup.append({
            "first_url": url1,
            "second_url": url2,
            "first_title": first["title"],
            "second_title": second["title"],
            "first_text": first["text"],
            "second_text": second["text"]
        })

    markup_len = len(honey_records) * 9
    random.shuffle(markup)
    final_markup = markup[:markup_len] + honey_records
    random.shuffle(final_markup)
    print(len(final_markup))

    write_markup_tsv(final_markup, output_tsv)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--original-jsonl", type=str, required=True)
    parser.add_argument("--threads-json", type=str, required=True)
    parser.add_argument("--honey-tsv", type=str, required=True)
    parser.add_argument("--current-markup-tsv", type=str, default=None)
    parser.add_argument("--output-tsv", type=str, required=True)
    args = parser.parse_args()
    main(**vars(args))
