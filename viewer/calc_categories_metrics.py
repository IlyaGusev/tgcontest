import json
import argparse

from sklearn.metrics import classification_report


def calc_categories_metrics(documents_file, cat_gold, cat_output, output_json):
    with open(documents_file, "r") as r:
        orig_records = json.load(r)
        assert orig_records
        first_record = orig_records[0]
        assert first_record.get("file_name")
        assert first_record.get("url")
        orig_records = {r["file_name"]: r for r in orig_records}
        url_to_record = {r["url"]: r for _, r in orig_records.items()}
    with open(cat_gold, "r") as r:
        cat_gold_records = json.load(r)
        assert cat_gold_records
        first_record = cat_gold_records[0]
        assert first_record.get("url")
        assert first_record.get("category")
        cat_gold_records = {r["url"]: r["category"] for r in cat_gold_records}
    with open(cat_output, "r") as r:
        cat_output_data = json.load(r)
        assert cat_output_data
        first_category = cat_output_data[0]
        assert first_category.get("category")
        assert first_category.get("articles") is not None
        cat_output_records = dict()
        for chunk in cat_output_data:
            category = chunk["category"]
            if category == "any":
                continue
            for file_name in chunk["articles"]:
                rec = orig_records[file_name]
                if rec["url"] in cat_gold_records:
                    cat_output_records[rec["url"]] = category
        assert len(cat_output_records) == len(cat_gold_records)

    cat_output_records = list(sorted(cat_output_records.items()))
    predicted_cats = [v for k, v in cat_output_records]
    target_cats = [v for k, v in sorted(cat_gold_records.items())]

    unique_cats = list(set(predicted_cats).union(set(target_cats)))
    cat2label = {cat: i for i, cat in enumerate(unique_cats)}
    label2cat = {label: cat for cat, label in cat2label.items()}

    predicted_labels = [cat2label[cat] for cat in predicted_cats]
    target_labels = [cat2label[cat] for cat in target_cats]

    cat_metrics = classification_report(target_labels, predicted_labels, output_dict=True)
    cat_metrics["categories"] = []
    for label, cat in sorted(label2cat.items(), key=lambda x: -cat_metrics[str(x[0])]["support"]):
        cat_metrics["categories"].append((label2cat[int(label)], cat_metrics.pop(str(label))))

    cat_errors = []
    for i, (pred, target) in enumerate(zip(predicted_labels, target_labels)):
        if pred == target:
            continue
        record = url_to_record[cat_output_records[i][0]]
        cat_errors.append({
            "title": record["title"],
            "url": record["url"],
            "prediction": label2cat[pred],
            "target": label2cat[target]
        })

    with open(output_json, "w") as w:
        json.dump({
            "categories_metrics": cat_metrics,
            "categories_errors": cat_errors
        }, w, ensure_ascii=False, indent=4)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--documents-file", type=str, required=True)
    parser.add_argument("--cat-gold", type=str, required=True)
    parser.add_argument("--cat-output", type=str, required=True)
    parser.add_argument("--output-json", type=str, required=True)
    args = parser.parse_args()
    calc_categories_metrics(**vars(args))
