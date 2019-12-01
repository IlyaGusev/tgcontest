import argparse
import os
import csv
import json
import random

def main(input_file, original_file, output_file, cat_file):
    records = []
    if input_file and original_file:
        with open(input_file, "r") as r, open(original_file, "r") as original:
            original_data = {rec["title"]: rec for rec in json.load(original)}
            for line in r:
                line = line.strip()
                label = int(line.split(" ")[0][-1])
                title = " ".join(line.split(" ")[1:])
                if title not in original_data:
                    continue
                text = original_data[title]["text"]
                content = title + " " + text
                content = content.replace("\n", " ").strip()
                records.append({
                    "res": "news" if label == 1 else "not_news",
                    "content": content
                })

    if cat_file:
        with open(cat_file, "r") as r:
            for line in r:
                label = line.split(" ")[0].split("__")[-1]
                content = " ".join(line.split(" ")[1:])
                content = content.replace("\n", " ").strip()
                if label != "not_news":
                    label = "news"
                records.append({
                    "res": label,
                    "content": content
                })

    if not records:
        return
    random.shuffle(records)
    with open(output_file, "w") as w:
        for r in records:
            w.write("__label__{} {}\n".format(r["res"], r["content"]))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-file", type=str, default=None)
    parser.add_argument("--original-file", type=str, default=None)
    parser.add_argument("--output-file", type=str, required=True)
    parser.add_argument("--cat-file", type=str, default=None)
    args = parser.parse_args()
    main(**vars(args))

