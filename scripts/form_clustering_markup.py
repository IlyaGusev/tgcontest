import argparse
import csv
import json
import random
from collections import defaultdict


def main(original_jsonl, threads_json, honey_tsv):
    url2record = dict()
    filename2url = dict()
    with open(original_jsonl, "r") as r:
        for line in r:
            record = json.loads(line)
            url2record[record["url"]] = record
            filename2url[record["file_name"]] = record["url"]
    prev_thread_url = None
    markup = []
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
                if random.random() < 0.5:
                    markup.append((first_url, second_url))
            if prev_thread_url and random.random() < 0.1:
                markup.append((prev_thread_url, first_url))
            prev_thread_url = first_url
    full_markup = []
    for url1, url2 in markup:
        first = url2record[url1]
        second = url2record[url2]
        full_markup.append({
            "first_url": url1,
            "second_url": url2,
            "first_title": first["title"],
            "second_title": second["title"],
            "first_text": first["text"],
            "second_text": second["text"]
        })

    honey = []
    with open(honey_tsv, "r") as r:
        header = tuple([h.split(":")[-1] for h in next(r).strip().split("\t")])
        reader = csv.reader(r, delimiter='\t', quotechar='"')
        for row in reader:
            record = dict(zip(header, row))
            honey.append(record)
    markup_len = len(honey) * 9
    random.shuffle(full_markup)

    final_markup = full_markup[:markup_len] + honey
    random.shuffle(final_markup)
    print(len(final_markup))
    with open("markup.tsv", "w") as w:
        w.write("\t".join([
            "INPUT:first_title", "INPUT:second_title",
            "INPUT:first_url", "INPUT:second_url",
            "INPUT:first_text", "INPUT:second_text",
            "GOLDEN:quality"
        ]) + "\n")
        writer = csv.writer(w, delimiter='\t', quotechar='"')
        for record in final_markup:
            writer.writerow((
                record["first_title"], record["second_title"], record["first_url"],
                record["second_url"], record["first_text"], record["second_text"], record.get("quality", "")
            ))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--original-jsonl", type=str, required=True)
    parser.add_argument("--threads-json", type=str, required=True)
    parser.add_argument("--honey-tsv", type=str, required=True)
    args = parser.parse_args()
    main(**vars(args))
