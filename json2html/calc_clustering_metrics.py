import argparse
import csv
import json
from collections import defaultdict
from sklearn.metrics import classification_report


def main(clustering_markup, original_jsonl, threads_json):
    markup = defaultdict(dict)
    with open(clustering_markup, "r") as r:
        reader = csv.reader(r, delimiter='\t', quotechar='"')
        header = next(reader)
        for row in reader:
            assert len(header) == len(row)
            record = dict(zip(header, row))
            first_url = record["INPUT:first_url"]
            second_url = record["INPUT:second_url"]
            quality = int(record["OUTPUT:quality"] == "OK")
            markup[first_url][second_url] = {"target": quality}
    url2record = dict()
    filename2url = dict()
    with open(original_jsonl, "r") as r:
        for line in r:
            record = json.loads(line)
            url2record[record["url"]] = record
            filename2url[record["file_name"]] = record["url"]
    with open(threads_json, "r") as r:
        threads = json.load(r)
        for thread in threads:
            thread_urls = {filename2url[filename] for filename in thread["articles"]}
            for url in thread_urls:
                for second_url in markup.get(url, []):
                    markup[url][second_url]["prediction"] = int(second_url in thread_urls)

    targets = []
    predictions = []
    errors = []
    count_bad = 0
    for first_url, d in markup.items():
        for second_url, res in d.items():
            target = res["target"]
            if "prediction" not in res:
                count_bad += 1
                continue
            prediction = res["prediction"]
            targets.append(target)
            predictions.append(prediction)
            if target == prediction:
                continue
            first = url2record.get(first_url, {"title": None})
            second = url2record.get(second_url, {"title": None})
            errors.append({
                "target": target,
                "prediction": prediction,
                "first_url": first_url,
                "second_url": second_url,
                "first_title": first["title"],
                "second_title": second["title"]
            })
    for error in errors:
        print(error["target"], error["prediction"], " ||| ", error["first_title"], " ||| ", error["second_title"])
    print(classification_report(targets, predictions))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--clustering-markup", type=str, required=True)
    parser.add_argument("--original-jsonl", type=str, required=True)
    parser.add_argument("--threads-json", type=str, required=True)
    args = parser.parse_args()
    main(**vars(args))
