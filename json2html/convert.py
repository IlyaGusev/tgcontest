import os
import json
import copy
import argparse
import shutil
from datetime import datetime
from jinja2 import Environment, FileSystemLoader

def convert(templates_dir, output_dir, documents_file, tops_file, languages, version):
    file_loader = FileSystemLoader(templates_dir)
    env = Environment(loader=file_loader)
    rubric_template = env.get_template("rubric.html")
    with open(documents_file, "r") as r:
        documents = json.load(r)
        documents = {doc["file_name"]: doc for doc in documents}
    with open(tops_file, "r") as r:
        tops = json.load(r)
    icons_dir = os.path.join(output_dir, "icons")
    os.makedirs(icons_dir, exist_ok=True)
    templates_icons_dir = os.path.join(templates_dir, "icons")
    for file_name in os.listdir(templates_icons_dir):
        shutil.copyfile(os.path.join(templates_icons_dir, file_name), os.path.join(icons_dir, file_name))
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
            top["clusters"] = [cluster for cluster in clusters if cluster["articles"]]
            top["category"] = category
            output_file_name = os.path.join(lang_dir, category + ".html")
            with open(output_file_name, "w", encoding="utf-8") as w:
                w.write(rubric_template.render(
                    top=top,
                    version=version,
                    language=language
                ))
    index_file = os.path.join(output_dir, "index.html")
    with open(index_file, "w", encoding="utf-8") as w:
        base_template = env.get_template("index.html")
        w.write(base_template.render(
            version=version,
            language="en"
        ))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--templates-dir', type=str, default="templates")
    parser.add_argument('--output-dir', type=str, default="output")
    parser.add_argument('--documents-file', type=str, required=True)
    parser.add_argument('--tops-file', type=str, required=True)
    parser.add_argument('--languages', type=str, default="ru,en")
    parser.add_argument('--version', type=str, default="2.0.0")
    args = parser.parse_args()
    convert(**vars(args))

