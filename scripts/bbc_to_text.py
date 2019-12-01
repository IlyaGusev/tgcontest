import argparse
import os
import csv
import random

def main(input_directory, output_file):
    assert os.path.exists(input_directory)
    records = []
    for rubric_dir in os.listdir(input_directory):
        rubric_dir = os.path.join(input_directory, rubric_dir)
        if not os.path.isdir(rubric_dir):
            continue
        for file_name in os.listdir(rubric_dir):
            file_name = os.path.join(rubric_dir, file_name)
            with open(file_name, "r") as r:
                try:
                    content = r.read().replace("\n", " ")
                except Exception as e:
                    continue
                records.append(content)

    with open(output_file, "w") as w:
        for r in records:
            w.write("{}\n".format(r))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-directory", type=str, required=True)
    parser.add_argument("--output-file", type=str, required=True)
    args = parser.parse_args()
    main(**vars(args))

