import argparse
import os
import csv
import json
import random

def main(input_file, output_file, part):
    assert os.path.exists(input_file)

    records = []
    with open(input_file, "r") as r:
        for line in r:
            data = json.loads(line)
            title = data["headline"]
            text = data["short_description"]
            records.append(title + " " + text)

    with open(output_file, "w") as w:
        for r in records:
            if random.random() > part:
                continue
            w.write(r + "\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-file", type=str, required=True)
    parser.add_argument("--output-file", type=str, required=True)
    parser.add_argument("--part", type=float, default=1.0)
    args = parser.parse_args()
    main(**vars(args))

