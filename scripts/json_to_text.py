import argparse
import os
import csv
import random
import json


def main(input_file, output_file):
    assert os.path.exists(input_file)
    with open(input_file, "r") as r, open(output_file, "w") as w:
        data = json.load(r)
        for record in data:
            w.write(record["title"].replace("\n", " ") + " " + record["text"].replace("\n", " ") + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-file", type=str, required=True)
    parser.add_argument("--output-file", type=str, required=True)
    args = parser.parse_args()
    main(**vars(args))

