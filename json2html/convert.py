import json
import argparse
from datetime import datetime
from jinja2 import Environment, FileSystemLoader

def convert(template_file_name, output_file_name, documents_file, tops_file):
    file_loader = FileSystemLoader(".")
    env = Environment(loader=file_loader)
    template = env.get_template(template_file_name)
    with open(documents_file, "r") as r:
        documents = json.load(r)
        documents = {doc["file_name"]: doc for doc in documents}
    with open(tops_file, "r") as r:
        tops = json.load(r)
    main_top = [top for top in tops if top["category"] == "any"][0]["threads"][:10]
    for cluster in main_top:
        articles = [documents[file_name] for file_name in cluster.pop("articles")]
        for article in articles:
            article["date"] = datetime.utcfromtimestamp(article["timestamp"]).strftime("%e %b %H:%M")
        cluster["articles"] = articles
        cluster["date"] = articles[0]["date"]
        cluster["size"] = len(cluster["articles"])
        cluster["id"] = abs(hash(cluster["title"]) + hash(cluster["date"]) + hash(cluster["size"]))
    with open(output_file_name, "w", encoding="utf-8") as w:
        w.write(template.render(
            main_top=main_top
        ))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--template-file-name', type=str, default="template.html")
    parser.add_argument('--output-file-name', type=str, default="index.html")
    parser.add_argument('--documents-file', type=str, required=True)
    parser.add_argument('--tops-file', type=str, required=True)
    args = parser.parse_args()
    convert(**vars(args))

