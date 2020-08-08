import argparse
import csv
import json
from collections import defaultdict

from sklearn.metrics import classification_report


def read_markup(file_name):
    with open(file_name, "r") as r:
        reader = csv.reader(r, delimiter='\t', quotechar='"')
        header = next(reader)
        for row in reader:
            assert len(header) == len(row)
            record = dict(zip(header, row))
            yield record


def calc_threads_metrics(clustering_markup, original_jsonl, threads_json, errors_tsv, output_json):
    markup = defaultdict(dict)
    for record in read_markup(clustering_markup):
        first_url = record["INPUT:first_url"]
        second_url = record["INPUT:second_url"]
        quality = int(record["OUTPUT:quality"] == "OK")
        markup[(first_url, second_url)] = quality

    url2record = dict()
    filename2url = dict()
    with open(original_jsonl, "r") as r:
        for line in r:
            record = json.loads(line)
            url2record[record["url"]] = record
            filename2url[record["file_name"]] = record["url"]

    threads = []
    with open(threads_json, "r") as r:
        threads = json.load(r)
    labels = dict()
    for label, thread in enumerate(threads):
        thread_urls = {filename2url[filename] for filename in thread["articles"]}
        for url in thread_urls:
            labels[url] = label

    not_found_count = 0
    for first_url, second_url in list(markup.keys()):
        not_found_in_labels = first_url not in labels or second_url not in labels
        not_found_in_records = first_url not in url2record or second_url not in url2record
        if not_found_in_labels or not_found_in_records:
            not_found_count += 1
            markup.pop((first_url, second_url))
    print("Not found {} pairs from markup".format(not_found_count))

    targets = []
    predictions = []
    errors = []
    for (first_url, second_url), target in markup.items():
        prediction = int(labels[first_url] == labels[second_url])
        first = url2record.get(first_url)
        second = url2record.get(second_url)
        targets.append(target)
        predictions.append(prediction)
        if target == prediction:
            continue
        errors.append({
            "target": target,
            "prediction": prediction,
            "first_url": first_url,
            "second_url": second_url,
            "first_file_name": first["file_name"],
            "second_file_name": second["file_name"],
            "first_title": first["title"],
            "second_title": second["title"],
            "first_text": first["text"],
            "second_text": second["text"]
        })
    for error in errors:
        print(error["target"], error["prediction"], " ||| ", error["first_title"], " ||| ", error["second_title"])
    metrics = classification_report(targets, predictions, output_dict=True)
    metrics["categories"] = [(0, metrics.pop("0")), (1, metrics.pop("1"))]

    with open(output_json, "w") as w:
        json.dump({
            "threads_metrics": metrics,
            "threads_errors": errors
        }, w, ensure_ascii=False, indent=4)

    if errors_tsv:
        with open(errors_tsv, "w") as w:
            writer = csv.writer(w, delimiter='\t', quotechar='"')
            keys = ("first_url", "second_url", "first_title", "second_title", "first_text", "second_text")
            for error in errors:
                writer.writerow([error[key] for key in keys] + ["OK" if error["target"] == 1 else "BAD"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--clustering-markup", type=str, required=True)
    parser.add_argument("--original-jsonl", type=str, required=True)
    parser.add_argument("--threads-json", type=str, required=True)
    parser.add_argument("--output-json", type=str, required=True)
    parser.add_argument("--errors-tsv", type=str, default=None)
    args = parser.parse_args()
    calc_threads_metrics(**vars(args))
