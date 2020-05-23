import os
import json
import copy
import argparse
import shutil
from datetime import datetime
from jinja2 import Environment, FileSystemLoader

def convert(templates_dir, output_dir, documents_file, tops_file, languages, version, nclusters):
    file_loader = FileSystemLoader(templates_dir)
    env = Environment(loader=file_loader)
    rubric_template = env.get_template("rubric.html")
    with open(documents_file, "r") as r:
        documents = json.load(r)
        documents = {doc["file_name"]: doc for doc in documents}
    with open(tops_file, "r") as r:
        tops = json.load(r)
    static_dirs = ("icons", "css", "js")
    for dir_name in static_dirs:
        static_output_dir = os.path.join(output_dir, dir_name)
        static_input_dir = os.path.join(templates_dir, dir_name)
        os.makedirs(static_output_dir, exist_ok=True)
        for file_name in os.listdir(static_input_dir):
            from_file = os.path.join(static_input_dir, file_name)
            to_file = os.path.join(static_output_dir, file_name)
            shutil.copyfile(from_file, to_file)
    languages = languages.split(",")
    assert languages
    for language in languages:
        lang_dir = os.path.join(output_dir, language)
        os.makedirs(lang_dir, exist_ok=True)
        for top in tops:
            top = copy.deepcopy(top)
            clusters = top.pop("threads")
            category = top.pop("category")
            category = category if category != "any" else "main"
            for cluster in clusters:
                articles = [documents[file_name] for file_name in cluster.pop("articles")]
                for article in articles:
                    article["date"] = datetime.utcfromtimestamp(article["timestamp"]).strftime("%e %b %H:%M")
                cluster["articles"] = [a for a in articles if a["language"] == language]
                cluster["date"] = articles[0]["date"]
                cluster["size"] = len(cluster["articles"])
                cluster["best_date"] = datetime.utcfromtimestamp(cluster["best_time"]).strftime("%e %b %H:%M")
            top["clusters"] = [cluster for cluster in clusters if cluster["articles"]]
            if nclusters:
                top["clusters"] = top["clusters"][:nclusters]
            top["category"] = category
            current_page = category + ".html"
            output_file_name = os.path.join(lang_dir, current_page)
            with open(output_file_name, "w", encoding="utf-8") as w:
                w.write(rubric_template.render(
                    top=top,
                    version=version,
                    language=language,
                    current_page=current_page
                ))
    shutil.copyfile(os.path.join(templates_dir, "index.html"), os.path.join(output_dir, "index.html"))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--templates-dir', type=str, default="templates")
    parser.add_argument('--output-dir', type=str, default="output")
    parser.add_argument('--documents-file', type=str, required=True)
    parser.add_argument('--tops-file', type=str, required=True)
    parser.add_argument('--languages', type=str, default="ru,en")
    parser.add_argument('--version', type=str, default="2.0.0")
    parser.add_argument('--nclusters', type=int, default=200)
    args = parser.parse_args()
    convert(**vars(args))

