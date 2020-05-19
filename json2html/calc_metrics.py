import os
import json
import argparse

from jinja2 import Environment, FileSystemLoader
from sklearn.metrics import classification_report

def calc_metrics(
    templates_dir,
    output_dir,
    documents_file,
    cat_gold,
    cat_output,
    threads_gold,
    threads_output,
    language
):
    with open(documents_file, "r") as r:
        orig_records = json.load(r)
        assert orig_records
        first_record = orig_records[0]
        assert first_record.get("file_name")
        assert first_record.get("url")
        orig_records = {r["file_name"]: r for r in orig_records}
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
    predicted_cats = [v for k, v in sorted(cat_output_records.items())]
    target_cats = [v for k, v in sorted(cat_gold_records.items())]
    cat2label = {cat: i for i, cat in enumerate(list(set(predicted_cats).union(set(target_cats))))}
    label2cat = {label: cat for cat, label in cat2label.items()}
    predicted_labels = [cat2label[cat] for cat in predicted_cats]
    target_labels = [cat2label[cat] for cat in target_cats]
    cat_metrics = classification_report(target_labels, predicted_labels, output_dict=True)

    cat_metrics["categories"] = []
    for label, cat in sorted(label2cat.items(), key=lambda x: -cat_metrics[str(x[0])]["support"]):
        cat_metrics["categories"].append((label2cat[int(label)], cat_metrics.pop(str(label))))
    file_loader = FileSystemLoader(templates_dir)
    env = Environment(loader=file_loader)
    metrics_template = env.get_template("metrics.html")
    with open(os.path.join(output_dir, "metrics.html"), "w", encoding="utf-8") as w:
         w.write(metrics_template.render(
            cat_metrics=cat_metrics,
            language=language,
            current_page="metrics.html"
         ))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--templates-dir", type=str, default="templates")
    parser.add_argument("--output-dir", type=str, default="output")
    parser.add_argument("--documents-file", type=str, required=True)
    parser.add_argument("--cat-gold", type=str, required=True)
    parser.add_argument("--cat-output", type=str, required=True)
    parser.add_argument("--threads-gold", type=str, default=None)
    parser.add_argument("--threads-output", type=str, default=None)
    parser.add_argument("--language", type=str, required=True)
    args = parser.parse_args()
    calc_metrics(**vars(args))
    args = parser.parse_args()
    calc_metrics(**vars(args))
