import argparse
import csv
import json
import random

def main(tg_json_file, lenta_csv_file, output_file, lenta_part):
    with open(tg_json_file, "r") as r, open(output_file, "w") as w, open(lenta_csv_file, "r") as lenta:
        data = json.load(r)
        records = []
        for d in data:
            title = d["title"].replace("\n", " ").strip()
            text = d["text"].replace("\n", " ").strip()
            records.append((title, text))
        print(len(records))
        next(lenta)
        reader = csv.reader(lenta, delimiter=',')
        for row in reader:
            _, title, text, _, _ = row
            title = title.replace("\n", " ").strip()
            text = text.replace("\n", " ").strip()
            if random.random() < lenta_part:
                records.append((title, text))
        print(len(records))

        random.shuffle(records)
        for title, text in records:
            w.write("{} {}\n".format(title, text))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--tg-json-file", type=str, required=True)
    parser.add_argument("--lenta-csv-file", type=str, required=True)
    parser.add_argument("--output-file", type=str, required=True)
    parser.add_argument("--lenta-part", type=float, default=0.2)
    args = parser.parse_args()
    main(**vars(args))

