import argparse
import os
import csv
import json
import random

def main(input_file, output_file, part):
    assert os.path.exists(input_file)
    records = []
    cat2res = {
        "POLITICS": "society",
        "ENTERTAINMENT": "entertainment",
        "BUSINESS": "economy",
        "CRIME": "society",
        "ARTS & CULTURE": "entertainment",
        "CULTURE & ARTS": "entertainment",
        "TECH": "technology",
        "SCIENCE": "science",
        "SPORTS": "sports",
        "HEALTHY LIVING": "not_news",
        "THE WORLDPOST": "society"
    }

    with open(input_file, "r") as r:
        for line in r:
            data = json.loads(line)
            title = data["headline"]
            text = data["short_description"]
            data["title"] = title
            data["text"] = text
            category = data["category"]
            if category in cat2res:
                data["res"] = cat2res[category]
                records.append(data)
            else:
                print("Skipping: ", category, title)

    with open(output_file, "w") as w:
        for r in records:
            if random.random() > part:
                continue
            w.write("__label__{} {} {}\n".format(r["res"], r["title"], r["text"]))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-file", type=str, required=True)
    parser.add_argument("--output-file", type=str, required=True)
    parser.add_argument("--part", type=float, default=0.05)
    args = parser.parse_args()
    main(**vars(args))

