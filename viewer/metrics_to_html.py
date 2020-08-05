import os
import json
import argparse

from jinja2 import Environment, FileSystemLoader

def metrics_to_html(
    categories_json,
    threads_json,
    templates_dir,
    output_dir,
    language,
    version,
    date
):
    file_loader = FileSystemLoader(templates_dir)
    env = Environment(loader=file_loader)
    metrics_template = env.get_template("metrics.html")
    cat_metrics = None
    cat_errors = None
    if categories_json:
        with open(categories_json, "r") as r:
            categories = json.load(r)
            cat_metrics = categories["categories_metrics"]
            cat_errors = categories["categories_errors"]
    with open(os.path.join(output_dir, "metrics.html"), "w", encoding="utf-8") as w:
         w.write(metrics_template.render(
            cat_metrics=cat_metrics,
            cat_errors=cat_errors,
            language=language,
            current_page="metrics.html",
            version=version,
            date=date
         ))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--categories-json", type=str, default=None)
    parser.add_argument("--threads-json", type=str, default=None)
    parser.add_argument("--templates-dir", type=str, default="templates")
    parser.add_argument("--output-dir", type=str, default="output")
    parser.add_argument("--language", type=str, required=True)
    parser.add_argument('--version', type=str, default="3.0.0")
    parser.add_argument('--date', type=str, default="03 May")
    args = parser.parse_args()
    metrics_to_html(**vars(args))
