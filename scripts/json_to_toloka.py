import argparse
import sys
import json
import csv
import random
import os

random.seed(42)


def clean_text(text):
    return text.replace("\t", " ").replace("\n", " ")


def main(json_file_name, output_dir, honey_file_name, pool_size, honey_size, existing_markup_file):
    honey = []
    with open(honey_file_name, "r") as r:
        header = tuple(next(r).strip().split("\t"))
        assert header == ("INPUT:url", "INPUT:title", "INPUT:text", "GOLDEN:res")
        for line in r:
            url, title, text, res = line.strip().split("\t")
            honey.append({"url": url, "title": title, "text": text, "res": res})

    existing_urls = set()
    if existing_markup_file:
        with open(existing_markup_file, "r") as r:
            header = tuple(next(r).strip().split("\t"))
            assert header[0] == "INPUT:url"
            for line in r:
                url = line.strip().split("\t")[0]
                existing_urls.add(url)

    with open(json_file_name, "r") as r:
        records = json.load(r)
        random.shuffle(records)
        current_pool = []
        i = 0
        for record in records:
            if i % pool_size == 0 and i != 0:
                clean_pool = []
                for r in current_pool:
                    title = clean_text(r["title"])
                    text = clean_text(r["text"])
                    url = r["url"]
                    clean_pool.append({"url": url, "title": title, "text": text, "res": ""})
                random.shuffle(honey)
                current_pool = clean_pool + honey[:honey_size]
                random.shuffle(current_pool)

                pool_num = (i - 1) // 1000
                pool_file_name = "pool_{}.tsv".format(pool_num)
                pool_file_name = os.path.join(output_dir, pool_file_name)
                with open(pool_file_name, "w", encoding="utf-8") as w:
                    w.write("INPUT:url\tINPUT:title\tINPUT:text\tGOLDEN:res\n")
                    writer = csv.writer(w, delimiter='\t', quoting=csv.QUOTE_MINIMAL)
                    for r in current_pool:
                        writer.writerow((r["url"], r["title"], r["text"], r["res"]))
                current_pool = []
            if record["url"] in existing_urls or not clean_text(record["text"]) or not clean_text(record["title"]):
                continue
            current_pool.append(record)
            i += 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--json-file-name", type=str, required=True)
    parser.add_argument("--output-dir", type=str, required=True)
    parser.add_argument("--honey-file-name", type=str, required=True)
    parser.add_argument("--pool-size", type=int, default=200)
    parser.add_argument("--honey-size", type=int, default=50)
    parser.add_argument("--existing-markup-file", type=str, default=None)
    args = parser.parse_args()
    main(**vars(args))

