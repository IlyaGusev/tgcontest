import argparse
import os
import csv
import random

def main(input_directory, output_file):
    rubrics = {}
    assert os.path.exists(input_directory)
    rubrics[os.path.join(input_directory, "business")] = "economy"
    rubrics[os.path.join(input_directory, "entertainment")] = "entertainment"
    rubrics[os.path.join(input_directory, "politics")] = "society"
    rubrics[os.path.join(input_directory, "sport")] = "sports"
    rubrics[os.path.join(input_directory, "tech")] = "technology"
    records = []
    for directory, label in rubrics.items():
        for file_name in os.listdir(directory):
            file_name = os.path.join(directory, file_name)
            with open(file_name, "r") as r:
                try:
                    content = r.read().replace("\n", " ")
                except Exception as e:
                    continue
                records.append({"res": label, "content": content})

    with open(output_file, "w") as w:
        for r in records:
            w.write("__label__{} {}\n".format(r["res"], r["content"]))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-directory", type=str, required=True)
    parser.add_argument("--output-file", type=str, required=True)
    args = parser.parse_args()
    main(**vars(args))

