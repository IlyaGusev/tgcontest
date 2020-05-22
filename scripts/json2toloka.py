import os
import argparse
import json
import csv
import random

def clean_text(text):
    return text.replace("\t", " ").replace("\n", " ").replace('"', "'")


def write_pool(file_path, pool):
    with open(file_path, "w", encoding="utf-8") as w:
        header = "INPUT:url\tINPUT:title\tINPUT:text\tGOLDEN:res";
        w.write(header + "\n")
        writer = csv.writer(w, delimiter='\t', quoting=csv.QUOTE_MINIMAL)
        for r in pool:
            row = (r["url"], r["title"], r["text"], r["res"])
            writer.writerow(row)
        print("{} created!".format(file_path))

def read_honey(honey_path):
    honey = []
    with open(honey_path, "r") as r:
        header = tuple(next(r).strip().split("\t"))
        assert header == ("INPUT:url", "INPUT:title", "INPUT:text", "GOLDEN:res")
        for line in r:
            url, title, text, res = line.strip().split("\t")
            honey.append({"url": url, "title": title, "text": text, "res": res})
    return honey

def main(original_json_path, output_dir, honey_path, honey_size, pool_size, skip):
    records = []
    with open(original_json_path, "r") as r:
        records = json.load(r)
    random.shuffle(records)


    honey = read_honey(honey_path)

    current_pool = []
    i = 0
    for record in records[skip:]:
        if i % pool_size == 0 and i != 0:
            clean_pool = []
            for r in current_pool:
                title = clean_text(r["title"])
                text = clean_text(r["text"])
                url = r["url"]
                pool_record = {"url": url, "title": title, "text": text, "res": ""}
                clean_pool.append(pool_record)
            random.shuffle(honey)
            current_pool = clean_pool + honey[:honey_size]
            random.shuffle(current_pool)

            pool_num = (i - 1) // pool_size
            pool_file_name = "pool_{}.tsv".format(pool_num)
            pool_file_name = os.path.join(output_dir, pool_file_name)
            write_pool(pool_file_name, current_pool)
            current_pool = []
        if not clean_text(record["text"]) or not clean_text(record["title"]):
           print("Skipping: {}".format(record["url"]))
           continue
        current_pool.append(record)
        i += 1



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--original-json-path", type=str, required=True)
    parser.add_argument("--output-dir", type=str, required=True)
    parser.add_argument("--honey-path", type=str, required=True)
    parser.add_argument("--pool-size", type=int, default=80)
    parser.add_argument("--honey-size", type=int, default=20)
    parser.add_argument("--skip", type=int, default=0)
    args = parser.parse_args()
    main(**vars(args))
