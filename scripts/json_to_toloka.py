import argparse
import sys
import time
import json
import csv
import random
import os
from yandex.Translater import Translater

random.seed(42)


def clean_text(text):
    return text.replace("\t", " ").replace("\n", " ").replace('"', "'")


def main(json_file_name, output_dir, honey_file_name,
         pool_size, honey_size, existing_markup_file,
         use_translation, skip, tr_key):
    honey = []
    with open(honey_file_name, "r") as r:
        header = tuple(next(r).strip().split("\t"))
        if not use_translation:
            assert header == ("INPUT:url", "INPUT:title", "INPUT:text", "GOLDEN:res")
        else:
            assert header == ("INPUT:url", "INPUT:title", "INPUT:text", "GOLDEN:res", "INPUT:trans_title", "INPUT:trans_text")
        for line in r:
            if not use_translation:
                url, title, text, res = line.strip().split("\t")
                honey.append({"url": url, "title": title, "text": text, "res": res})
            else:
                url, title, text, res, trans_title, trans_text = line.strip().split("\t")
                honey.append({"url": url, "title": title, "text": text, "res": res, "trans_title": trans_title, "trans_text": trans_text})

    existing_urls = set()
    if existing_markup_file:
        with open(existing_markup_file, "r") as r:
            header = tuple(next(r).strip().split("\t"))
            assert header[0] == "INPUT:url"
            for line in r:
                url = line.strip().split("\t")[0]
                existing_urls.add(url)

    tr = Translater()
    tr.set_key(tr_key)
    tr.set_from_lang("en")
    tr.set_to_lang("ru")

    with open(json_file_name, "r") as r:
        records = json.load(r)
        random.shuffle(records)
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
                    if use_translation:
                        pool_record["trans_title"] = r["trans_title"]
                        pool_record["trans_text"] = r["trans_text"]
                    clean_pool.append(pool_record)
                random.shuffle(honey)
                current_pool = clean_pool + honey[:honey_size]
                random.shuffle(current_pool)

                pool_num = (i - 1) // 1000
                pool_file_name = "pool_{}.tsv".format(pool_num)
                pool_file_name = os.path.join(output_dir, pool_file_name)
                with open(pool_file_name, "w", encoding="utf-8") as w:
                    header = "INPUT:url\tINPUT:title\tINPUT:text\tGOLDEN:res";
                    if use_translation:
                        header += "\tINPUT:trans_title\tINPUT:trans_text";
                    w.write(header + "\n")
                    writer = csv.writer(w, delimiter='\t', quoting=csv.QUOTE_MINIMAL)
                    for r in current_pool:
                        row = [r["url"], r["title"], r["text"], r["res"]]
                        if use_translation:
                            row += [r["trans_title"], r["trans_text"]]
                        writer.writerow(tuple(row))
                current_pool = []
            if record["url"] in existing_urls or not clean_text(record["text"]) or not clean_text(record["title"]):
                print("Skipping: {}".format(record["url"]))
                continue
            if use_translation:
                try:
                    tr.set_text(clean_text(record["title"]))
                    record["trans_title"] = tr.translate()
                    tr.set_text(clean_text(record["text"]))
                    record["trans_text"] = tr.translate()
                except Exception as e:
                    print("Translation failed: {}".format(e))
                    continue

            print("OK")
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
    parser.add_argument("--use-translation", default=False, action="store_true")
    parser.add_argument("--skip", default=0, type=int)
    parser.add_argument("--tr-key", type=str, default=None)
    args = parser.parse_args()
    main(**vars(args))

